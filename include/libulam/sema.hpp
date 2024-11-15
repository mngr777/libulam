#pragma once
#include <libulam/ast.hpp>
#include <libulam/semantic/scope.hpp>
#include <stack>

namespace ulam {

namespace sema {
class Visitor;
}

class Diag;

class Sema {
    friend sema::Visitor;

public:
    explicit Sema(Diag& diag): _diag{diag} {}

    void analyze(ast::Ref<ast::Root> ast);

private:
    void reset();
    void enter_scope();
    void exit_scope();

    Scope& scope();

    Diag& _diag;
    std::stack<Scope> _scopes;
};

} // namespace ulam
