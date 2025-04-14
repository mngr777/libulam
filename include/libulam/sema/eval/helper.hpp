#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam::sema {

class EvalHelper : public EvalBase {
public:
    EvalHelper(
        EvalVisitor& eval,
        Ref<Program> program,
        Ref<Scope> scope,
        eval_flags_t flags):
        EvalBase{program, flags}, _eval{eval}, _scope{scope} {}

protected:
    EvalVisitor& eval() { return _eval; }

    Ref<Scope> scope() { return _scope; }
    Ref<const Scope> scope() const { return _scope; }

private:
    EvalVisitor& _eval;
    Ref<Scope> _scope;
};

} // namespace ulam::sema
