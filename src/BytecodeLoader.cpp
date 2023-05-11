#include "BytecodeLoader.hpp"
#include <HookManager.hpp>
#include <intercept.hpp>
#include <filesystem>
#include <base64.h>
#include <sstream>
#include "common.hpp"

static sqf_script_type* Compound_string_bytecode_type;

class ArmaScriptProfiler_ProfInterface {
public:
    virtual game_value createScope(r_string name);
    //v2
    virtual game_value createScopeCustomThread(r_string name, uint64_t threadID);
    //v3
    virtual void ASM_createScopeInstr(game_state& state, game_data_code* bodyCode);
    virtual game_value compile(game_state& state, game_value_parameter code, bool final);
};

ArmaScriptProfiler_ProfInterface* profiler = nullptr;


class GameDataBytecode : public game_data {
public:
    GameDataBytecode() = default;
    //GameDataBytecode(std::shared_ptr<scopeData>&& _data) noexcept : data(std::move(_data)) {}
    void lastRefDeleted() const override { delete this; }
    const sqf_script_type& type() const override { return *Compound_string_bytecode_type; }
    ~GameDataBytecode() override = default;
    bool get_as_bool() const override { return true; }
    float get_as_number() const override { return 0.f; }
    const r_string& get_as_string() const override { 
        if (!preprocCache.empty()) return preprocCache;
        preprocCache = intercept::sqf::preprocess_file_line_numbers(originalFilePath);

        return preprocCache;
    }
    game_data* copy() const override { return new GameDataBytecode(*this); } //#TODO is copying scopes even allowed?!
    r_string to_string() const override {
        if (!preprocCache.empty()) return preprocCache;
        preprocCache = intercept::sqf::preprocess_file_line_numbers(originalFilePath);

        return preprocCache;
    }
    virtual bool equals(const game_data* equ) const override {
        auto x = (game_data_code*)equ;
        auto y = (game_data_string*)equ;


        return false;
    }
    const char* type_as_string() const override { return "bytecode"; }
    bool is_nil() const override { return false; }
    bool can_serialize() override { return true; }//Setting this to false causes a fail in scheduled and global vars

    serialization_return serialize(param_archive& ar) override {
        game_data::serialize(ar); //#TODO
        __debugbreak();
        return serialization_return::no_error;
    }
    mutable r_string preprocCache;
    r_string originalFilePath;
    std::filesystem::path binPath;
};

game_data* createGameDataBytecode(param_archive* ar) {
    //#TODO use armaAlloc
    auto x = new GameDataBytecode();
    if (ar)
        x->serialize(*ar);
    return x;
}

extern std::string instructionToString(game_state* gs, const ref<game_instruction>& instr);

game_data_code* getNewCode(bool final = false) {
    static game_value_static normalCode{1};
    static game_value_static finalCode{1};

    if (final) {
        if (finalCode.type_enum() != game_data_type::SCALAR)
            return reinterpret_cast<game_data_code*>(finalCode.get_as<game_data_code>()->copy());
        finalCode = sqf::compile_final("");
        return reinterpret_cast<game_data_code*>(finalCode.get_as<game_data_code>()->copy());
    } else {
        if (normalCode.type_enum() != game_data_type::SCALAR)
            return reinterpret_cast<game_data_code*>(normalCode.get_as<game_data_code>()->copy());
        normalCode = sqf::compile("");
        return reinterpret_cast<game_data_code*>(normalCode.get_as<game_data_code>()->copy());
    }
}



game_value BytecodeLoader::buildCodeInstructions(const CompiledCodeData& data, const ScriptCodePiece& inst, r_string originalPath, bool final) const {
    auto gs = intercept::client::host::functions.get_engine_allocator()->gameState;

    auto_array<ref<game_instruction>> instructions;
    instructions.reserve(inst.code.size());
    STRINGTYPE content;
    auto contentStringIndex = std::get<0>(data.constants[data.codeIndex]).contentString;
    STRINGTYPE fileContent = std::get<STRINGTYPE>(data.constants[contentStringIndex]);
    if (inst.contentSplit.isOffset) {
        content = fileContent.substr(inst.contentSplit.offset, inst.contentSplit.length);
    } else {
        content = fileContent;
    }

    int line = 0;
    for (auto& it : inst.code) {
        switch (it.type) {

        case InstructionType::endStatement:
            instructions.emplace_back(GameInstructionNewExpression::make());
            break;
        case InstructionType::push: {
            //#TODO if we push an array, we need to push a callUnary operator `+` to make sure it's copied.
            //Unless next instruction is something like params or forEach
#ifdef ASC_INTERCEPT
            auto index = std::get<1>(it.content);
            instructions.emplace_back(GameInstructionConst::make(data.builtConstants[index])); //#TODO push of type array must be custom instruction that copies array on access
#endif
        } break;
        case InstructionType::callUnary: {
#ifdef ASC_INTERCEPT
            r_string name = std::get<0>(it.content);
#else
            r_string name = r_string(std::get<0>(it.content));
#endif

            if (!optimizer.onUnary(name, instructions)) {
                auto fnc = &gs->get_script_functions().get(name);
                instructions.emplace_back(GameInstructionFunction::make(fnc));
            }
        } break;
        case InstructionType::callBinary: {
#ifdef ASC_INTERCEPT
            r_string name = std::get<0>(it.content);
#else
            r_string name = r_string(std::get<0>(it.content));
#endif

            if (!optimizer.onBinary(name, instructions)) {
                auto& fnc = gs->get_script_operators().get(name);
                instructions.emplace_back(GameInstructionOperator::make(&fnc));
            }
        } break;
        case InstructionType::callNular: {
#ifdef ASC_INTERCEPT
            r_string name = std::get<0>(it.content);
#else
            r_string name = r_string(std::get<0>(it.content));
#endif

            if (!optimizer.onNular(name, instructions)) {
                //auto fnc = &gs->get_script_nulars().get(name);
                instructions.emplace_back(GameInstructionVariable::make(name));
            }
        } break;
        case InstructionType::assignTo: {
#ifdef ASC_INTERCEPT
            r_string name = std::get<0>(it.content);
#else
            r_string name = r_string(std::get<0>(it.content));
#endif

            instructions.emplace_back(GameInstructionAssignment::make(name, false));
        } break;
        case InstructionType::assignToLocal: {
#ifdef ASC_INTERCEPT
            r_string name = std::get<0>(it.content);
#else
            r_string name = r_string(std::get<0>(it.content));
#endif

            instructions.emplace_back(GameInstructionAssignment::make(name, true));
        } break;
        case InstructionType::getVariable: {
#ifdef ASC_INTERCEPT
            r_string name = std::get<0>(it.content);
#else
            r_string name = r_string(std::get<0>(it.content));
#endif

            instructions.emplace_back(GameInstructionVariable::make(name));
        } break;
        case InstructionType::makeArray: {
            instructions.emplace_back(GameInstructionArray::make(std::get<1>(it.content)));
        } break;
        default:;
        }
        instructions.back()->sdp->pos = it.offset;
        instructions.back()->sdp->sourcefile = data.fileNames[it.fileIndex];
        instructions.back()->sdp->content = fileContent;
        instructions.back()->sdp->sourceline = it.line;

    }

    auto newCode = getNewCode(final);
    newCode->instructions = std::move(instructions);
    //auto file = data.fileNames[inst.front().fileIndex];
    newCode->code_string = content;

    return newCode;
}

game_value BytecodeLoader::buildConstant(const CompiledCodeData& data, const ScriptConstant& cnst, r_string originalPath) const {
    switch (getConstantType(cnst)) {
        case ConstantType::code: return buildCodeInstructions(data, std::get<0>(cnst), originalPath);
        case ConstantType::string: return std::get<1>(cnst);
        case ConstantType::scalar: return std::get<2>(cnst);
        case ConstantType::boolean:return std::get<3>(cnst);
        case ConstantType::array: {
            const ScriptConstantArray& arr = std::get<4>(cnst);
            auto_array<game_value> arrg;
            arrg.reserve(arr.content.size());
            for (auto& it : arr.content)
                arrg.emplace_back(buildConstant(data, it, originalPath));
            return arrg;
        }
        default: ;
    }
    __debugbreak();
}


game_value BytecodeLoader::buildCode(CompiledCodeData data, r_string originalPath, bool final) const {
#ifdef ASC_INTERCEPT
    data.builtConstants.reserve(data.constants.size());
    for (auto& it : data.constants) {
        data.builtConstants.emplace_back(buildConstant(data, it, originalPath));
    }
#endif
    return buildCodeInstructions(data, std::get<0>(data.constants[data.codeIndex]), originalPath, final);
}

game_value fileExists_sqf(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;
    return BytecodeLoader::get().fileExists(name.c_str());
}

bool fileExists_raw(r_string filename) {
    return BytecodeLoader::get().fileExists(filename.c_str());
}


std::string toASM(game_state& gamestate, game_data_code* newCode) {
    std::string out;
    for (auto& it : newCode->instructions) {
        //auto& type = typeid(*it.get());
        auto res = instructionToString(&gamestate, it);
        if (res.empty() < 3) continue;
        out += res;
        out += "\n";
    }
    return out;
}

game_value compile(game_state& gamestate, game_value_parameter bytecode) {
    auto bc = bytecode.get_as<GameDataBytecode>();

    auto e = bytecode.type_enum();
    if (e == game_data_type::STRING) {
        if (profiler)
            return profiler->compile(gamestate, bytecode, false);
        return intercept::sqf::compile(bytecode);
    }


    static const r_string scopeName("BC_LoadF"sv);
    game_value profScope;

    if (profiler)
        profScope = profiler->createScope(scopeName);

    auto inputFile = bc->binPath.string();

    auto encodedBytecode = intercept::sqf::load_file(inputFile);

    static const r_string scopeName2("BC_DEC"sv);
    if (profiler)
        profScope = profiler->createScope(scopeName2);


    auto decoded = base64_decode(encodedBytecode);
    auto code = ScriptSerializer::binaryToCompiledCompressed(decoded);

    static const r_string scopeName3("BC_BUILD"sv);
    if (profiler)
        profScope = profiler->createScope(scopeName3);

    auto res = BytecodeLoader::get().buildCode(std::move(code), bc->originalFilePath);
    if (profiler) profiler->ASM_createScopeInstr(gamestate, res.get_as<game_data_code>()); //Let profiler inject it's instrumentation
    //auto a1 = intercept::sqf::compile(bytecode);
    //
    //auto b1 = a1.get_as<game_data_code>();
    //auto b2 = res.get_as<game_data_code>();
    //
    //auto c1 = toASM(gamestate, b1);
    //auto c2 = toASM(gamestate, b2);
    //
    //if (c1.substr(0, strlen("endStatement;\nendStatement;\n")) == "endStatement;\nendStatement;\n"sv)
    //    c1 = c1.substr(strlen("endStatement;\n"));
    //
    //if (inputFile.find("cba") == std::string::npos)
    //    if (c1 != c2) __debugbreak();
    profScope = game_value();

    return res;
}

game_value compileF(game_state& gamestate, game_value_parameter bytecode) {
    auto bc = bytecode.get_as<GameDataBytecode>();

    auto e = bytecode.type_enum();
    if (e == game_data_type::STRING) {
        if (profiler)
            return profiler->compile(gamestate, bytecode, true);
        return intercept::sqf::compile_final(bytecode);
    }

    static const r_string scopeName("BC_LoadF"sv);
    game_value profScope;

    if (profiler)
        profScope = profiler->createScope(scopeName);

    auto inputFile = bc->binPath.string();


    auto encodedBytecode = intercept::sqf::load_file(inputFile);

    static const r_string scopeName2("BC_DEC"sv);
    profScope = game_value();
    if (profiler)
        profScope = profiler->createScope(scopeName2);

    auto decoded = base64_decode(encodedBytecode);
    auto code = ScriptSerializer::binaryToCompiledCompressed(decoded);

    static const r_string scopeName3("BC_BUILD"sv);
    profScope = game_value();
    if (profiler)
        profScope = profiler->createScope(scopeName3);

    auto res = BytecodeLoader::get().buildCode(std::move(code), bc->originalFilePath, true);
    if (profiler) profiler->ASM_createScopeInstr(gamestate, res.get_as<game_data_code>()); //Let profiler inject it's instrumentation
    //auto a1 = intercept::sqf::compile(bytecode);
    //
    //auto b1 = a1.get_as<game_data_code>();
    //auto b2 = res.get_as<game_data_code>();
    //
    //auto c1 = toASM(gamestate, b1);
    //auto c2 = toASM(gamestate, b2);
    //
    //if (c1.substr(0, strlen("endStatement;\nendStatement;\n")) == "endStatement;\nendStatement;\n"sv)
    //    c1 = c1.substr(strlen("endStatement;\n"));
    //if (inputFile.find("cba") == std::string::npos)
    //    if (c1 != c2) __debugbreak();
    profScope = game_value();
    return res;
}

game_value preprocFileLN(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;

    std::filesystem::path path(name.c_str());
    auto newPath = path.parent_path() / (path.stem().string() + ".sqfc");
    if (BytecodeLoader::get().fileExists(newPath.string().c_str())) {
        auto newT = new GameDataBytecode();
        newT->originalFilePath = name;
        newT->binPath = newPath;
        return game_value(newT);
    }
    return intercept::sqf::preprocess_file_line_numbers(filename);
}

game_value preprocFile(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;

    std::filesystem::path path(name.c_str());
    auto newPath = path.parent_path() / (path.stem().string() + ".sqfc");
    if (BytecodeLoader::get().fileExists(newPath.string().c_str())) {
        auto newT = new GameDataBytecode();
        newT->originalFilePath = name;
        newT->binPath = newPath;
        return game_value(newT);
    }

    return intercept::sqf::preprocess_file(filename);
}


HookManager::Pattern fileExistsPat{
    "xxxx?xxxx?xxxxxxx?????xxxxxxxx????x????xxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx??xxxxxxxxxxxxxxxx????xxxxxx?xxxx?xxxxxx"sv,
    "\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x80\x3D\x00\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xF1\x0F\x84\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xD8\x48\x85\xC0\x0F\x84\x00\x00\x00\x00\x48\x85\xFF\x74\x4B\x4C\x8B\x07\x48\x8B\xD0\x48\x8B\xCF\x41\xFF\x10\x84\xC0\x75\x3B\x48\x8B\x4B\x40\x48\x85\xC9\x74\x10\x48\x83\xC8\xFF\x48\xFF\xC0\x80\x7C\x01\x00\x00\x75\xF6\xEB\x02\x33\xC0\x48\x63\xD0\x48\x8B\xCB\x48\x03\xD6\xE8\x00\x00\x00\x00\x32\xC0\x48\x8B\x5C\x24\x00\x48\x8B\x74\x24\x00\x48\x83\xC4\x20\x5F\xC3"sv
};


extern std::string get_command_line();

std::optional<std::string> getCommandLineParam(std::string_view needle) {
    std::string commandLine = get_command_line();
    const auto found = commandLine.find(needle);
    if (found != std::string::npos) {
        const auto spacePos = commandLine.find(' ', found + needle.length() + 1);
        const auto valueLength = spacePos - (found + needle.length() + 1);
        auto adapterStr = commandLine.substr(found + needle.length() + 1, valueLength);
        if (adapterStr.back() == '"')
            adapterStr = adapterStr.substr(0, adapterStr.length() - 1);
        return adapterStr;
    }
    return {};
}


void BytecodeLoader::preStart() {

    auto iface = client::host::request_plugin_interface("ArmaScriptProfilerProfIFace", 3);
    if (iface)
        profiler = static_cast<ArmaScriptProfiler_ProfInterface*>(*iface);

    optimizer.init();

    HookManager manager;
    fileExistsInt = (bool (*)(const char*,context*))manager.findPattern(fileExistsPat);

    static auto codeType = intercept::client::host::register_sqf_type("Bytecode"sv, "Bytecode"sv, "SQF Assembly bytecode"sv, "Bytecode"sv, createGameDataBytecode);
    Compound_string_bytecode_type = intercept::client::host::register_compound_sqf_type({ codeType.first, game_data_type::STRING });


    //static auto _fileExists = intercept::client::host::register_sqf_command("fileExists", "", fileExists_sqf, game_data_type::BOOL, game_data_type::STRING);
    //static auto _compileB = intercept::client::host::register_sqf_command("compile", "", compile, game_data_type::CODE, codeType.first);
    //static auto _compileFB = intercept::client::host::register_sqf_command("compileFinal", "", compileF, game_data_type::CODE, codeType.first);

    //if (!getCommandLineParam("-sqfasm-no-sqfc")) {
    //    static auto _compile2B = intercept::client::host::register_sqf_command("compile", "", compile, game_data_type::CODE, game_data_type::STRING);
    //    static auto _compile2FB = intercept::client::host::register_sqf_command("compileFinal", "", compileF, game_data_type::CODE, game_data_type::STRING);

    //    static auto _preprocLNB = intercept::client::host::register_sqf_command("preprocessFileLineNumbers", "", preprocFileLN, codeType.first, game_data_type::STRING);
    //    static auto _preprocB = intercept::client::host::register_sqf_command("preprocessFile", "", preprocFile, codeType.first, game_data_type::STRING);
    //}
}

void BytecodeLoader::registerInterfaces() {
    //Tell arma script profiler that it should not override compile
    //if (!getCommandLineParam("-sqfasm-no-sqfc"))
    //    client::host::register_plugin_interface("ProfilerNoCompile"sv, 1, reinterpret_cast<void*>(1));
    //client::host::register_plugin_interface("sqfasm_fileExists"sv, 1, reinterpret_cast<void*>(fileExists_raw));
}

bool BytecodeLoader::fileExists(const char* name) const {
    if (!fileExistsInt) return false;
    if (*name == '\\') name++;
    return fileExistsInt(name, nullptr);
}
