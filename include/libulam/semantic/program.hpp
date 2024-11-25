#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/type_id_gen.hpp>
#include <list>
#include <unordered_map>

namespace ulam::ast {
class Root;
}

namespace ulam {

class Program {
public:
    Program(Diag& diag, ast::Ref<ast::Root> ast);

    Ref<PrimTypeTpl> prim_type_tpl(BuiltinTypeId id);
    Ref<PrimType> prim_type(BuiltinTypeId id);

    Diag& diag() { return _diag; }
    ast::Ref<ast::Root> ast() { return _ast; }

    auto& modules() { return _modules; }

    void add_module(Ptr<Module>&& module) {
        _modules.push_back(std::move(module));
    }

    type_id_t next_type_id() { return _type_id_gen.next(); }
    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    Diag& _diag;
    ast::Ref<ast::Root> _ast;
    TypeIdGen _type_id_gen;
    std::unordered_map<BuiltinTypeId, Ptr<PrimTypeTpl>> _prim_type_tpls;
    std::unordered_map<BuiltinTypeId, Ptr<PrimType>> _prim_types;
    std::list<Ptr<Module>> _modules;
};

} // namespace ulam
