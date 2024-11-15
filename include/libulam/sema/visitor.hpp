#pragma once
#include "libulam/ast/nodes/module.hpp"
#include "libulam/ast/visitor.hpp"
#include <libulam/ast.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {
class Diag;
class Scope;
class Sema;
} // namespace ulam

namespace ulam::sema {

class RecVisitor : public ast::RecVisitor {
public:
    using ast::RecVisitor::visit;

    RecVisitor(Sema& sema, ast::Ref<ast::Root> ast): _sema{sema}, _ast{ast} {}

    bool visit(ast::Ref<ast::ModuleDef> module_def) override;
    bool visit(ast::Ref<ast::ClassDef> class_def) override;
    bool visit(ast::Ref<ast::ClassDefBody> class_def_body) override;
    // TODO: blocks, loops, ...

protected:
    class InScope {
    public:
        explicit InScope(Sema& sema);
        ~InScope();

    private:
        Sema& _sema;
    };

    InScope in_scope() { return InScope{_sema}; }

    Diag& diag();
    Scope* scope();

    const std::string_view str(str_id_t str_id) {
        return _ast->ctx().str(str_id);
    }

    Sema& _sema;
    ast::Ref<ast::Root> _ast;

    ast::Ref<ast::ModuleDef> _module_def{};
    ast::Ref<ast::ClassDef> _class_def{};
};

} // namespace ulam::sema
