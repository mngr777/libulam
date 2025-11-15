#pragma once
#include "./codegen.hpp"
#include "./env.hpp"
#include "./test_context.hpp"
#include <libulam/semantic/value.hpp>

class EvalHelper {
public:
    EvalHelper(EvalEnv& env): _env{env} {}
    virtual ~EvalHelper() {}

    bool in_main() const;
    bool codegen_enabled() const;

    Codegen& gen() { return _env.gen(); }

    EvalEnv::EvalTestContextRaii test_ctx_raii(ulam::LValue active_atom) {
        return _env.test_ctx_raii(active_atom);
    }

    EvalTestContext& test_ctx() { return _env.test_ctx(); }

    ulam::sema::ExprRes call_native(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::LValue self,
        ulam::sema::ExprResList&& args);

private:
    EvalEnv& _env;
};
