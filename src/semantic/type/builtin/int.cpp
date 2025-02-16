#include "src/semantic/detail/integer.hpp"
#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

TypedValue IntType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{RValue{detail::integer_min(bitsize())}}};
    case TypeOp::MaxOf:
        return {this, Value{RValue{detail::integer_max(bitsize())}}};
    default:
        assert(false);
    }
}

RValue IntType::from_datum(Datum datum) const {
    int shift = sizeof(datum) * 8 - bitsize();
    if (shift == 0)
        return RValue{(Integer)datum};
    assert(shift > 0);
    if (1 << (bitsize() - 1) & datum) {
        // negative, prepend with 1's
        datum |= ((1 << (shift + 1)) - 1) << bitsize();
    }
    return RValue{(Integer)datum};
}

Datum IntType::to_datum(const RValue& rval) const {
    assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    auto datum = (Datum)int_val;
    int shift = sizeof(datum) * 8 - bitsize();
    if (shift == 0)
        return datum;
    assert(shift > 0);
    datum = (datum << shift) >> shift;
    return datum;
}

bool IntType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return expl;
    case BoolId:
        return false;
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
        assert(false);
    }
}

bool IntType::is_castable_to(Ref<PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->builtin_type_id()) {
    case IntId:
        return expl || size <= bitsize();
    case UnsignedId:
        return expl || size <= bitsize() + 1;
    case BoolId:
        return false;
    case UnaryId:
        return expl || detail::unary_unsigned_bitsize(size) <= bitsize() + 1;
    case BitsId:
        return expl || size <= bitsize();
    case AtomId:
        return false;
        return false;
    case FunId:
    case VoidId:
    default:
        assert(false);
    }
}

PrimTypedValue IntType::cast_to(BuiltinTypeId id, Value&& value) {
    assert(is_expl_castable_to(id));
    assert(!value.empty());

    auto rval = value.move_rvalue();
    assert(rval.is<Integer>());

    auto int_val = rval.get<Integer>();
    switch (id) {
    case IntId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case UnsignedId: {
        int_val = std::max((Integer)0, int_val);
        auto size = detail::bitsize(int_val);
        auto type = builtins().prim_type(UnsignedId, size);
        return {type, Value{RValue{(Unsigned)int_val}}};
    }
    case UnaryId: {
        Unsigned val = std::max((Integer)0, int_val);
        val = std::min((Unsigned)ULAM_MAX_INT_SIZE, detail::ones(val));
        auto type = builtins().prim_type(UnaryId, val);
        return {type, Value{RValue{val}}};
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        store(val.bits().view(), 0, rval);
        return {type, Value{RValue{std::move(val)}}};
    }
    default:
        assert(false);
    }
}

RValue IntType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Integer>());

    auto int_val = rval.get<Integer>();
    switch (type->builtin_type_id()) {
    case IntId: {
        auto int_min = detail::integer_min(type->bitsize());
        if (int_val < int_min)
            return RValue{int_min};
        auto int_max = detail::integer_max(type->bitsize());
        if (int_val > int_max)
            return RValue{int_max};
        return std::move(rval);
    }
    case UnsignedId: {
        Unsigned val = std::max((Integer)0, int_val);
        val = std::min(detail::unsigned_max(type->bitsize()), val);
        return RValue{val};
    }
    case UnaryId: {
        Unsigned val = std::max((Integer)0, int_val);
        val = std::min((Unsigned)type->bitsize(), detail::ones(val));
        return RValue{val};
    }
    default:
        assert(false);
    }
}

PrimTypedValue IntType::binary_op(
    Op op,
    Value&& left_val,
    Ref<const PrimType> right_type,
    Value&& right_val) {
    assert(right_type->is(IntId));

    auto left_rval = left_val.move_rvalue();
    auto right_rval = right_val.move_rvalue();
    assert(left_rval.empty() || left_rval.is<Integer>());
    assert(right_rval.empty() || right_rval.is<Integer>());

    bool is_unknown = left_rval.empty() || right_rval.empty();
    Integer left_int_val = left_rval.empty() ? 0 : left_rval.get<Integer>();
    Integer right_int_val = right_rval.empty() ? 0 : right_rval.get<Integer>();

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        return {type, Value{type->construct(left_int_val == right_int_val)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        return {type, Value{type->construct(left_int_val != right_int_val)}};
    }
    case Op::Prod: {
        // Int(a) * Int(b) = Int(a + b)
        auto size =
            std::min(MaxSize, (bitsize_t)(bitsize() + right_type->bitsize()));
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_prod(left_int_val, right_int_val);
        return {type, Value{RValue{detail::truncate(val, size)}}};
    }
    case Op::Quot: {
        // Int(a) / Int(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_quot(left_int_val, right_int_val);
        return {this, Value{RValue{val}}};
    }
    case Op::Rem: {
        // Int(a) % Int(b) = Int(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_rem(left_int_val, right_int_val);
        return {this, Value{RValue{val}}};
    }
    case Op::Sum: {
        // Int(a) + Int(b) + Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_sum(left_int_val, right_int_val);
        return {type, Value{RValue{val}}};
    }
    case Op::Diff: {
        // Int(a) - Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_diff(left_int_val, right_int_val);
        return {type, Value{RValue{val}}};
    }
    case Op::Less: {
        return {
            builtins().boolean(),
            Value{RValue{(Unsigned)(left_int_val < right_int_val)}}};
    }
    case Op::LessOrEq: {
        return {
            builtins().boolean(),
            Value(RValue{(Unsigned)(left_int_val <= right_int_val)})};
    }
    case Op::Greater: {
        return {
            builtins().boolean(),
            Value{RValue{(Unsigned)(left_int_val > right_int_val)}}};
    }
    case Op::GreaterOrEq: {
        return {
            builtins().boolean(),
            Value(RValue{(Unsigned)(left_int_val >= right_int_val)})};
    }
    default:
        assert(false);
    }
}

} // namespace ulam
