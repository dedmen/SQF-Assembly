#include "common.hpp"

namespace intercept::assembly {

    asshelper::asshelper(game_state* gs) {
        nmap["missionnamespace"] = sqf::mission_namespace();
        nmap["uinamespace"] = sqf::ui_namespace();
        nmap["parsingnamespace"] = sqf::parsing_namespace();
        nmap["profilenamespace"] = sqf::profile_namespace();

        umap["sqrt"] = [](game_value right) -> std::optional<game_value> {
            if (right.type_enum() != types::GameDataType::SCALAR) return {};
            return game_value(sqrt(static_cast<float>(right)));
        };
        bmap["mod"] = [](game_value left, game_value right) -> std::optional<game_value> {
            if (left.type_enum() != types::GameDataType::SCALAR || right.type_enum() != types::GameDataType::SCALAR)
                return {};
            return game_value(fmodf(static_cast<float>(left), static_cast<float>(right)));
        };
    }

    bool asshelper::containsNular(std::string_view key) const { return nmap.find(key) != nmap.end(); }
    bool asshelper::containsUnary(std::string_view key) const { return umap.find(key) != umap.end(); }
    bool asshelper::containsBinary(std::string_view key) const { return bmap.find(key) != bmap.end(); }
    bool isconst(game_state* gs, const asshelper* nh, ref<game_instruction> instr);
    game_value asshelper::get(std::string_view key) const { return nmap.at(key); }

    std::optional<game_value> asshelper::get(game_state* gs, std::string_view key, ref<game_instruction> right) const {
        if (!isconst(gs, this, right))
            return {};
        auto rightval = static_cast<GameInstructionConst*>(right.get());
        return umap.at(key)(rightval->value);
    }

    std::optional<game_value> asshelper::get(game_state* gs, std::string_view key, ref<game_instruction> left,
                                             ref<game_instruction> right) const {
        if (!isconst(gs, this, left) || !isconst(gs, this, right))
            return {};
        auto leftval = static_cast<GameInstructionConst*>(left.get());
        auto rightval = static_cast<GameInstructionConst*>(right.get());
        return bmap.at(key)(leftval->value, rightval->value);
    }

    enum class insttype {
        NA = -1,
        endStatement,
        push,
        callUnary,
        callBinary,
        assignToLocal,
        assignTo,
        callNular,
        getVariable,
        makeArray
    };

    insttype getinsttype(game_state* gs, ref<game_instruction> instr) {
        auto typeHash = typeid(*instr.get()).hash_code();
        std::string typeName = typeid(*instr.get()).name();

        switch (typeHash) {
            case GameInstructionNewExpression::typeIDHash:
                return insttype::endStatement;
            case GameInstructionConst::typeIDHash:
                return insttype::push;
            case GameInstructionFunction::typeIDHash: {
                return insttype::callUnary;
            }
                break;
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
                break;
            case GameInstructionVariable::typeIDHash: {
                GameInstructionVariable* inst = static_cast<GameInstructionVariable*>(instr.get());
                auto varname = inst->name;
                if (gs->_scriptNulars.has_key(varname.c_str())) {
                    return insttype::callNular;
                } else {
                    return insttype::getVariable;
                }
            }
                break;
            case GameInstructionArray::typeIDHash:
                return insttype::makeArray;
            default:
                return insttype::NA;
        }
    }

    bool isconst(game_state* gs, const asshelper* nh, ref<game_instruction> instr) {
        auto type = getinsttype(gs, instr);
        if (type == insttype::push) {
            return true;
        } else if (type == insttype::callNular) {
            auto inst = static_cast<GameInstructionVariable*>(instr.get());
            return nh->containsNular(inst->name.c_str());
        }
        return false;
    }

    game_value getconst(game_state* gs, asshelper* nh, ref<game_instruction> instr) {
        auto type = getinsttype(gs, instr);
        if (type == insttype::push) {
            return static_cast<GameInstructionConst*>(instr.get())->value;
        } else if (type == insttype::callNular) {
            auto inst = static_cast<GameInstructionVariable*>(instr.get());
            return nh->get(inst->name.c_str());
        } else {
            throw std::exception();
        }
    }

    void optimize(game_state* gs, asshelper* nh, ref<compact_array<ref<game_instruction>>> instructions) {
        size_t count = instructions->size();
        int died = 0;
        for (int i = 0; i < count; i++) {
            auto instr = instructions->get(i);
            //GameInstructionConst::make(array);
            switch (getinsttype(gs, instr)) {
                case insttype::makeArray: {
                    auto inst = static_cast<GameInstructionArray*>(instr.get());
                    size_t arrsize = inst->size;
                    //In case makeArray has zero size, just transform to a push instruction
                    if (arrsize == 0) {
                        instructions->data()[i] = GameInstructionConst::make(auto_array<game_value>());
                        break;
                    }
                    bool abortflag = false;
                    //Backtrack - Check if non-constant values are existing
                    for (int j = i - arrsize - died; j < i - died; j++) {
                        if (!isconst(gs, nh, instructions->get(j))) {
                            abortflag = true;
                            break;
                        }
                    }
                    //If abortflag was set, abort conversion
                    if (abortflag) {
                        break;
                    }
                    //Backtrack - Add elements to array
                    auto_array<game_value> arr;
                    for (int j = i - arrsize - died; j < i - died; j++) {
                        arr.push_back(getconst(gs, nh, instructions->get(j)));
                    }
                    died += arrsize;
                    instructions->data()[i] = GameInstructionConst::make(std::move(arr));
                }
                    break;
                case insttype::callUnary: {
                    auto inst = static_cast<GameInstructionFunction*>(instr.get());
                    auto newValue = nh->get(gs, inst->getFuncName().c_str(), instructions->get(i - died - 1));
                    if (newValue) {
                        died += 1;
                        instructions->data()[i] = GameInstructionConst::make(std::move(*newValue));
                    }
                }
                    break;
                case insttype::callBinary: {
                    auto inst = static_cast<GameInstructionFunction*>(instr.get());
                    game_value valueslot;
                    auto newValue = nh->get(gs, inst->getFuncName().c_str(), instructions->get(i - died - 2),
                                            instructions->get(i - died - 1));
                    if (newValue) {
                        died += 2;
                        instructions->data()[i] = GameInstructionConst::make(std::move(*newValue));
                    }
                }
                    break;
            }
            if (died > 0) {
                instructions->data()[i - died] = instructions->data()[i];
            }
        }
        for (auto i = instructions->begin() + (count - died); i < instructions->end(); i++) {
            i->free();
        }
        instructions->_size -= died;
    }
}
