#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam::sema {

class EvalEnv;

class EvalHelper : public EvalBase {
public:
    EvalHelper(EvalEnv& env);

    EvalHelper(EvalHelper&&) = delete;
    EvalHelper&& operator=(EvalHelper&&) = delete;

protected:
    EvalEnv& env() { return _env; }

    // convenience
    Scope* scope();
    eval_flags_t has_flag(eval_flags_t flag) const;
    eval_flags_t flags() const;

private:
    EvalEnv& _env;
};

} // namespace ulam::sema
