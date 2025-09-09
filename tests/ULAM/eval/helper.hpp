#pragma once
#include "./codegen.hpp"
#include "./env.hpp"

class EvalHelper {
public:
    EvalHelper(EvalEnv& env): _env{env} {}

    bool in_main() const;
    bool codegen_enabled() const;

    Codegen& gen() { return _env.gen(); }

private:
    EvalEnv& _env;
};
