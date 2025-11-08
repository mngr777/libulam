#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <optional>
#include <unordered_set>

namespace ulam::sema {

// TODO: better diagnostics, trace type/value resolution somehow
// TODO: split into eval helper and program/module resolver?

class EvalEnv;

class Resolver : public EvalHelper {
public:
    Resolver(EvalEnv& env, bool in_expr): EvalHelper{env}, _in_expr{in_expr} {}

    void resolve(Ref<Program> program); // TODO: move to constr (or somewhere)
    bool init(Ref<Class> cls);
    bool resolve(Ref<Class> cls);
    bool resolve(Ref<AliasType> alias);
    bool resolve(Ref<Var> var);
    bool resolve(Ref<Prop> prop);
    bool init_default_value(Ref<Prop> prop);
    bool resolve(Ref<FunSet> fset);
    bool resolve(Ref<Fun> fun);

    Ref<Class> resolve_class_name(
        Ref<ast::TypeName> type_name, bool resolve_class = false);

    Ref<Type> resolve_full_type_name(
        Ref<ast::FullTypeName> full_type_name, bool resolve_class = false);

    Ref<Type>
    resolve_type_name(Ref<ast::TypeName> type_name, bool resolve_class = false);

private:
    using ClassSet = std::unordered_set<Ref<Class>>;
    using VarDefaults = std::unordered_map<Ref<Var>, ExprRes>;

    class VarDefaultsRaii {
    public:
        VarDefaultsRaii(Resolver& resolver, VarDefaults&& var_defaults):
            _resolver{&resolver}, _var_defaults(std::move(var_defaults)) {
            std::swap(_var_defaults, resolver._var_defaults);
            resolver._var_defaults.merge(_var_defaults);
        }

        VarDefaultsRaii(): _resolver{} {}

        ~VarDefaultsRaii() {
            if (_resolver)
                std::swap(_var_defaults, _resolver->_var_defaults);
        }

        VarDefaultsRaii(VarDefaultsRaii&& other) {
            operator=(std::move(other));
        }

        VarDefaultsRaii& operator=(VarDefaultsRaii&& other) {
            std::swap(_resolver, other._resolver);
            std::swap(_var_defaults, other._var_defaults);
            return *this;
        }

    private:
        Resolver* _resolver;
        VarDefaults _var_defaults;
    };

    VarDefaultsRaii var_defaults_raii(VarDefaults&& var_defaults);

    Ref<Type> do_resolve_type_name(
        Ref<ast::TypeName> type_name,
        Ref<AliasType> exclude_alias,
        bool resolve_class);

    Ref<Type> resolve_type_spec(
        Ref<ast::TypeSpec> type_spec, Ref<AliasType> exclude_alias);

    bitsize_t bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id);

    array_size_t array_size(Ref<ast::Expr> expr);

    // {dimensions, is_array}
    std::pair<ArrayDimList, bool>
    array_dims(unsigned num, Ref<ast::InitValue> init);

    std::pair<TypedValueList, bool>
    eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl);

    std::pair<TypedValueList, bool>
    do_eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl);

    std::pair<TypedValueList, bool>
    do_eval_tpl_args_compat(Ref<ast::ArgList> args, Ref<ClassTpl> tpl);

    std::pair<ExprResList, bool> eval_args(Ref<ast::ArgList> args);

    PersScopeView decl_scope_view(Ref<Decl> decl);

    bool resolve_class_deps(Ref<Type> type);

    Ref<Type> resolve_var_decl_type(
        Ref<ast::TypeName> type_name,
        Ref<ast::VarDecl> node,
        bool resolve_class = false);

    Ref<Type> resolve_fun_ret_type(Ref<ast::FunRetType> node);

    Ref<Type> apply_array_dims(Ref<Type> type, Ref<ast::ExprList> dims);

    Ref<Type> apply_array_dims(
        Ref<Type> type, Ref<ast::ExprList> dims, Ref<ast::InitValue> init);

    std::optional<bool> check_state(Ref<Decl> decl);
    void update_state(Ref<Decl> decl, bool is_resolved);

    bool _in_expr;
    ClassSet _classes;
    VarDefaults _var_defaults;
};

} // namespace ulam::sema
