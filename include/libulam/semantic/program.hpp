#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <vector>

namespace ulam::ast {
class Root;
class ModuleDef;
}

namespace ulam {

class Program {
public:
    Program(Diag& diag, ast::Ref<ast::Root> ast);
    ~Program();

    Ref<PrimTypeTpl> prim_type_tpl(BuiltinTypeId id);
    Ref<PrimType> prim_type(BuiltinTypeId id);
    // TODO: Atom (built-in non-primitive)

    Diag& diag() { return _diag; }

    ast::Ref<ast::Root> ast() { return _ast; }
    ast::Ref<const ast::Root> ast() const { return _ast; }

    auto& modules() { return _modules; }

    Ref<Scope> scope() { return ref(_scope); }

    // TODO: refactoring?
    str_id_t self_str_id();
    str_id_t self_inst_str_id();

    Ref<Module> module(module_id_t id);
    Ref<Module> add_module(ast::Ref<ast::ModuleDef> node);

    type_id_t next_type_id() { return _next_type_id++; }

private:
    Diag& _diag;
    ast::Ref<ast::Root> _ast;
    type_id_t _next_type_id{1};
    std::unordered_map<BuiltinTypeId, Ptr<PrimTypeTpl>> _prim_type_tpls;
    std::unordered_map<BuiltinTypeId, Ptr<PrimType>> _prim_types;
    std::vector<Ptr<Module>> _modules;
    Ptr<Scope> _scope;
};

} // namespace ulam
