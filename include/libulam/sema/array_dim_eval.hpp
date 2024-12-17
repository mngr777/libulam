#pragma once
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {
class Program;
}

namespace ulam::ast {
class Expr;
}

namespace ulam::sema {

class ArrayDimEval {
public:
    explicit ArrayDimEval(Ref<Program> program, Ref<Scope> scope):
        _program{program}, _scope{scope} {}

    std::pair<array_size_t, bool> eval(Ref<ast::Expr> expr);

private:
    Ref<Program> _program;
    Ref<Scope> _scope;
};

} // namespace ulam::sema
