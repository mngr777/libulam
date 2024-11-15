#pragma once
#include <libulam/ast.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <stack>
#include <utility>

namespace ulam {

namespace sema {
class RecVisitor;
}

class Diag;
class Program;

class Sema {
    friend sema::RecVisitor;

public:
    explicit Sema(Diag& diag);
    ~Sema();

    void analyze(ast::Ref<ast::Root> ast);

    Ptr<Program> move_program();

private:
    void reset();
    void enter_scope(Ref<Scope> scope);
    void enter_scope(Scope::Flag flags = Scope::NoFlags);
    void exit_scope();

    Diag& diag() { return _diag; }
    Ref<Program> program() { return ref(_program); }
    Ref<Scope> scope();

    Diag& _diag;
    Ptr<Program> _program;
    std::stack<std::pair<Ref<Scope>, Ptr<Scope>>> _scopes;
};

} // namespace ulam
