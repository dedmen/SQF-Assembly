#pragma once
#include <common/singleton.hpp>

class BytecodeLoader : public intercept::singleton<BytecodeLoader> {
public:
    void preStart();
    bool fileExists(const char* name) const;



private:
    class context {
        virtual bool stuff(const void* bank) const = delete;
    };

    bool (*fileExistsInt)(const char* name, context* ctx);
};