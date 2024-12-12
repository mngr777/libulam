#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <optional>

namespace ulam::sema {

class Resolver {
public:
    Resolver(Ref<Program> program): _program{program} {}

    void resolve();

private:
    void resolve(Ref<Module> module);
    bool resolve(Ref<ClassTpl> cls_tpl);
    bool init(Ref<Class> cls);
    bool resolve(Ref<Class> cls);
    bool resolve(Ref<AliasType> alias, ScopeProxy scope);
    bool resolve(Ref<Var> var, ScopeProxy scope);
    bool resolve(Ref<Fun> fun);
    bool resolve(Ref<FunOverload> overload);

    Ref<Type> resolve_var_decl_type(
        Ref<ast::TypeName> type_name,
        Ref<ast::VarDecl> node,
        ScopeProxy scope);

    Ref<Type>
    resolve_type_name(Ref<ast::TypeName> type_name, ScopeProxy scope);

    Ref<Type> apply_array_dims(
        Ref<Type> type, Ref<ast::ExprList> dims, ScopeProxy scope);

    std::optional<bool> check_state(Ref<ScopeObject> obj);
    void update_state(Ref<ScopeObject> obj, bool is_resolved);

    Diag& diag() { return _program->diag(); }
    Ref<ast::Root> ast() { return _program->ast(); }
    std::string_view str(str_id_t str_id) const;

    Ref<Program> _program;
    std::list<Ref<Class>> _classes;
};

} // namespace ulam::sema
