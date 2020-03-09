#pragma once

#include "common.hpp"
#include <unordered_map>
#include <functional>

class BytecodeOptimizer {
public:
    void init();

    bool onNular(std::string_view name, auto_array<ref<game_instruction>>& instructions) const;
    bool onUnary(std::string_view name, auto_array<ref<game_instruction>>& instructions) const;
    bool onBinary(std::string_view name, auto_array<ref<game_instruction>>& instructions) const;

private:
    std::unordered_map<std::string_view, game_value_static> nmap;
    std::unordered_map<std::string_view, std::function<bool(auto_array<ref<game_instruction>>&)>> umap;
    std::unordered_map<std::string_view, std::function<bool(auto_array<ref<game_instruction>>&)>> bmap;

    static bool isInstructionConst(ref<game_instruction> instr);

};
