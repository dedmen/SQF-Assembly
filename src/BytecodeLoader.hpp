#pragma once
#include <common/singleton.hpp>
#define ASC_INTERCEPT
#include <scriptSerializer.hpp>
#include "BytecodeOptimizer.hpp"

class BytecodeLoader : public intercept::singleton<BytecodeLoader> {
public:
    void preStart();
    void registerInterfaces();
    bool fileExists(const char* name) const;


    game_value buildCode(CompiledCodeData data, r_string originalPath, bool final = false) const;
    game_value buildConstant(const CompiledCodeData& data, const ScriptConstant& cnst, r_string originalPath) const;
    game_value buildCodeInstructions(const CompiledCodeData& data, const ScriptCodePiece& inst, r_string originalPath, bool final = false) const;

private:
    BytecodeOptimizer optimizer;
    class context {
        virtual bool stuff(const void* bank) const = delete;
    };

    bool (*fileExistsInt)(const char* name, context* ctx);
};