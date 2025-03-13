#pragma once
#include "libulam/semantic/scope/view.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <optional>

namespace ulam::sema {

// TODO: better diagnostics, trace type/value resolution somehow

class Resolver {
public:
    Resolver(Ref<Program> program): _program{program} {}

    void resolve();

    bool resolve(Ref<ClassTpl> cls_tpl);
    bool init(Ref<Class> cls);
    bool resolve(Ref<Class> cls);
    bool resolve(Ref<AliasType> alias, Ref<Scope> scope);
    bool resolve(Ref<Var> var, Ref<Scope> scope);
    bool resolve(Ref<AliasType> alias);
    bool resolve(Ref<Var> var);
    bool resolve(Ref<Prop> prop);
    bool resolve(Ref<FunSet> fset);
    bool resolve(Ref<Fun> fun);

    Ref<Class> resolve_class_name(
        Ref<ast::TypeName> type_name,
        Ref<Scope> scope,
        bool resolve_class = false);

    Ref<Type> resolve_full_type_name(
        Ref<ast::FullTypeName> full_type_name,
        Ref<Scope> scope,
        bool resolve_class = false);

    Ref<Type> resolve_type_name(
        Ref<ast::TypeName> type_name,
        Ref<Scope> scope,
        bool resolve_class = false);

    Ref<Type> resolve_type_spec(Ref<ast::TypeSpec> type_spec, Ref<Scope> scope);

private:
    Ptr<PersScopeView> decl_scope_view(Ref<Decl> decl);

    bool resolve_class_deps(Ref<Type> type);

    Ref<Type> resolve_var_decl_type(
        Ref<ast::TypeName> type_name,
        Ref<ast::VarDecl> node,
        Ref<Scope> scope,
        bool resolve_class = false);

    Ref<Type> resolve_fun_ret_type(Ref<ast::FunRetType> node, Ref<Scope> scope);

    Ref<Type> apply_array_dims(
        Ref<Type> type, Ref<ast::ExprList> dims, Ref<Scope> scope) {
        return apply_array_dims(type, dims, Ref<ast::InitList>{}, scope);
    }

    Ref<Type> apply_array_dims(
        Ref<Type> type,
        Ref<ast::ExprList> dims,
        Ref<ast::InitList> init_list,
        Ref<Scope> scope);

    std::optional<bool> check_state(Ref<Decl> decl);
    void update_state(Ref<Decl> decl, bool is_resolved);

    Diag& diag() { return _program->diag(); }
    std::string_view str(str_id_t str_id) const;

    Ref<Program> _program;
    std::set<Ref<Class>> _classes;
};

} // namespace ulam::sema
