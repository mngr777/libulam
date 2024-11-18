#pragma once
#include <libulam/ast.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam {

namespace sema {
class RecVisitor;
}

class Diag;

class Sema {
    friend sema::RecVisitor;

public:
    explicit Sema(Diag& diag);
    ~Sema();

    void analyze(ast::Ref<ast::Root> ast);

private:
    Diag& _diag;
};

} // namespace ulam
