#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class AtomType;
class BoolType;
class BoolTypeTpl;
class BitsType;
class BitsTypeTpl;
class FunType;
class IntType;
class IntTypeTpl;
class StringType;
class UnaryType;
class UnaryTypeTpl;
class UnsignedType;
class UnsignedTypeTpl;
class VoidType;

class Builtins {
public:
    Builtins(TypeIdGen& id_gen, UniqStrPool& text_pool);
    ~Builtins();

    Builtins(Builtins&&) = default;
    Builtins& operator=(Builtins&&) = default;

    Ref<IntType> int_type(bitsize_t size = NoBitsize);
    Ref<IntTypeTpl> int_tpl();

    Ref<UnsignedType> unsigned_type(bitsize_t size = NoBitsize);
    Ref<UnsignedTypeTpl> unsigned_tpl();

    Ref<UnaryType> unary_type(bitsize_t size = NoBitsize);
    Ref<UnaryTypeTpl> unary_tpl();

    Ref<BitsType> bits_type(bitsize_t size = NoBitsize);
    Ref<BitsTypeTpl> bits_tpl();

    Ref<BoolType> bool_type(bitsize_t size = NoBitsize);
    Ref<BoolTypeTpl> bool_tpl();

    Ref<AtomType> atom_type();
    Ref<StringType> string_type();
    Ref<VoidType> void_type();
    Ref<FunType> fun_type();

    Ref<BoolType> boolean() { return bool_type(1); }

    Ref<PrimTypeTpl> prim_type_tpl(BuiltinTypeId id);

    Ref<PrimType> prim_type(BuiltinTypeId id, bitsize_t size = NoBitsize);

    Ref<Type> type(BuiltinTypeId id, bitsize_t size = NoBitsize);

private:
    Ptr<IntTypeTpl> _int_tpl;
    Ptr<UnsignedTypeTpl> _unsigned_tpl;
    Ptr<UnaryTypeTpl> _unary_tpl;
    Ptr<BitsTypeTpl> _bits_tpl;
    Ptr<BoolTypeTpl> _bool_tpl;
    Ptr<AtomType> _atom_type;
    Ptr<StringType> _string_type;
    Ptr<VoidType> _void_type;
    Ptr<FunType> _fun_type;
};

} // namespace ulam
