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

class EvalVisitor;

class Resolver : public EvalHelperBase {
public:
    Resolver(EvalVisitor& eval, Ref<Program> program, bool in_expr):
        EvalHelperBase{eval, program}, _eval{eval}, _in_expr{in_expr} {}

    void resolve(Ref<Program> program);
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

    Ref<Type> resolve_type_spec(Ref<ast::TypeSpec> type_spec);

private:
    using ClassSet = std::unordered_set<Ref<Class>>;

    bitsize_t bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id);

    array_size_t array_size(Ref<ast::Expr> expr);

    // {dimensions, is_array}
    std::pair<ArrayDimList, bool>
    array_dims(unsigned num, Ref<ast::InitValue> init);

    std::pair<TypedValueList, bool>
    eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl);

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

    Scope* scope();
    eval_flags_t flags() const;

    EvalVisitor& eval();
    EvalVisitor& eval() const;

    EvalVisitor& _eval;
    bool _in_expr;
    ClassSet _classes;
};

} // namespace ulam::sema
