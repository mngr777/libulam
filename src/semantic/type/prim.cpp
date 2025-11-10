#include <cassert>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

// PrimType

PrimType::PrimType(Builtins& builtins, TypeIdGen* id_gen):
    Type{builtins, id_gen} {
    assert(id_gen);
}

const std::string_view PrimType::name() const {
    return builtin_type_str(bi_type_id());
}

RValue PrimType::load(const BitsView data, bitsize_t off) {
    return from_datum(data.read(off, bitsize()));
}
void PrimType::store(BitsView data, bitsize_t off, const RValue& rval) {
    data.write(off, bitsize(), to_datum(rval));
}

Ref<Type> PrimType::common(Ref<Type> type) {
    if (!type->is_prim())
        return {};
    return common_prim(type->as_prim());
}

Ref<Type>
PrimType::common(const Value& val1, Ref<Type> type, const Value& val2) {
    if (!type->is_prim())
        return {};
    return common_prim(val1, type->as_prim(), val2);
}

bool PrimType::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    if (!type->is_prim())
        return false;
    return (!val.empty() && !expl)
               ? is_impl_castable_to_prim(type->as_prim(), val)
               : is_castable_to_prim(type->as_prim(), expl);
}

bool PrimType::is_castable_to(
    BuiltinTypeId bi_type_id, const Value& val, bool expl) const {
    if (!ulam::is_prim(bi_type_id))
        return false;
    return (!val.empty() && !expl) ? is_impl_castable_to_prim(bi_type_id, val)
                                   : is_castable_to_prim(bi_type_id, expl);
}

Value PrimType::cast_to(Ref<Type> type, Value&& val) {
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

conv_cost_t PrimType::conv_cost(
    Ref<const Type> type, const Value& val, bool allow_cast) const {
    if (is_same(type))
        return 0;
    if (is_impl_castable_to(type, val))
        return prim_conv_cost(this, type->as_prim());
    if (allow_cast && is_expl_castable_to(type))
        return prim_cast_cost(this, type->as_prim());
    ;
    return MaxConvCost;
}

Ref<Type> PrimType::common_prim(Ref<PrimType> type) {
    if (type == this)
        return this;
    if (type->bi_type_id() != bi_type_id())
        return {};
    if (type->is_castable_to_prim(this, false))
        return this;
    if (is_castable_to_prim(type, false))
        return type;
    return {};
}

Ref<Type> PrimType::common_prim(
    const Value& val1, Ref<PrimType> type, const Value& val2) {
    if (type == this)
        return this;
    if (type->bi_type_id() != bi_type_id())
        return {};
    if (type->is_impl_castable_to_prim(this, val2))
        return this;
    if (is_impl_castable_to_prim(type, val1))
        return type;
    return {};
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
