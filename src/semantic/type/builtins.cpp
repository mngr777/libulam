#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/fun.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

Builtins::Builtins(TypeIdGen& id_gen) {
    _prim_type_tpls[IntId] = make<IntTypeTpl>(id_gen);
    _prim_type_tpls[UnsignedId] = make<UnsignedTypeTpl>(id_gen);
    _prim_type_tpls[BoolId] = make<BoolTypeTpl>(id_gen);
    _prim_type_tpls[UnaryId] = make<UnaryTypeTpl>(id_gen);
    _prim_type_tpls[BitsId] = make<BitsTypeTpl>(id_gen);
    _prim_types[VoidId] = make<VoidType>(&id_gen);
    _prim_types[StringId] = make<StringType>(&id_gen);
    _other_types[FunId] = make<FunType>(&id_gen);
}

Ref<PrimTypeTpl> Builtins::prim_type_tpl(BuiltinTypeId id) {
    assert(has_bitsize(id));
    assert(_prim_type_tpls.count(id) == 1);
    return ref(_prim_type_tpls[id]);
}

Ref<PrimType> Builtins::prim_type(BuiltinTypeId id) {
    assert(is_prim(id) && !has_bitsize(id));
    assert(_prim_types.count(id) == 1);
    return ref(_prim_types[id]);
}

Ref<Type> Builtins::type(BuiltinTypeId id) {
    assert(!has_bitsize(id));
    Ref<Type> type{};
    if (is_prim(id)) {
        type = ref(_prim_types[id]);
    } else {
        type = ref(_other_types[id]);
    }
    assert(type);
    return type;
}

Ref<Type> Builtins::boolean() { return prim_type_tpl(BoolId)->type(1); }

} // namespace ulam
