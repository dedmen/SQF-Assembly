#include <intercept.hpp>



namespace intercept::__internal {

    class gsFuncBase {
    public:
        r_string _name;
        void copyPH(const gsFuncBase* other) noexcept {
            securityStuff = other->securityStuff;
            //std::copy(std::begin(other->securityStuff), std::end(other->securityStuff), std::begin(securityStuff));
        }
    private:
        std::array<size_t,
        #if _WIN64 || __X86_64__
            10
        #else
        #ifdef __linux__
            8
        #else
            11
        #endif
        #endif
        > securityStuff{};  //Will scale with x64
                            //size_t securityStuff[11];
    };
    class gsFunction : public gsFuncBase {
        void* placeholder12{ nullptr };//0x30  //jni function
    public:
        r_string _name2;//0x34 this is (tolower name)
        unary_operator * _operator;//0x38
    #ifndef __linux__
        r_string _rightType;//0x3c RString to something
        r_string _description;//0x38
        r_string _example;
        r_string _example2;
        r_string placeholder_11;
        r_string placeholder_12;
        r_string _category{ "intercept"sv }; //0x48
    #endif
                                            //const rv_string* placeholder13;
    };
    class gsOperator : public gsFuncBase {
        void* placeholder12{ nullptr };//0x30  JNI function
    public:
        r_string _name2;//0x34 this is (tolower name)
        int32_t placeholder_10{ 4 }; //0x38 Small int 0-5  priority
        binary_operator * _operator;//0x3c
    #ifndef __linux__
        r_string _leftType;//0x40 Description of left hand side parameter
        r_string _rightType;//0x44 Description of right hand side parameter
        r_string _description;//0x48
        r_string _example;//0x4c
        r_string placeholder_11;//0x60
        r_string _version;//0x64 some version number
        r_string placeholder_12;//0x68
        r_string _category{ "intercept"sv }; //0x6c
    #endif
    };
    class gsNular : public gsFuncBase {
    public:
        r_string _name2;//0x30 this is (tolower name)
        nular_operator * _operator;//0x34
    #ifndef __linux__
        r_string _description;//0x38
        r_string _example;
        r_string _example2;
        r_string _version;//0x44 some version number
        r_string placeholder_10;
        r_string _category; //0x4d
    #endif
        void* placeholder11{ nullptr };//0x50 JNI probably
        const char *get_map_key() const noexcept { return _name2.data(); }
    };



    class game_functions : public auto_array<gsFunction>, public gsFuncBase {
    public:
        game_functions(std::string name) : _name(name.c_str()) {}
        r_string _name;
        game_functions() noexcept {}
        const char *get_map_key() const noexcept { return _name.data(); }
    };

    class game_operators : public auto_array<gsOperator>, public gsFuncBase {
    public:
        game_operators(std::string name) : _name(name.c_str()) {}
        r_string _name;
        int32_t placeholder10{ 4 }; //0x2C Small int 0-5  priority
        game_operators() noexcept {}
        const char *get_map_key() const noexcept { return _name.data(); }
    };
}
using namespace intercept::__internal;

struct instructionVtable {
    void* vtbl[8];

    void init(game_instruction* engine, game_instruction* intercept) {
        auto in = *reinterpret_cast<instructionVtable**>(intercept);
        auto en = *reinterpret_cast<instructionVtable**>(engine);

        vtbl[0] = en->vtbl[-1];
        vtbl[1] = in->vtbl[0];
        vtbl[2] = in->vtbl[1];
        vtbl[3] = in->vtbl[2];
        vtbl[4] = en->vtbl[3];
        vtbl[5] = en->vtbl[4];
        vtbl[6] = en->vtbl[5];
        vtbl[7] = en->vtbl[6];
    }
};


class GameInstructionConst : public game_instruction {
public:
    game_value value;
    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }


    GameInstructionConst(game_value&& val) : value(val) { setVtable(); }
    GameInstructionConst() { setVtable(); }
    GameInstructionConst(std::nullptr_t) {}
    static ref<GameInstructionConst> make(game_value val) {
        return new GameInstructionConst(std::move(val));
        //return rv_allocator<GameInstructionConst>::create_single(std::move(val));
    }
    template<class... _Types>
    static ref<GameInstructionConst> make(_Types&&... _Args) {
        return new GameInstructionConst(std::forward<_Types>(_Args)...);
        //return rv_allocator<GameInstructionConst>::create_single(std::forward<_Types>(_Args)...);
    }
    //virtual void lastRefDeleted() const { rv_allocator<GameInstructionConst>::destroy_deallocate(const_cast<GameInstructionConst*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

};

class GameInstructionVariable : public game_instruction {
public:
    r_string name;

    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }

    GameInstructionVariable() { setVtable(); }
    GameInstructionVariable(r_string gf) : name(gf) { setVtable(); }
    GameInstructionVariable(std::nullptr_t) {}

    template<class... _Types>
    static ref<GameInstructionVariable> make(_Types&&... _Args) {
        return rv_allocator<GameInstructionVariable>::create_single(std::forward<_Types>(_Args)...);
    }
    virtual void lastRefDeleted() const { rv_allocator<GameInstructionVariable>::destroy_deallocate(const_cast<GameInstructionVariable*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

};

class GameInstructionOperator : public game_instruction {
public:
    const game_operators *_operators;

    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }

    template<class... _Types>
    static ref<GameInstructionOperator> make(_Types&&... _Args) {
        return rv_allocator<GameInstructionOperator>::create_single(std::forward<_Types>(_Args)...);
    }
    virtual void lastRefDeleted() const { rv_allocator<GameInstructionOperator>::destroy_deallocate(const_cast<GameInstructionOperator*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

    GameInstructionOperator() { setVtable(); }
    GameInstructionOperator(const game_operators* gf) : _operators(gf) { setVtable(); }
    GameInstructionOperator(std::nullptr_t) {}

    r_string getFuncName() const {
        return _operators->_name;
    }


};

class GameInstructionFunction : public game_instruction {
public:
    const game_functions *_functions;

    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }

    template<class... _Types>
    static ref<GameInstructionFunction> make(_Types&&... _Args) {
        return rv_allocator<GameInstructionFunction>::create_single(std::forward<_Types>(_Args)...);
    }
    virtual void lastRefDeleted() const { rv_allocator<GameInstructionFunction>::destroy_deallocate(const_cast<GameInstructionFunction*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

    GameInstructionFunction() { setVtable(); }
    GameInstructionFunction(const game_functions* gf) : _functions(gf) { setVtable(); }
    GameInstructionFunction(std::nullptr_t) {}

    r_string getFuncName() const {
        return _functions->_name;
    }



};

class GameInstructionArray : public game_instruction {
public:
    int size;

    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }

    template<class... _Types>
    static ref<GameInstructionArray> make(_Types&&... _Args) {
        return rv_allocator<GameInstructionArray>::create_single(std::forward<_Types>(_Args)...);
    }
    virtual void lastRefDeleted() const { rv_allocator<GameInstructionArray>::destroy_deallocate(const_cast<GameInstructionArray*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

    GameInstructionArray() { setVtable(); }
    GameInstructionArray(std::nullptr_t) {}
    GameInstructionArray(int sz) : size(sz) { setVtable(); }
};

class GameInstructionAssignment : public game_instruction {
public:
    r_string name;
    bool forceLocal;

    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }

    template<class... _Types>
    static ref<GameInstructionAssignment> make(_Types&&... _Args) {
        return rv_allocator<GameInstructionAssignment>::create_single(std::forward<_Types>(_Args)...);
    }
    virtual void lastRefDeleted() const { rv_allocator<GameInstructionAssignment>::destroy_deallocate(const_cast<GameInstructionAssignment*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

    GameInstructionAssignment() { setVtable(); }
    GameInstructionAssignment(std::nullptr_t) {}
    GameInstructionAssignment(r_string nm, bool local) : name(nm), forceLocal(local) { setVtable(); }
};

class GameInstructionNewExpression : public game_instruction {
public:
    int beg{0};
    int end{0};

    static inline instructionVtable defVtable;
    void setVtable() {
        auto vtbl = reinterpret_cast<instructionVtable*>(&defVtable.vtbl[1]);
        *reinterpret_cast<instructionVtable**>(this) = vtbl;
    }

    template<class... _Types>
    static ref<GameInstructionNewExpression> make(_Types&&... _Args) {
        return new GameInstructionNewExpression(std::forward<_Types>(_Args)...);
        //return rv_allocator<GameInstructionNewExpression>::create_single(std::forward<_Types>(_Args)...);
    }
    //virtual void lastRefDeleted() const { rv_allocator<GameInstructionNewExpression>::destroy_deallocate(const_cast<GameInstructionNewExpression*>(this)); }
    virtual bool exec(game_state& state, vm_context& t) { return false; }
    virtual int stack_size(void* t) const { return 0; }
    virtual r_string get_name() const { return ""sv; }

    GameInstructionNewExpression() { setVtable(); }
    GameInstructionNewExpression(std::nullptr_t) { }
};


void prepVtables(const ref<game_instruction>& instr) {
    auto typeHash = typeid(*instr.get()).hash_code();
    std::string typeName = typeid(*instr.get()).name();

    switch (typeHash) {
        case 0xc2bb0eeb: { //GameInstructionNewExpression
            ref<GameInstructionNewExpression> mInst = GameInstructionNewExpression::make(nullptr);
            GameInstructionNewExpression::defVtable.init(instr.get(), mInst.get());
        } break;
        case 0x8c0dbf90: { //GameInstructionConst
            ref<GameInstructionConst> mInst = GameInstructionConst::make(nullptr);
            GameInstructionConst::defVtable.init(instr.get(), mInst.get());
        } break;
        case 0x72ff7d2d: { //GameInstructionFunction
            ref<GameInstructionFunction> mInst = GameInstructionFunction::make(nullptr);
            GameInstructionFunction::defVtable.init(instr.get(), mInst.get());
        } break;
        case 0x0ac32571: { //GameInstructionOperator
            ref<GameInstructionOperator> mInst = GameInstructionOperator::make(nullptr);
            GameInstructionOperator::defVtable.init(instr.get(), mInst.get());
        } break;
        case 0xd27a68ec: { //GameInstructionAssignment
            ref<GameInstructionAssignment> mInst = GameInstructionAssignment::make(nullptr);
            GameInstructionAssignment::defVtable.init(instr.get(), mInst.get());
        } break;
        case 0xc04f83b1: { //GameInstructionVariable
            ref<GameInstructionVariable> mInst = GameInstructionVariable::make(nullptr);
            GameInstructionVariable::defVtable.init(instr.get(), mInst.get());
        } break;
        case 0x4b5efb7a: { //GameInstructionArray
            ref<GameInstructionArray> mInst = GameInstructionArray::make(nullptr);
            GameInstructionArray::defVtable.init(instr.get(), mInst.get());
        } break;

        default: __debugbreak();
    }
}



std::string instructionToString(game_state* gs, const ref<game_instruction>& instr) {
    auto typeHash = typeid(*instr.get()).hash_code();
    std::string typeName = typeid(*instr.get()).name();

    switch (typeHash) {
        case 0xc2bb0eeb: { //GameInstructionNewExpression
            return "endStatement;";
        } break;
        case 0x8c0dbf90: { //GameInstructionConst
            GameInstructionConst* inst = static_cast<GameInstructionConst*>(instr.get());
            return std::string("push ") + inst->value.data->type_as_string() + " " + inst->value.data->to_string().c_str() + ";";
        } break;
        case 0x72ff7d2d: { //GameInstructionFunction
            GameInstructionFunction* inst = static_cast<GameInstructionFunction*>(instr.get());
            return std::string("callFunction ") + inst->getFuncName().c_str() + ";";
        } break;
        case 0x0ac32571: { //GameInstructionOperator
            GameInstructionOperator* inst = static_cast<GameInstructionOperator*>(instr.get());
            return std::string("callOperator ") + inst->getFuncName().c_str() + ";";
        } break;
        case 0xd27a68ec: { //GameInstructionAssignment
            GameInstructionAssignment* inst = static_cast<GameInstructionAssignment*>(instr.get());
            if (inst->forceLocal) {
                return std::string("assignToLocal ") + inst->name.c_str() + ";";
            } else {
                return std::string("assignTo ") + inst->name.c_str() + ";";
            }
        } break;
        case 0xc04f83b1: { //GameInstructionVariable
            GameInstructionVariable* inst = static_cast<GameInstructionVariable*>(instr.get());
            auto varname = inst->name;
            if (gs->_scriptNulars.has_key(varname.c_str())) {
                return std::string("callNular ") + varname.c_str() + ";";
            } else {
                return std::string("getVariable ") + varname.c_str() + ";";
            }
        } break;
        case 0x4b5efb7a: { //GameInstructionArray
            GameInstructionArray* inst = static_cast<GameInstructionArray*>(instr.get());
            return std::string("makeArray ") + std::to_string(inst->size) + ";";
        } break;

        default: __debugbreak();
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

ref<GameInstructionConst> parseConst (std::string_view& cnst) {
    //type ...
    auto type = cnst.substr(0, cnst.find_first_of(' '));
    cnst = cnst.substr(type.length() + 1);

    if (type == "string") {
        auto stringLength = cnst.find_first_of(';');
        auto string = cnst.substr(1, stringLength-2);
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
     auto& f = gs->_scriptFunctions.get(name.data());
     if (gs->_scriptFunctions.is_null(f)) return nullptr;
 
     return GameInstructionFunction::make(&f);
    return nullptr;
}

ref<GameInstructionOperator> parseOperator(game_state* gs, std::string_view& cnst) {
    auto type = cnst.substr(0, cnst.find_first_of(';'));
    cnst = cnst.substr(type.length() + 1);


    std::string name(type);
    auto tb = gs->_scriptOperators.get_table_for_key(name.data());
    auto& f = gs->_scriptOperators.get(type.data());
    if (gs->_scriptOperators.is_null(f)) return nullptr;
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


game_value instructionCount(uintptr_t gs, game_value_parameter code) {
    if (code.is_nil()) return 0;
    auto c = (game_data_code*) code.data.get();
    if (!c->instructions) return 0;
    std::string out;
    for (auto& it : *c->instructions) {
        auto& type = typeid(*it.get());
        out += instructionToString(reinterpret_cast<game_state*>(gs), it);
        out += "\n";
    }

    return out;
}

game_value compileAssembly(uintptr_t gs, game_value_parameter code) {
    auto gamestate = (game_state*) gs;
    std::string_view cd = (r_string)code;
    std::vector<ref<game_instruction>> instr;

    while (cd.length() > 2) {
        skipWhitespace(cd);

        auto type = cd.substr(0, cd.find_first_of(' '));
        cd = cd.substr(type.length()+1);

            /*
            return "endStatement";

            return std::string("makeArray ") + std::to_string(inst->size) + ";";

            */


        if (type == "push") {
            auto cnst = parseConst(cd);
            instr.emplace_back(cnst);
        } else if (type == "callFunction") {
            auto cnst = parseFunction(gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "callOperator") {
            auto cnst = parseOperator(gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "getVariable") {
            auto cnst = parseVariable(gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "callNular") {
            auto cnst = parseVariable(gamestate, cd);
            instr.emplace_back(cnst);
        } else if (type == "assignToLocal") {
            auto cnst = parseAssign(gamestate, cd, true);
            instr.emplace_back(cnst);
        } else if (type == "assignTo") {
            auto cnst = parseAssign(gamestate, cd, false);
            instr.emplace_back(cnst);
        } else if (type == "makeArray") {
            auto cnst = parseArray(gamestate, cd, false);
            instr.emplace_back(cnst);
        } else if (type == "endStatement;") {
            auto cnst = GameInstructionNewExpression::make();
            instr.emplace_back(cnst);
        }


        
    }

    auto c = sqf::compile("test");

    auto compiled = static_cast<game_data_code*>(c.data.get());
    compiled->code_string = code;
    compiled->instructions = compact_array<ref<game_instruction>>::create_zero(instr.size());

    auto& arr = *compiled->instructions;

    uint32_t idx = 0;
    for (auto& it : instr) {
        arr.data()[idx++] = it;
    }

    return c;
}










int intercept::api_version() {
    return 1;
}

void intercept::pre_start() {
	
	
	 static auto _instructionCount = intercept::client::host::registerFunction("instructionCount", "", instructionCount, GameDataType::SCALAR, GameDataType::CODE);

    static auto _compileAsm = intercept::client::host::registerFunction("compileASM"sv, ""sv, compileAssembly, GameDataType::CODE, GameDataType::STRING);

    auto code = sqf::compile("_var = [1, player, player setPos[1, 2, 3], getPos player];");
    auto c = (game_data_code*) code.data.get();
    for (auto& it : *c->instructions) {
        prepVtables(it);
    }


}