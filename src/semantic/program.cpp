#include "libulam/semantic/type/builtin_type_id.hpp"
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/builtin.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

Program::Program(Diag& diag, ast::Ref<ast::Root> ast):
    _diag{diag}, _ast{ast} {
    Ref<Program> self{this};
    _prim_type_tpls[IntId] = make<IntTypeTpl>(self);
    _prim_type_tpls[UnsignedId] = make<UnsignedTypeTpl>(self);
    _prim_type_tpls[BoolId] = make<BoolTypeTpl>(self);
    _prim_type_tpls[UnaryId] = make<UnaryTypeTpl>(self);
    _prim_type_tpls[BitsId] = make<BitsTypeTpl>(self);
    _prim_types[VoidId] = make<VoidType>(self);
    _prim_types[StringId] = make<StringType>(self);
}

Program::~Program() {}

str_id_t Program::self_str_id() { return _ast->ctx().str_id("Self"); }

str_id_t Program::self_inst_str_id() { return _ast->ctx().str_id("self"); }

Ref<Module> Program::module(module_id_t id) {
    assert(id < _modules.size());
    return ref(_modules[id]);
}

Ref<Module> Program::add_module(ast::Ref<ast::ModuleDef> node) {
    module_id_t id = _modules.size();
    _modules.push_back(ulam::make<Module>(this, id, node));
    return ref(_modules[id]);
}

Ref<PrimTypeTpl> Program::prim_type_tpl(BuiltinTypeId id) {
    assert(has_bitsize(id));
    assert(_prim_type_tpls.count(id) == 1);
    return ref(_prim_type_tpls[id]);
}

Ref<PrimType> Program::prim_type(BuiltinTypeId id) {
    assert(is_prim(id) && !has_bitsize(id));
    assert(_prim_types.count(id) == 1);
    return ref(_prim_types[id]);
}

Ref<TypeTpl> Program::builtin_type_tpl(BuiltinTypeId id) {
    assert(has_bitsize(id));
    return ref(_prim_type_tpls[id]);
}

Ref<Type> Program::builtin_type(BuiltinTypeId id) {
    assert(!has_bitsize(id));
    assert(is_prim(id) && "not implemented");
    return ref(_prim_types[id]);
}


} // namespace ulam
