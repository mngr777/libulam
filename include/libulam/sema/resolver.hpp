#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
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
    bool resolve(Ref<ClassTpl> cls_tpl); // only resolve params
    bool resolve(Ref<Class> cls);        // resolve all members
    bool resolve(Ref<AliasType> alias, ScopeProxy scope);
    bool resolve(Ref<Var> var, ScopeProxy scope);
    bool resolve(Ref<Fun> fun);

    Ref<Type>
    resolve_type_name(ast::Ref<ast::TypeName> type_name, ScopeProxy scope);

    std::optional<bool> check_state(Ref<ScopeObject> obj);
    void update_state(Ref<ScopeObject> obj, bool is_resolved);

    Diag& diag() { return _program->diag(); }
    ast::Ref<ast::Root> ast() { return _program->ast(); }
    std::string_view str(str_id_t str_id) const;

    Ref<Program> _program;
};

} // namespace ulam::sema
