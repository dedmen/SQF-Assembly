#include "common.hpp"

asshelper::asshelper() {
    nmap["missionnamespace"] = sqf::mission_namespace();
    nmap["uinamespace"] = sqf::ui_namespace();
    nmap["parsingnamespace"] = sqf::parsing_namespace();
    nmap["profilenamespace"] = sqf::profile_namespace();
    nmap["true"] = true;
    nmap["false"] = false;
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


    umap["sqrt"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::SCALAR) return {};
        return game_value(sqrt(static_cast<float>(right)));
    };

    umap["localize"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::STRING) return {};
        return game_value(sqf::localize(right));
    };

    umap["!"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::BOOL) return {};
        return game_value(!static_cast<bool>(right));
    };

    umap["getnumber"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::CONFIG) return {};
        return game_value(sqf::get_number(right));
    };

    umap["gettext"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::CONFIG) return {};
        return game_value(sqf::get_text(right));
    };

    umap["getarray"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::CONFIG) return {};
        return game_value(sqf::get_array(right));
    };

    umap["isclass"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::CONFIG) return {};
        return game_value(sqf::is_class(right));
    };

    umap["istext"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::CONFIG) return {};
        return game_value(sqf::is_text(right));
    };

    umap["configproperties"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::ARRAY) return {};
        auto cfg = right.get(0);
        if (!cfg) return {};
        auto cond = right.get(0);
        auto inherit = right.get(0);
        //#TODO test
        return game_value(sqf::config_properties(
            *cfg,
            cond ? *cond : "true"sv,
            inherit ? *inherit : true
        ));
    };

    umap["count"] = [](game_value_parameter right) -> std::optional<game_value> {
        if (right.type_enum() != types::GameDataType::ARRAY) return {};
        return game_value(right.size());
    };

    //#TODO typename intercept func
    //#TODO optimize typename comparison into isEqualType
    //umap["typename"] = [](game_value right) -> std::optional<game_value> {
    //    return game_value(sqf::type());
    //};

    //#TODO configFile >> "CfgWeapons" compiletime

    //#TODO check if https://community.bistudio.com/wiki/-_a is properly done compiletime

    bmap["mod"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
        if (left.type_enum() != types::GameDataType::SCALAR || right.type_enum() != types::GameDataType::SCALAR)
            return {};
        return game_value(fmodf(static_cast<float>(left), static_cast<float>(right)));
    };
    bmap["else"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
        if (left.type_enum() != types::GameDataType::CODE || right.type_enum() != types::GameDataType::CODE)
            return {};
        return game_value({left, right});
    };

    bmap[">>"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
        if (left.type_enum() != types::GameDataType::CONFIG || right.type_enum() != types::GameDataType::STRING)
            return {}; //#TODO test
        return game_value(sqf::config_entry(left) >> right);
    };


    //#TODO does this obey * before - ?
    bmap["+"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
        if (left.type_enum() != types::GameDataType::SCALAR || right.type_enum() != types::GameDataType::SCALAR)
            return {}; //#TODO test
        return game_value(static_cast<float>(left) + static_cast<float>(right));
    };

    bmap["-"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
        if (left.type_enum() != types::GameDataType::SCALAR || right.type_enum() != types::GameDataType::SCALAR)
            return {}; //#TODO test
        return game_value(static_cast<float>(left) - static_cast<float>(right));
    };

    bmap["/"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
        if (left.type_enum() != types::GameDataType::SCALAR || right.type_enum() != types::GameDataType::SCALAR)
            return {}; //#TODO test
        return game_value(static_cast<float>(left) / static_cast<float>(right));
    };
	bmap["*"] = [](game_value_parameter left, game_value_parameter right) -> std::optional<game_value> {
		if (left.type_enum() != types::GameDataType::SCALAR || right.type_enum() != types::GameDataType::SCALAR)
			return {}; //#TODO test
		return game_value(static_cast<float>(left) * static_cast<float>(right));
	};
}

bool asshelper::containsNular(std::string_view key) const { return nmap.find(key) != nmap.end(); }
bool asshelper::containsUnary(std::string_view key) const { return umap.find(key) != umap.end(); }
bool asshelper::containsBinary(std::string_view key) const { return bmap.find(key) != bmap.end(); }
game_value asshelper::get(std::string_view key) const { return nmap.at(key); }

std::optional<game_value> asshelper::get(game_state* gs, std::string_view key, ref<game_instruction> right) const {
    if (!isconst(gs, right))
        return {};
    auto rightval = static_cast<GameInstructionConst*>(right.get());

	auto found = umap.find(key);
	if (found == umap.end()) return {};
	return found->second(rightval->value);
}

std::optional<game_value> asshelper::get(game_state* gs, std::string_view key, ref<game_instruction> left,
                                         ref<game_instruction> right) const {
    if (!isconst(gs, left) || !isconst(gs, right))
        return {};
    auto leftval = static_cast<GameInstructionConst*>(left.get());
    auto rightval = static_cast<GameInstructionConst*>(right.get());

	auto found = bmap.find(key);
	if (found == bmap.end()) return {};
    return found->second(leftval->value, rightval->value);
}

asshelper::insttype asshelper::getinsttype(game_state* gs, ref<game_instruction> instr) {
    auto typeHash = typeid(*instr.get()).hash_code();
    //std::string typeName = typeid(*instr.get()).name();

    switch (typeHash) {
        case GameInstructionNewExpression::typeIDHash:
            return insttype::endStatement;
        case GameInstructionConst::typeIDHash:
            return insttype::push;
        case GameInstructionFunction::typeIDHash:
            return insttype::callUnary;
        case GameInstructionOperator::typeIDHash:
            return insttype::callBinary;
        case GameInstructionAssignment::typeIDHash: {
            GameInstructionAssignment* inst = static_cast<GameInstructionAssignment*>(instr.get());
            if (inst->forceLocal) {
                return insttype::assignToLocal;
            } else {
                return insttype::assignTo;
            }
        }
        case GameInstructionVariable::typeIDHash: {
            GameInstructionVariable* inst = static_cast<GameInstructionVariable*>(instr.get());
            auto varname = inst->name;
            if (gs->get_script_nulars().has_key(varname.c_str())) {
                return insttype::callNular;
            } else {
                return insttype::getVariable;
            }
        }
        case GameInstructionArray::typeIDHash:
            return insttype::makeArray;
        default:
            return insttype::NA;
    }
}

bool asshelper::isconst(game_state* gs, ref<game_instruction> instr) const {
    auto type = getinsttype(gs, instr);
    if (type == insttype::push) {
        return true;
    } else if (type == insttype::callNular) {
        auto inst = static_cast<GameInstructionVariable*>(instr.get());
        return containsNular(inst->name.c_str());
    }
    return false;
}

game_value asshelper::getconst(game_state* gs, ref<game_instruction> instr) const {
    auto type = getinsttype(gs, instr);
    if (type == insttype::push) {
        return static_cast<GameInstructionConst*>(instr.get())->value;
    } else if (type == insttype::callNular) {
        auto inst = static_cast<GameInstructionVariable*>(instr.get());
        return get(inst->name.c_str());
    } else {
        throw std::exception();
    }
}

std::vector<ref<game_instruction>> asshelper::optimize(game_state& gs,
                                                       auto_array<ref<game_instruction>>& instructionsInput) {
    std::vector<ref<game_instruction>> instructions;
    instructions.reserve(instructionsInput.size());

    for (auto& it : instructionsInput)
        instructions.emplace_back(std::move(it));

    const auto hasNConstValues = [this, gs = &gs, &instructions](size_t offset, size_t numberOfConsts) {
        if (numberOfConsts > offset) return false;
        for (size_t i = offset - numberOfConsts; i < offset; ++i) {
            if (!isconst(gs, instructions[i])) return false;
        }
        return true;
    };

    for (size_t i = 0; i < instructions.size(); i++) {
        auto& instr = instructions[i];
        //GameInstructionConst::make(array);
        switch (getinsttype(&gs, instr)) {
            case insttype::makeArray: {
                auto inst = static_cast<GameInstructionArray*>(instr.get());
                size_t arrsize = inst->size;
                //In case makeArray has zero size, just transform to a push instruction
                if (arrsize == 0) {
                    instr = GameInstructionConst::make(auto_array<game_value>());
                    break;
                }

                //Not all array elements are constant
                if (!hasNConstValues(i, arrsize)) break;

                //Backtrack - Add elements to array
                auto_array<game_value> arr;
                for (size_t j = i - arrsize; j < i; ++j) {
                    arr.emplace_back(getconst(&gs, instructions[j]));
                }
                instructions.erase(instructions.begin() + i - arrsize, instructions.begin() + i); //#TODO check
                i -= arrsize - 1;
                instructions[i] = GameInstructionConst::make(std::move(arr));
            }
                break;
            case insttype::callUnary: {
                //auto inst = static_cast<GameInstructionFunction*>(instr.get());
                //auto newValue = get(&gs, inst->getFuncName().c_str(), instructions[i - 1]);
                //if (newValue) {
                //    i -= 1;
                //    instructions.erase(instructions.begin() + i);
                //    instructions[i] = GameInstructionConst::make(std::move(*newValue));
                //}
            }
                break;
            case insttype::callBinary: {
                //auto inst = static_cast<GameInstructionFunction*>(instr.get());
                //game_value valueslot;
                //auto newValue = get(&gs, inst->getFuncName().c_str(), instructions[i - 2],
                //                    instructions[i - 1]);
                //if (newValue) {
                //    instructions.erase(instructions.begin() + i - 1);
                //    instructions.erase(instructions.begin() + i - 2);
                //    i -= 2;
                //    instructions[i] = GameInstructionConst::make(std::move(*newValue));
                //}
            }
                break;
            case insttype::push: {
                auto inst = static_cast<GameInstructionConst*>(instr.get());
                if (inst->value.type_enum() == GameDataType::CODE) {

                    //#TODO only if code is non-empty
                    auto compiled = static_cast<game_data_code*>(inst->value.data.get());

                    auto newInstructions = optimize(gs, compiled->instructions);

                    compiled->instructions = auto_array<ref<game_instruction>>(newInstructions.begin(), newInstructions.end());
                    //#TODO test
                }
            }
                break;
        }
    }
    return instructions;
}
