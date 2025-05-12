#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <optional>

namespace ulam::sema {

// TODO: better diagnostics, trace type/value resolution somehow

class EvalVisitor;

class Resolver {
public:
    Resolver(
        EvalVisitor& eval,
        Ref<Program> program,
        bool in_expr,
        eval_flags_t flags = evl::NoFlags):
        _eval{eval},
        _diag{program->diag()},
        _builtins{program->builtins()},
        _str_pool{program->str_pool()},
        _in_expr{in_expr},
        _flags{flags} {}

    void resolve(Ref<Program> program);
    bool init(Ref<Class> cls);
    bool resolve(Ref<Class> cls);
    bool resolve(Ref<AliasType> alias, Scope* scope);
    bool resolve(Ref<Var> var, Scope* scope);
    bool resolve(Ref<AliasType> alias);
    bool resolve(Ref<Var> var);
    bool resolve(Ref<Prop> prop);
    bool init_default_value(Ref<Prop> prop);
    bool resolve(Ref<FunSet> fset);
    bool resolve(Ref<Fun> fun);

    Ref<Class> resolve_class_name(
        Ref<ast::TypeName> type_name, Scope* scope, bool resolve_class = false);

    Ref<Type> resolve_full_type_name(
        Ref<ast::FullTypeName> full_type_name,
        Scope* scope,
        bool resolve_class = false);

    Ref<Type> resolve_type_name(
        Ref<ast::TypeName> type_name, Scope* scope, bool resolve_class = false);

    Ref<Type> resolve_type_spec(Ref<ast::TypeSpec> type_spec, Scope* scope);

private:
    PersScopeView decl_scope_view(Ref<Decl> decl);

    bool resolve_class_deps(Ref<Type> type);

    Ref<Type> resolve_var_decl_type(
        Ref<ast::TypeName> type_name,
        Ref<ast::VarDecl> node,
        Scope* scope,
        bool resolve_class = false);

    Ref<Type> resolve_fun_ret_type(Ref<ast::FunRetType> node, Scope* scope);

    Ref<Type>
    apply_array_dims(Ref<Type> type, Ref<ast::ExprList> dims, Scope* scope) {
        return apply_array_dims(type, dims, Ref<ast::InitValue>{}, scope);
    }

    Ref<Type> apply_array_dims(
        Ref<Type> type,
        Ref<ast::ExprList> dims,
        Ref<ast::InitValue> init,
        Scope* scope);

    std::optional<bool> check_state(Ref<Decl> decl);
    void update_state(Ref<Decl> decl, bool is_resolved);

    std::string_view str(str_id_t str_id) const;

    EvalVisitor& _eval;
    Diag& _diag;
    Builtins& _builtins;
    UniqStrPool& _str_pool;
    bool _in_expr;
    eval_flags_t _flags;
    std::set<Ref<Class>> _classes;
};

} // namespace ulam::sema
