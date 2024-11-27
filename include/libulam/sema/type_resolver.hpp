#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::sema {

class TypeResolver {
public:
    TypeResolver(ast::Ref<ast::Root> ast): _ast{ast} {}

    Ref<Type> resolve(ast::Ref<ast::TypeName> type_name, Scope* scope);

private:
    Ref<Type> resolve_first(ast::Ref<ast::TypeSpec> type_spec, Scope* scope);
    Ref<Type> resolve_rest(ast::Ref<ast::TypeName> type_name, Scope* scope);

    Ref<Program> program() {
        assert(_ast->program());
        return _ast->program();
    }
    Diag& diag() { return program()->diag(); }

    ast::Ref<ast::Root> _ast;
};

} // namespace ulam::sema
