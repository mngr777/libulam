#include "libulam/semantic/value/flags.hpp"
#include <libulam/assert.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/utils/integer.hpp>

namespace ulam {

Unsigned UnaryType::unsigned_value(const RValue& rval) {
    ulam_assert(rval.has_rvalue());
    Unsigned uns_val = utils::count_ones(rval.get<Unsigned>());
    ulam_assert(uns_val <= bitsize());
    return uns_val;
}

TypedValue UnaryType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{construct(0, value::IsConsteval)}};
    case TypeOp::MaxOf:
        return {this, Value{construct(bitsize(), value::IsConsteval)}};
    default:
        return _PrimType::type_op(op);
    }
}

RValue UnaryType::construct_default(value::flags_t rval_flags) {
    return construct(0, rval_flags);
}

RValue UnaryType::construct(Unsigned uns_val, value::flags_t rval_flags) {
    ulam_assert(uns_val <= bitsize());
    return RValue::make(utils::ones(uns_val), rval_flags);
}

RValue UnaryType::from_datum(Datum datum) {
    return RValue::make((Unsigned)datum);
}

Datum UnaryType::to_datum(const RValue& rval) {
    ulam_assert(rval.is<Unsigned>());
    return (Datum)rval.get<Unsigned>();
}

TypedValue UnaryType::unary_op(Op op, RValue&& rval) {
    if (!rval.has_rvalue())
        return {this, Value{RValue{}}};

    bool is_consteval = rval.is_consteval();
    Unsigned uns_val = unsigned_value(rval);
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (op) {
    case Op::UnaryMinus: {
        bitsize_t size = utils::bitsize(uns_val);
        ulam_assert(size <= IntType::DefaultSize);
        auto type = builtins().int_type(size);
        return {type, Value{type->construct(-(Integer)uns_val, rval_flags)}};
    }
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        is_consteval = false;
        if (uns_val < bitsize())
            ++uns_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        is_consteval = false;
        if (uns_val > 0)
            --uns_val;
        break;
    default:
        unreachable();
    }
    return {this, Value::make_r(utils::ones(uns_val), rval_flags)};
}

TypedValue UnaryType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    ulam_assert(r_type->is(UnaryId));
    ulam_assert(!l_rval.has_rvalue() || l_rval.is<Unsigned>());
    ulam_assert(!r_rval.has_rvalue() || r_rval.is<Unsigned>());

    bool is_unknown = !l_rval.has_rvalue() || !r_rval.has_rvalue();

    Unsigned l_uns = l_rval.has_rvalue() ? unsigned_value(l_rval) : 0;
    auto r_unary_type = builtins().unary_type(r_type->bitsize());
    Unsigned r_uns =
        r_rval.has_rvalue() ? r_unary_type->unsigned_value(r_rval) : 0;

    bool is_consteval =
        !is_unknown && l_rval.is_consteval() && r_rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_uns == r_uns, rval_flags)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_uns != r_uns, rval_flags)}};
    }
    default:
        unreachable();
    }
}

bool UnaryType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->bi_type_id()) {
    case IntId:
        return expl || utils::integer_max(size) >= bitsize();
    case UnsignedId:
        return expl || utils::unsigned_max(size) >= bitsize();
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return expl || size >= bitsize();
    case BitsId:
        return expl || size >= bitsize();
    case AtomId:
        return false;
    case StringId:
        return false;
    case FunId:
    case VoidId:
    default:
        unreachable();
    }
}

bool UnaryType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return true;
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return true;
    case BitsId:
        return true;
    case AtomId:
        return false;
    case StringId:
        return false;
    case FunId:
    case VoidId:
    default:
        unreachable();
    }
}

TypedValue UnaryType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    ulam_assert(is_expl_castable_to(id));

    bool is_unknown = !rval.has_rvalue();
    bool is_consteval = rval.is_consteval();
    auto uns_val = is_unknown ? 0 : unsigned_value(rval);
    value::flags_t rval_flags = value::IsConsteval;

    switch (id) {
    case IntId: {
        Integer int_val = uns_val;
        if (is_consteval) {
            auto size = utils::bitsize(int_val);
            auto type = builtins().int_type(size);
            return {type, Value{type->construct(int_val, rval_flags)}};
        } else {
            auto size = utils::bitsize((Integer)bitsize());
            auto type = builtins().int_type(size);
            if (is_unknown)
                return {type, Value::make_r_ph()};
            return {type, Value{type->construct(int_val, rval_flags)}};
        }
    }
    case UnsignedId: {
        if (is_consteval) {
            auto size = utils::bitsize(uns_val);
            auto type = builtins().unsigned_type(size);
            return {type, Value{type->construct(uns_val, rval_flags)}};
        } else {
            auto size = utils::bitsize((Unsigned)bitsize());
            auto type = builtins().unsigned_type(size);
            if (is_unknown)
                return {type, Value::make_r_ph()};
            return {type, Value{type->construct(uns_val, rval_flags)}};
        }
    }
    case BoolId: {
        ulam_assert(bitsize() == 1);
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(uns_val > 0, rval_flags)}};
    }
    case UnaryId: {
        unreachable();
        // return {this, Value{std::move(rval)}};
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        Bits bits_val{size};
        store(bits_val.view(), 0, rval);
        return {type, Value::make_r(std::move(bits_val), rval_flags)};
    }
    default:
        unreachable();
    }
}

RValue UnaryType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    ulam_assert(is_expl_castable_to(type));
    if (!rval.has_rvalue())
        return std::move(rval);

    auto unary_type = builtins().unary_type(type->bitsize());
    Unsigned uns_val = unary_type->unsigned_value(rval);
    value::flags_t rval_flags = value::IsConsteval * rval.is_consteval();

    switch (type->bi_type_id()) {
    case IntId: {
        Unsigned int_max = utils::integer_max(type->bitsize());
        Integer int_val = std::min(int_max, uns_val);
        return RValue::make(int_val, rval_flags);
    }
    case UnsignedId: {
        uns_val = std::min(utils::unsigned_max(type->bitsize()), uns_val);
        return RValue::make(uns_val, rval_flags);
    }
    case BoolId: {
        ulam_assert(bitsize() == 1);
        auto bool_type = builtins().bool_type(type->bitsize());
        return bool_type->construct(uns_val > 0, rval_flags);
    }
    case UnaryId: {
        uns_val = std::min<Unsigned>(type->bitsize(), uns_val);
        return RValue::make(utils::ones(uns_val), rval_flags);
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        uns_val = std::min<bitsize_t>(uns_val, size);
        Datum datum = utils::ones(uns_val);
        auto bits_rval = type->construct_default(rval_flags);
        bits_rval.get<Bits>().write_right(size, datum);
        return bits_rval;
    }
    default:
        unreachable();
    }
}

} // namespace ulam
