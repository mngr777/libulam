#include <libulam/assert.hpp>
#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/utils/integer.hpp>

namespace ulam {

namespace {
bool _is_true(Unsigned uns_val, bitsize_t bitsize) {
    return utils::count_ones(uns_val) >= (bitsize + 1u) / 2;
}

Datum _to_datum(bool value, bitsize_t bitsize) {
    return utils::ones(bitsize * value);
}

} // namespace

TypedValue BoolType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{construct(false, value::IsConsteval)}};
    case TypeOp::MaxOf:
        return {this, Value{construct(true, value::IsConsteval)}};
    default:
        return _PrimType::type_op(op);
    }
}

RValue BoolType::construct_default(value::flags_t rval_flags) {
    return RValue::make(Unsigned{}, rval_flags);
}

RValue BoolType::construct(bool value, value::flags_t rval_flags) {
    Unsigned uns_val = value ? utils::ones(bitsize()) : 0;
    return RValue::make(uns_val, rval_flags);
}

bool BoolType::is_true(const RValue& rval) const {
    ulam_assert(rval.is<Unsigned>());
    return _is_true(rval.get<Unsigned>(), bitsize());
}

RValue BoolType::from_datum(Datum datum) {
    return construct(_is_true((Unsigned)datum, bitsize()));
}

Datum BoolType::to_datum(const RValue& rval) {
    ulam_assert(rval.is<Unsigned>());
    bool truth = _is_true(rval.get<Unsigned>(), bitsize());
    return _to_datum(truth, bitsize());
}

TypedValue BoolType::unary_op(Op op, RValue&& rval) {
    if (!rval.has_rvalue())
        return {this, Value{std::move(rval)}};

    bool truth = is_true(rval);
    bool is_consteval = rval.is_consteval();

    switch (op) {
    case Op::Negate: {
        auto rval = construct(!truth, value::IsConsteval * is_consteval);
        return {this, Value{std::move(rval)}};
    }
    default:
        unreachable();
    }
}

TypedValue BoolType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    ulam_assert(r_type->is(BoolId));
    ulam_assert(!l_rval.has_rvalue() || l_rval.is<Unsigned>());
    ulam_assert(!r_rval.has_rvalue() || r_rval.is<Unsigned>());

    auto type = builtins().bool_type(std::max(bitsize(), bitsize()));
    bool is_unknown = !l_rval.has_rvalue() || !r_rval.has_rvalue();
    if (is_unknown)
        return {type, Value::make_r_ph()};

    auto left = is_true(l_rval);
    auto right = builtins().bool_type(r_type->bitsize())->is_true(r_rval);
    bool is_consteval = l_rval.is_consteval() && r_rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (op) {
    case Op::Equal:
        return {type, Value{type->construct(left == right, rval_flags)}};
    case Op::NotEqual:
        return {type, Value{type->construct(left != right, rval_flags)}};
    case Op::And:
        return {type, Value{type->construct(left && right, rval_flags)}};
    case Op::Or:
        return {type, Value{type->construct(left || right, rval_flags)}};
    default:
        unreachable();
    }
}

bool BoolType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case BoolId:
        return true;
    case BitsId:
        return type->bitsize() >= bitsize();
    default:
        return is_castable_to(type->bi_type_id(), expl);
    }
}

bool BoolType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl;
    case BoolId:
        return true;
    case UnaryId:
        return expl;
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

TypedValue BoolType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    ulam_assert(is_expl_castable_to(id));

    switch (id) {
    case IntId: {
        auto type = builtins().int_type(2);
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    case UnsignedId: {
        auto type = builtins().unsigned_type(1);
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    case UnaryId: {
        auto type = builtins().unary_type(1);
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    case BoolId: {
        unreachable();
        // return {this, Value{std::move(rval)}};
    }
    case BitsId: {
        auto type = builtins().bits_type(bitsize());
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    default:
        unreachable();
    }
}

RValue BoolType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    ulam_assert(is_expl_castable_to(type));
    if (!rval.has_rvalue())
        return std::move(rval);

    auto rval_flags = value::IsConsteval * rval.is_consteval();
    bool truth = is_true(rval);

    switch (type->bi_type_id()) {
    case IntId: {
        return RValue::make((Integer)truth, rval_flags);
    }
    case UnsignedId: {
        return RValue::make((Unsigned)truth, rval_flags);
    }
    case UnaryId: {
        return RValue::make((Unsigned)truth, rval_flags);
    }
    case BoolId: {
        return RValue::make(_to_datum(truth, type->bitsize()), rval_flags);
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        auto boolean = builtins().bool_type(size);
        auto datum = boolean->to_datum(boolean->construct(truth));
        auto bits_rval = type->construct_default(rval_flags);
        bits_rval.get<Bits>().write_right(size, datum);
        return bits_rval;
    }
    default:
        unreachable();
    }
}

} // namespace ulam
