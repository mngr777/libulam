#include <cassert>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <sstream>

namespace ulam {

// PrimType

PrimType::PrimType(Builtins& builtins, TypeIdGen* id_gen):
    Type{builtins, id_gen} {
    assert(id_gen);
}

std::string PrimType::name() const {
    std::stringstream ss;
    ss << builtin_type_str(bi_type_id());
    ss << "(" << (Unsigned)bitsize() << ")";
    return ss.str();
}

RValue PrimType::load(const BitsView data, bitsize_t off) const {
    return from_datum(data.read(off, bitsize()));
}
void PrimType::store(
    BitsView data, bitsize_t off, const RValue& rval) const {
    data.write(off, bitsize(), to_datum(rval));
}

bool PrimType::is_castable_to(Ref<const Type> type, bool expl) const {
    auto canon = type->canon();
    if (!canon->is_prim())
        return false;
    return is_castable_to_prim(canon->as_prim(), expl);
}

bool PrimType::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    if (!ulam::is_prim(bi_type_id))
        return false;
    return is_castable_to_prim(bi_type_id, expl);
}

bool PrimType::is_impl_castable_to(
    Ref<const Type> type, const Value& val) const {
    auto canon = type->canon();
    if (!canon->is_prim())
        return false;
    return is_impl_castable_to_prim(canon->as_prim(), val);
}

bool PrimType::is_impl_castable_to(
    BuiltinTypeId bi_type_id, const Value& val) const {
    if (!ulam::is_prim(bi_type_id))
        return false;
    return is_impl_castable_to_prim(bi_type_id, val);
}

Value PrimType::cast_to(Ref<const Type> type, Value&& val) {
    return Value{cast_to_prim(type->as_prim(), val.move_rvalue())};
}

TypedValue PrimType::cast_to(BuiltinTypeId bi_type_id, Value&& val) {
    return cast_to_prim(bi_type_id, val.move_rvalue());
}

conv_cost_t PrimType::conv_cost(Ref<const Type> type, bool allow_cast) const {
    if (is_same(type))
        return 0;
    if (is_impl_castable_to(type))
        return prim_conv_cost(this, type->as_prim());
    if (allow_cast && is_expl_castable_to(type))
        return prim_cast_cost(this, type->as_prim());
    return MaxConvCost;
}

conv_cost_t PrimType::conv_cost(Ref<const Type> type, const Value& val, bool allow_cast) const {
    if (is_same(type))
        return 0;
    if (is_impl_castable_to(type, val))
        return prim_conv_cost(this, type->as_prim());
    if (allow_cast && is_expl_castable_to(type))
        return prim_cast_cost(this, type->as_prim());;
    return MaxConvCost;
}

bool PrimType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    return false;
}

bool PrimType::is_castable_to_prim(BuiltinTypeId bi_type_id, bool expl) const {
    return false;
}

bool PrimType::is_impl_castable_to_prim(
    Ref<const PrimType> type, const Value& val) const {
    return is_castable_to_prim(type, false);
}

bool PrimType::is_impl_castable_to_prim(
    BuiltinTypeId bi_type_id, const Value& value) const {
    return is_castable_to_prim(bi_type_id, false);
}

// PrimTypeTpl

PrimTypeTpl::PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen):
    TypeTpl{id_gen}, _builtins{builtins} {}

} // namespace ulam
