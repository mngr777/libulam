#include "libulam/semantic/type/builtin_type_id.hpp"
#include "libulam/semantic/value/types.hpp"
#include <libulam/semantic/type/builtin/atom.hpp>
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
    _int_tpl = make<IntTypeTpl>(*this, id_gen);
    _unsigned_tpl = make<UnsignedTypeTpl>(*this, id_gen);
    _unary_tpl = make<UnaryTypeTpl>(*this, id_gen);
    _bits_tpl = make<BitsTypeTpl>(*this, id_gen);
    _bool_tpl = make<BoolTypeTpl>(*this, id_gen);
    _atom_type = make<AtomType>(*this, id_gen);
    _string_type = make<StringType>(*this, id_gen);
    _void_type = make<VoidType>(*this, id_gen);
    _fun_type = make<FunType>(id_gen);
}

Builtins::~Builtins() {}

Ref<PrimTypeTpl> Builtins::prim_type_tpl(BuiltinTypeId id) {
    switch (id) {
    case IntId:
        return ref(_int_tpl);
    case UnsignedId:
        return ref(_unsigned_tpl);
    case UnaryId:
        return ref(_unary_tpl);
    case BitsId:
        return ref(_bits_tpl);
    case BoolId:
        return ref(_bool_tpl);
    default:
        assert(false);
    }
}

Ref<PrimType> Builtins::prim_type(BuiltinTypeId id, bitsize_t size) {
    assert(size == NoBitsize || has_bitsize(id));
    switch (id) {
    case IntId:
        return int_type(size);
    case UnsignedId:
        return unsigned_type(size);
    case UnaryId:
        return unary_type(size);
    case BitsId:
        return bits_type(size);
    case BoolId:
        return bool_type(size);
    case StringId:
        return ref(_string_type);
    case VoidId:
        return ref(_void_type);
    default:
        assert(false);
    }
}

Ref<Type> Builtins::type(BuiltinTypeId id, bitsize_t size) {
    assert(size == NoBitsize || has_bitsize(id));
    switch (id) {
    case IntId:
        return int_type(size);
    case UnsignedId:
        return unsigned_type(size);
    case UnaryId:
        return unary_type(size);
    case BitsId:
        return bits_type(size);
    case BoolId:
        return bool_type(size);
    case AtomId:
        return ref(_atom_type);
    case StringId:
        return ref(_string_type);
    case VoidId:
        return ref(_void_type);
    case FunId:
        return ref(_fun_type);
    default:
        assert(false);
    }
}

Ref<IntType> Builtins::int_type(bitsize_t size) {
    if (size == NoBitsize)
        size = IntType::DefaultSize;
    return _int_tpl->exact_type(size);
}

Ref<IntTypeTpl> Builtins::int_tpl() {
    return ref(_int_tpl);
}

Ref<UnsignedType> Builtins::unsigned_type(bitsize_t size) {
    if (size == NoBitsize)
        size = UnsignedType::DefaultSize;
    return _unsigned_tpl->exact_type(size);
}

Ref<UnsignedTypeTpl> Builtins::unsigned_tpl() {
    return ref(_unsigned_tpl);
}

Ref<UnaryType> Builtins::unary_type(bitsize_t size) {
    if (size == NoBitsize)
        size = UnaryType::DefaultSize;
    return _unary_tpl->exact_type(size);
}

Ref<UnaryTypeTpl> Builtins::unary_tpl() {
    return ref(_unary_tpl);
}

Ref<BitsType> Builtins::bits_type(bitsize_t size) {
    if (size == NoBitsize)
        size = BitsType::DefaultSize;
    return _bits_tpl->exact_type(size);
}

Ref<BitsTypeTpl> Builtins::bits_tpl() {
    return ref(_bits_tpl);
}

Ref<BoolType> Builtins::bool_type(bitsize_t size) {
    if (size == NoBitsize)
        size = BoolType::DefaultSize;
    return _bool_tpl->exact_type(size);
}

Ref<BoolTypeTpl> Builtins::bool_tpl() {
    return ref(_bool_tpl);
}

Ref<AtomType> Builtins::atom_type() {
    return ref(_atom_type);
}

Ref<StringType> Builtins::string_type() {
    return ref(_string_type);
}

Ref<VoidType> Builtins::void_type() {
    return ref(_void_type);
}


} // namespace ulam
