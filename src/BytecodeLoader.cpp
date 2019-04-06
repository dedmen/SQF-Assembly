#include "BytecodeLoader.hpp"
#include <HookManager.hpp>
#include <intercept.hpp>
#include <filesystem>

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
        if (!preprocCache.empty()) return preprocCache;
        preprocCache = intercept::sqf::preprocess_file_line_numbers(originalFilePath);

        return preprocCache;
    }
    game_data* copy() const override { return new GameDataBytecode(*this); } //#TODO is copying scopes even allowed?!
    r_string to_string() const override { return intercept::sqf::preprocess_file_line_numbers(originalFilePath);}
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
};

game_data* createGameDataBytecode(param_archive* ar) {
    //#TODO use armaAlloc
    auto x = new GameDataBytecode();
    if (ar)
        x->serialize(*ar);
    return x;
}



game_value fileExists_sqf(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;
    return BytecodeLoader::get().fileExists(name.c_str());
}

game_value compile(game_state& gamestate, game_value_parameter bytecode) {
    //r_string name = filename;
    return {};
}

game_value preprocFileLN(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;

    std::filesystem::path path(name.c_str());
    auto newPath = path.parent_path() / (path.filename().string() + ".sqfa");
    if (BytecodeLoader::get().fileExists(name.c_str())) {

    }

    return intercept::sqf::preprocess_file_line_numbers(filename);
}

game_value preprocFile(game_state& gamestate, game_value_parameter filename) {
    r_string name = filename;

    std::filesystem::path path(name.c_str());
    auto newPath = path.parent_path() / (path.filename().string() + ".sqfa");
    if (BytecodeLoader::get().fileExists(name.c_str())) {

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
    static auto _compileB = intercept::client::host::register_sqf_command("compile", "", compile, game_data_type::CODE, codeType.first);
    static auto _preprocLNB = intercept::client::host::register_sqf_command("preprocessFileLineNumbers", "", preprocFileLN, codeType.first, game_data_type::STRING);
    static auto _preprocB = intercept::client::host::register_sqf_command("preprocessFile", "", preprocFile, codeType.first, game_data_type::STRING);

}

bool BytecodeLoader::fileExists(const char* name) const {
    if (!fileExistsInt) __debugbreak();

    return fileExistsInt(name, nullptr);
}
