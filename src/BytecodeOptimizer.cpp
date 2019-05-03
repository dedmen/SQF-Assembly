#include "BytecodeOptimizer.hpp"
#include <intercept.hpp>

void BytecodeOptimizer::init() {
    nmap["missionnamespace"] = sqf::mission_namespace();
    nmap["uinamespace"] = sqf::ui_namespace();
    nmap["parsingnamespace"] = sqf::parsing_namespace();
    nmap["profilenamespace"] = sqf::profile_namespace();
    nmap["nil"] = game_value();

    nmap["objnull"] = sqf::obj_null();
    nmap["controlnull"] = sqf::control_null();
    nmap["displaynull"] = sqf::display_null();
    nmap["grpnull"] = sqf::grp_null();
    nmap["locationnull"] = sqf::location_null();
    nmap["scriptnull"] = sqf::script_null();
    nmap["confignull"] = sqf::config_null();
    //nmap["linebreak"] = sqf::line_break();//#TODO broken in Intercept. Doesn't return structured text
    nmap["hasinterface"] = sqf::has_interface();
    //#TODO isServer can be set if we have -server cmdline param
    nmap["configfile"] = sqf::config_file();
    nmap["missionconfigfile"] = sqf::mission_config_file();

    umap["localize"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        auto newVal = sqf::localize(lastVal);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(newVal));
        return true;
    };

    umap["getnumber"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        if (lastVal.type_enum() != types::game_data_type::CONFIG) return false;

        auto newVal = sqf::get_number(lastVal);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(newVal));
        return true;
    };

    umap["gettext"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        if (lastVal.type_enum() != types::game_data_type::CONFIG) return false;
    
        auto newVal = sqf::get_text(lastVal);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(newVal));
        return true;
    };

    umap["getarray"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        if (lastVal.type_enum() != types::game_data_type::CONFIG) return false;

        auto newVal = sqf::get_array(lastVal);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(newVal));
        return true;
    };

    umap["isclass"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        if (lastVal.type_enum() != types::game_data_type::CONFIG) return false;

        auto newVal = sqf::is_class(lastVal);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(newVal));
        return true;
    };

    umap["istext"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        if (lastVal.type_enum() != types::game_data_type::CONFIG) return false;

        auto newVal = sqf::is_text(lastVal);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(newVal));
        return true;
    };

    umap["configproperties"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto lastVal = static_cast<GameInstructionConst*>(instructions.back().get())->value;
        if (lastVal.type_enum() != types::game_data_type::ARRAY) return {};
    
        auto cfg = lastVal.get(0);
        if (!cfg) return false;
        auto cond = lastVal.get(1);
        auto inherit = lastVal.get(2);
        //#TODO test
        auto res = sqf::config_properties(
            *cfg,
            cond ? *cond : "true"sv,
            inherit ? *inherit : true
        );
    
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(res));
    
        return true;
    };


    bmap[">>"] = [](auto_array<ref<game_instruction>> & instructions) -> bool {
        auto left = static_cast<GameInstructionConst*>((instructions.end()-2)->get())->value;
        auto right = static_cast<GameInstructionConst*>(instructions.back().get())->value;
    
    
        if (left.type_enum() != types::game_data_type::CONFIG || right.type_enum() != types::game_data_type::STRING)
            return false; //#TODO test
    
        auto newVal = sqf::config_entry(left) >> right;
    
        instructions.erase(instructions.end() - 1);
        instructions.erase(instructions.end() - 1);
        instructions.emplace_back(GameInstructionConst::make(game_value(newVal)));
    
    
        return true;
    };


}


bool BytecodeOptimizer::onNular(std::string_view name, auto_array<ref<game_instruction>>& instructions) const {
    auto found = nmap.find(name);
    if (found != nmap.end()) {
        instructions.emplace_back(GameInstructionConst::make(found->second));
        return true;
    }
    return false;
}

bool BytecodeOptimizer::onUnary(std::string_view name, auto_array<ref<game_instruction>>& instructions) const {
    if (!isInstructionConst(instructions.back())) return false;

    auto found = umap.find(name);
    if (found != umap.end()) {
        return found->second(instructions);
    }
    return false;
}

bool BytecodeOptimizer::onBinary(std::string_view name, auto_array<ref<game_instruction>>& instructions) const {
    if (!isInstructionConst(instructions.back())) return false;
    if (!isInstructionConst(*(instructions.end()-2))) return false;


    auto found = bmap.find(name);
    if (found != bmap.end()) {
        return found->second(instructions);
    }
    return false;
} 


bool BytecodeOptimizer::isInstructionConst(ref<game_instruction> instr) {
        auto typeHash = typeid(*instr.get()).hash_code();
        //std::string typeName = typeid(*instr.get()).name();
        return typeHash == GameInstructionConst::typeIDHash;
}
