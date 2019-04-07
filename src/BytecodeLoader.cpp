#include "BytecodeLoader.hpp"
#include <HookManager.hpp>
#include <intercept.hpp>
#include <filesystem>
#include <base64.h>
#include <sstream>
#define ASC_INTERCEPT
#include <scriptSerializer.hpp>
#include "common.hpp"

static sqf_script_type Compound_string_bytecode_type;

class GameDataBytecode : public game_data {
public:
    GameDataBytecode() = default;
    //GameDataBytecode(std::shared_ptr<scopeData>&& _data) noexcept : data(std::move(_data)) {}
    void lastRefDeleted() const override { delete this; }
    const sqf_script_type& type() const override { return Compound_string_bytecode_type; }
    ~GameDataBytecode() override = default;
    bool get_as_bool() const override { return true; }
    float get_as_number() const override { return 0.f; }
    const r_string& get_as_string() const override { 
        auto gs = intercept::client::host::functions.get_engine_allocator()->gameState;

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
    //virtual bool equals(const game_data*) const override;
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


game_value buildCodeInstructions(const CompiledCodeData& data, const std::vector<ScriptInstruction>& inst, r_string originalPath, bool final = false) {
    auto gs = intercept::client::host::functions.get_engine_allocator()->gameState;
    auto_array<ref<game_instruction>> instructions;
    instructions.reserve(inst.size());
    auto temp = intercept::sqf::preprocess_file_line_numbers(originalPath); //#TODO seperate compiled code combo type that returns preproced file on to_string
    int line = 0;
    for (auto& it : inst) {
        switch (it.type) {

        case InstructionType::endStatement:
            instructions.emplace_back(GameInstructionNewExpression::make());
            break;
        case InstructionType::push: {
            auto index = std::get<1>(it.content);
            instructions.emplace_back(GameInstructionConst::make(data.builtConstants[index]));
        } break;
        case InstructionType::callUnary: {
            r_string name = std::get<0>(it.content);
            auto fnc = &gs->get_script_functions().get(name);
            instructions.emplace_back(GameInstructionFunction::make(fnc));
        } break;
        case InstructionType::callBinary: {
            r_string name = std::get<0>(it.content);
            auto fnc = &gs->get_script_operators().get(name);
            instructions.emplace_back(GameInstructionOperator::make(fnc));
        } break;
        case InstructionType::callNular: {
            r_string name = std::get<0>(it.content);
            //auto fnc = &gs->get_script_nulars().get(name);
            instructions.emplace_back(GameInstructionVariable::make(name));
        } break;
        case InstructionType::assignTo:
            instructions.emplace_back(GameInstructionAssignment::make(std::get<0>(it.content), false));
            break;
        case InstructionType::assignToLocal:
            instructions.emplace_back(GameInstructionAssignment::make(std::get<0>(it.content), true));
            break;
        case InstructionType::getVariable: {
            r_string name = std::get<0>(it.content);

            instructions.emplace_back(GameInstructionVariable::make(name));
        } break;
        case InstructionType::makeArray:
            instructions.emplace_back(GameInstructionArray::make(std::get<1>(it.content)));
            break;
        default:;
        }
        instructions.back()->sdp.pos = 0;
        instructions.back()->sdp.sourcefile = data.fileNames[it.fileIndex];
        instructions.back()->sdp.content = temp;
        instructions.back()->sdp.sourceline = line++;

    }

    auto newCode = new game_data_code();
    newCode->instructions = std::move(instructions);
    //auto file = data.fileNames[inst.front().fileIndex];
    newCode->code_string = temp;
    newCode->is_final = final;

    return newCode;
}

game_value buildConstant(const CompiledCodeData& data, const ScriptConstant& cnst, r_string originalPath) {
    switch (getConstantType(cnst)) {
        case ConstantType::code: return buildCodeInstructions(data, std::get<0>(cnst), originalPath);
        case ConstantType::string: return std::get<1>(cnst);
        case ConstantType::scalar: return std::get<2>(cnst);
        case ConstantType::boolean:return std::get<3>(cnst);
        default: ;
    }
    __debugbreak();
}


game_value buildCode(CompiledCodeData data, r_string originalPath, bool final = false) {
    data.builtConstants.reserve(data.constants.size());
    for (auto& it : data.constants) {
        data.builtConstants.emplace_back(buildConstant(data, it, originalPath));
    }
    return buildCodeInstructions(data, data.instructions, originalPath, final);
}

game_value fileExists_sqf(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;
    return BytecodeLoader::get().fileExists(name.c_str());
}


std::string toASM(game_state& gamestate, game_data_code* newCode) {
    std::string out;
    for (auto& it : newCode->instructions) {
        //auto& type = typeid(*it.get());
        out += instructionToString(&gamestate, it);
        out += "\n";
    }
    return out;
}

game_value compile(game_state& gamestate, game_value_parameter bytecode) {
    auto bc = bytecode.get_as<GameDataBytecode>();

    auto e = bytecode.type_enum();
    if (e == game_data_type::STRING) return intercept::sqf::compile(bytecode);


    auto inputFile = bc->binPath.string();


    auto encodedBytecode = intercept::sqf::load_file(inputFile);

    auto decoded = base64_decode(encodedBytecode);
    std::istringstream str(decoded);
    auto code = ScriptSerializer::binaryToCompiledCompressed(str);

    auto res = buildCode(std::move(code), bc->originalFilePath);

    auto a1 = intercept::sqf::compile(bytecode);

    auto b1 = a1.get_as<game_data_code>();
    auto b2 = res.get_as<game_data_code>();

    auto c1 = toASM(gamestate, b1);
    auto c2 = toASM(gamestate, b2);






    return res;
}

game_value compileF(game_state& gamestate, game_value_parameter bytecode) {
    auto bc = bytecode.get_as<GameDataBytecode>();

    auto e = bytecode.type_enum();
    if (e == game_data_type::STRING) return intercept::sqf::compile(bytecode);


    auto inputFile = bc->binPath.string();


    auto encodedBytecode = intercept::sqf::load_file(inputFile);

    auto decoded = base64_decode(encodedBytecode);
    std::istringstream str(decoded);
    auto code = ScriptSerializer::binaryToCompiledCompressed(str);

    auto res = buildCode(std::move(code), bc->originalFilePath, true);
    auto a1 = intercept::sqf::compile(bytecode);

    auto b1 = a1.get_as<game_data_code>();
    auto b2 = res.get_as<game_data_code>();

    auto c1 = toASM(gamestate, b1);
    auto c2 = toASM(gamestate, b2);

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

        if (name.find("settings") != std::string::npos) __debugbreak();

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


void BytecodeLoader::preStart() {

    HookManager manager;
    fileExistsInt = (bool (*)(const char*,context*))manager.findPattern(fileExistsPat);

    static auto codeType = intercept::client::host::register_sqf_type("Bytecode"sv, "Bytecode"sv, "SQF Assembly bytecode"sv, "Bytecode"sv, createGameDataBytecode);
    Compound_string_bytecode_type = intercept::client::host::register_compound_sqf_type({ codeType.first, game_data_type::STRING });


    static auto _fileExists = intercept::client::host::register_sqf_command("fileExists", "", fileExists_sqf, game_data_type::BOOL, game_data_type::STRING);
    //static auto _compileB = intercept::client::host::register_sqf_command("compile", "", compile, game_data_type::CODE, codeType.first);
    //static auto _compileFB = intercept::client::host::register_sqf_command("compileFinal", "", compileF, game_data_type::CODE, codeType.first);

    static auto _compile2B = intercept::client::host::register_sqf_command("compile", "", compile, game_data_type::CODE, game_data_type::STRING);
    static auto _compile2FB = intercept::client::host::register_sqf_command("compileFinal", "", compileF, game_data_type::CODE, game_data_type::STRING);
    
    static auto _preprocLNB = intercept::client::host::register_sqf_command("preprocessFileLineNumbers", "", preprocFileLN, codeType.first, game_data_type::STRING);
    static auto _preprocB = intercept::client::host::register_sqf_command("preprocessFile", "", preprocFile, codeType.first, game_data_type::STRING);

}

bool BytecodeLoader::fileExists(const char* name) const {
    if (!fileExistsInt) __debugbreak();
    if (*name == '\\') name++;
    return fileExistsInt(name, nullptr);
}
