#include "common.hpp"
#include "BytecodeLoader.hpp"


void prepVtables(const ref<game_instruction>& instr) {
    auto typeHash = typeid(*instr.get()).hash_code();
    std::string typeName = typeid(*instr.get()).name();

    switch (typeHash) {
        case GameInstructionNewExpression::typeIDHash: { //GameInstructionNewExpression
            ref<GameInstructionNewExpression> mInst = GameInstructionNewExpression::make(nullptr);
            GameInstructionNewExpression::defVtable.init(instr.get(), mInst.get());
			GameInstructionNewExpression::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionConst::typeIDHash: { //GameInstructionConst
            ref<GameInstructionConst> mInst = GameInstructionConst::make(nullptr);
            GameInstructionConst::defVtable.init(instr.get(), mInst.get());
			GameInstructionConst::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionFunction::typeIDHash: { //GameInstructionFunction
            ref<GameInstructionFunction> mInst = GameInstructionFunction::make(nullptr);
            GameInstructionFunction::defVtable.init(instr.get(), mInst.get());
			GameInstructionFunction::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionOperator::typeIDHash: { //GameInstructionOperator
            ref<GameInstructionOperator> mInst = GameInstructionOperator::make(nullptr);
            GameInstructionOperator::defVtable.init(instr.get(), mInst.get());
			GameInstructionOperator::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionAssignment::typeIDHash: { //GameInstructionAssignment
            ref<GameInstructionAssignment> mInst = GameInstructionAssignment::make(nullptr);
            GameInstructionAssignment::defVtable.init(instr.get(), mInst.get());
			GameInstructionAssignment::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionVariable::typeIDHash: { //GameInstructionVariable
            ref<GameInstructionVariable> mInst = GameInstructionVariable::make(nullptr);
            GameInstructionVariable::defVtable.init(instr.get(), mInst.get());
			GameInstructionVariable::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionArray::typeIDHash: { //GameInstructionArray
            ref<GameInstructionArray> mInst = GameInstructionArray::make(nullptr);
            GameInstructionArray::defVtable.init(instr.get(), mInst.get());
			GameInstructionArray::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;
        case GameInstructionNular::typeIDHash: { //GameInstructionNular
            ref<GameInstructionNular> mInst = GameInstructionNular::make(nullptr);
            GameInstructionNular::defVtable.init(instr.get(), mInst.get());
            GameInstructionNular::vtablePtr = *reinterpret_cast<instructionVtable**>(instr.get());
        } break;

        //default: if (IsDebuggerPresent()) __debugbreak();
    }
}



std::string instructionToString(game_state* gs, const ref<game_instruction>& instr) {
    auto typeHash = typeid(*instr.get()).hash_code();
    std::string typeName = typeid(*instr.get()).name();
//#define ITS_LINEEND  ";(" + std::to_string(inst->sdp.sourceline)
#define ITS_LINEEND  ";"
    switch (typeHash) {
        case GameInstructionNewExpression::typeIDHash: { //GameInstructionNewExpression
            //return "endStatement;";
            return "";
        } break;
        case GameInstructionConst::typeIDHash: { //GameInstructionConst
            GameInstructionConst* inst = static_cast<GameInstructionConst*>(instr.get());
            auto ret = std::string("push ") + inst->value.data->type_as_string();

            if (inst->value.data->type_as_string()=="code"sv) {
                ret += " {\n";
                
                for (auto& it : inst->value.get_as<game_data_code>()->instructions) {
                    auto res = instructionToString(gs, it);
                    if (!res.empty())
                        ret += res + "\n";
                }
                    
                ret += std::string("}") + ITS_LINEEND;
            } else if (inst->value.data->type_as_string() == "string"sv) {
                ret += " '';";
            } else if (inst->value.data->type_as_string() == "float"sv) {
                ret += " 0;";
            }
            else {
                ret += std::string(" ") + inst->value.data->to_string().c_str() + ITS_LINEEND;
            }

            return ret;
        } break;
        case GameInstructionFunction::typeIDHash: { //GameInstructionFunction
            GameInstructionFunction* inst = static_cast<GameInstructionFunction*>(instr.get());
            return std::string("callFunction ") + inst->getFuncName().c_str() + ITS_LINEEND;
        } break;
        case GameInstructionOperator::typeIDHash: { //GameInstructionOperator
            GameInstructionOperator* inst = static_cast<GameInstructionOperator*>(instr.get());
            return std::string("callOperator ") + inst->getFuncName().c_str() + ITS_LINEEND;
        } break;
        case GameInstructionAssignment::typeIDHash: { //GameInstructionAssignment
            GameInstructionAssignment* inst = static_cast<GameInstructionAssignment*>(instr.get());
            if (inst->forceLocal) {
                return std::string("assignToLocal ") + inst->name.c_str() + ITS_LINEEND;
            } else {
                return std::string("assignTo ") + inst->name.c_str() + ITS_LINEEND;
            }
        } break;
        case GameInstructionVariable::typeIDHash: { //GameInstructionVariable
            GameInstructionVariable* inst = static_cast<GameInstructionVariable*>(instr.get());
            auto varname = inst->name;
            if (gs->get_script_nulars().has_key(varname.c_str())) {
                return std::string("callNular ") + varname.c_str() + ITS_LINEEND;
            } else {
                return std::string("getVariable ") + varname.c_str() + ITS_LINEEND;
            }
        } break;
        case GameInstructionArray::typeIDHash: { //GameInstructionArray
            GameInstructionArray* inst = static_cast<GameInstructionArray*>(instr.get());
            return std::string("makeArray ") + std::to_string(inst->size) + ITS_LINEEND;
        } break;

        default: {
            return std::string("unknown ") + typeName;
        }
    }
    return "";
}

uint32_t matchBrackets(char bracketOpenType, char bracketCloseType, std::string_view str) {
    if (str[0] != bracketOpenType) return 0;
    uint32_t bracket = 0;
    uint32_t offset = 0;
    for (auto& ch : str) {
        if (ch == bracketOpenType) bracket++;
        if (ch == bracketCloseType) bracket--;
        offset++;
        if (bracket == 0) return offset;
    }
    return offset;
}

ref<GameInstructionConst> parseConst(std::string_view& cnst) {
    //type ...
    auto type = cnst.substr(0, cnst.find_first_of(' '));
    cnst = cnst.substr(type.length() + 1);

    if (type == "string") {
        auto stringLength = cnst.find_first_of(';');
        auto string = cnst.substr(1, stringLength - 2);
        cnst = cnst.substr(stringLength + 1);
        return GameInstructionConst::make(string);
    } else if (type == "code") {
        auto codeLength = matchBrackets('{', '}', cnst);
        auto code = sqf::compile(cnst.substr(0, codeLength));
        cnst = cnst.substr(codeLength + 2);
        return GameInstructionConst::make(std::move(code));
    } else if (type == "float") {
        auto numberLength = cnst.find_first_of(';');
        float num = std::stof(std::string(cnst.substr(0, numberLength)));
        cnst = cnst.substr(numberLength + 1);
        return GameInstructionConst::make(num);
    } else if (type == "array") {
        auto arrayLength = matchBrackets('[', ']', cnst);
        auto arrayText = cnst.substr(0, arrayLength);
        cnst = cnst.substr(arrayLength + 1);
        auto val = sqf::call(sqf::compile(arrayText));
        return GameInstructionConst::make(val);
    }
    __debugbreak();
    return nullptr;
}

ref<GameInstructionFunction> parseFunction(game_state* gs, std::string_view& cnst) {
    auto type = cnst.substr(0, cnst.find_first_of(';'));
    cnst = cnst.substr(type.length() + 1);

    std::string name(type);
    auto& f = gs->get_script_functions().get(name.data());
    if (gs->get_script_functions().is_null(f)) return nullptr;

    return GameInstructionFunction::make(&f);
}

ref<GameInstructionOperator> parseOperator(game_state* gs, std::string_view& cnst) {
    auto type = cnst.substr(0, cnst.find_first_of(';'));
    cnst = cnst.substr(type.length() + 1);

    std::string name(type);
    auto tb = gs->get_script_operators().get_table_for_key(name.data());
    auto& f = gs->get_script_operators().get(type.data());
    if (gs->get_script_operators().is_null(f)) return nullptr;
    return nullptr;

}

ref<GameInstructionVariable> parseVariable(game_state* gs, std::string_view& cnst) {
    auto type = cnst.substr(0, cnst.find_first_of(';'));
    cnst = cnst.substr(type.length() + 1);

    return GameInstructionVariable::make(type);
}

ref<GameInstructionAssignment> parseAssign(game_state* gs, std::string_view& cnst, bool local) {
    auto type = cnst.substr(0, cnst.find_first_of(';'));
    cnst = cnst.substr(type.length() + 1);

    return GameInstructionAssignment::make(type, local);
}

ref<GameInstructionArray> parseArray(game_state* gs, std::string_view& cnst, bool local) {
    auto type = cnst.substr(0, cnst.find_first_of(';'));
    cnst = cnst.substr(type.length() + 1);

    return GameInstructionArray::make(atoi(type.data()));
}

#include <cctype>
void skipWhitespace(std::string_view& str) {
    while (std::isspace(str[0]) && str.length() > 2)
        str = str.substr(1);
}

game_value decompileAssembly(game_state& gs, game_value_parameter code) {
    if (code.is_nil()) return 0;
    auto c = (game_data_code*) code.data.get();
    if (c->instructions.empty()) return 0;
    std::string out;
    for (auto& it : c->instructions) {
        //auto& type = typeid(*it.get());
        out += instructionToString(&gs, it);
        out += "\n";
    }

    return out;
}

game_value compileAssembly(game_state& gamestate, game_value_parameter code) {
    std::string_view cd = (r_string) code;
    std::vector<ref<game_instruction>> instr;

    while (cd.length() > 2) {
        skipWhitespace(cd);

        auto type = cd.substr(0, cd.find_first_of(' '));
        cd = cd.substr(type.length() + 1);

        if (type == "push") {
            auto cnst = parseConst(cd);
            instr.emplace_back(cnst);
        } else if (type == "callFunction") {
            auto cnst = parseFunction(&gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "callOperator") {
            auto cnst = parseOperator(&gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "getVariable") {
            auto cnst = parseVariable(&gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "callNular") {
            auto cnst = parseVariable(&gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "assignToLocal") {
            auto cnst = parseAssign(&gamestate, cd, true);
            instr.emplace_back(cnst);
        } else if (type == "assignTo") {
            auto cnst = parseAssign(&gamestate, cd, false);
            instr.emplace_back(cnst);
        } else if (type == "makeArray") {
            auto cnst = parseArray(&gamestate, cd, false);
            instr.emplace_back(cnst);
        } else if (type == "endStatement;") {
            auto cnst = GameInstructionNewExpression::make();
            instr.emplace_back(cnst);
        }



    }

    auto c = sqf::compile("test");

    auto compiled = static_cast<game_data_code*>(c.data.get());
    compiled->code_string = code;
    compiled->instructions.resize(instr.size());

    auto& arr = compiled->instructions;

    uint32_t idx = 0;
    for (auto& it : instr) {
        arr.data()[idx++] = it;
    }

    return c;
}




game_value optimizeCode(game_state& gamestate, game_value_parameter code) {
    //Broke AF
    //auto origCode = static_cast<game_data_code*>(code.data.get());
    //
    //
    //auto c = sqf::compile("test");
    //auto compiled = static_cast<game_data_code*>(c.data.get());
    //compiled->code_string = origCode->code_string;
    //
    ////ToDo: move into a gen-one-time static variable
    //asshelper nh;
    //auto newInstructions = nh.optimize(gamestate, origCode->instructions);
    //
    //
    //
    //compiled->instructions = auto_array<ref<game_instruction>>(newInstructions.begin(), newInstructions.end());
    return code;
}

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

static struct vtables {
	void* vt_GameInstructionNewExpression;
	void* vt_GameInstructionConst;
	void* vt_GameInstructionFunction;
	void* vt_GameInstructionOperator;
	void* vt_GameInstructionAssignment;
	void* vt_GameInstructionVariable;
	void* vt_GameInstructionArray;
	void* vt_GameInstructionNular;
} vtGlobal;


void intercept::register_interfaces() {
	
	//That should really be done in preStart. But we need it done before people access the interface
	auto code = sqf::compile("private _var = [1, player, player setPos[1, 2, 3], getPos player]; _var");
	auto c = (game_data_code*) code.data.get();
	for (auto& it : c->instructions) {
		prepVtables(it);
	}

	vtGlobal.vt_GameInstructionNewExpression = GameInstructionNewExpression::vtablePtr;
	vtGlobal.vt_GameInstructionConst = GameInstructionConst::vtablePtr;
	vtGlobal.vt_GameInstructionFunction = GameInstructionFunction::vtablePtr;
	vtGlobal.vt_GameInstructionOperator = GameInstructionOperator::vtablePtr;
	vtGlobal.vt_GameInstructionAssignment = GameInstructionAssignment::vtablePtr;
	vtGlobal.vt_GameInstructionVariable = GameInstructionVariable::vtablePtr;
	vtGlobal.vt_GameInstructionArray = GameInstructionArray::vtablePtr;
	vtGlobal.vt_GameInstructionNular = GameInstructionNular::vtablePtr;

	client::host::register_plugin_interface("sqf_asm_devIf", 1, &vtGlobal);
    BytecodeLoader::get().registerInterfaces();

}

void intercept::pre_start() {
    BytecodeLoader::get().preStart();

    static auto _decompileAsm = intercept::client::host::register_sqf_command("decompileASM", "", decompileAssembly, game_data_type::SCALAR, game_data_type::CODE);

    static auto _compileAsm = intercept::client::host::register_sqf_command("compileASM"sv, ""sv, compileAssembly, game_data_type::CODE, game_data_type::STRING);
    static auto _optimizeCode = intercept::client::host::register_sqf_command("optimize"sv, ""sv, optimizeCode, game_data_type::CODE, game_data_type::CODE);

    


}


