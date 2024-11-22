#include "libulam/semantic/type/builtin_type_id.hpp"
#include <cassert>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/builtin.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

Program::Program(Diag& diag, ast::Ref<ast::Root> ast): _diag{diag}, _ast{ast} {
    Ref<Program> self{this};
    _prim_type_tpls[IntId] = make<IntTypeTpl>(self);
    _prim_type_tpls[UnsignedId] = make<UnsignedTypeTpl>(self);
    _prim_type_tpls[BoolId] = make<BoolTypeTpl>(self);
    _prim_type_tpls[UnaryId] = make<UnaryTypeTpl>(self);
    _prim_type_tpls[BitsId] = make<BitsTypeTpl>(self);
    _prim_types[VoidId] = make<VoidType>(self);
    _prim_types[StringId] = make<StringType>(self);
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

} // namespace ulam
