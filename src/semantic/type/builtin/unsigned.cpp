#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

RValue UnsignedType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum UnsignedType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

bool UnsignedType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return true;
    case BoolId:
        return false;
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
        assert(false);
    }
}

bool UnsignedType::is_castable_to(Ref<const PrimType> type, bool expl) const {
    switch (type->builtin_type_id()) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl || type->bitsize() >= bitsize();
    case BoolId:
        return false;
    case UnaryId:
        return expl;
    case BitsId:
        return expl;
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

PrimTypedValue UnsignedType::cast_to(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));
    assert(!rval.empty());
    assert(rval.is<Unsigned>());

    auto uns_val = rval.get<Unsigned>();
    switch (id) {
    case IntId: {
        auto size = std::min(
            (bitsize_t)ULAM_MAX_INT_SIZE,
            (bitsize_t)(detail::bitsize(uns_val) + 1));
        Integer val = std::min((Unsigned)detail::integer_max(size), uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, Value{RValue{val}}};
    }
    case UnsignedId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)ULAM_MAX_INT_SIZE, uns_val);
        auto type = builtins().prim_type(UnaryId, detail::ones(uns_val));
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

RValue UnsignedType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = rval.get<Unsigned>();
    switch (type->builtin_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val};
    }
    case UnsignedId: {
        auto uns_max = detail::unsigned_max(type->bitsize());
        if (uns_val > uns_max)
            return RValue{uns_max};
        return std::move(rval);
    }
    case UnaryId: {
        Unsigned val =
            std::min((Unsigned)type->bitsize(), detail::ones(uns_val));
        return RValue{val};
    }
    default:
        assert(false);
    }
}

PrimTypedValue UnsignedType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{RValue{}}};

    auto uns_val = rval.get<Unsigned>();
    switch (op) {
    case Op::UnaryMinus: {
        bitsize_t size = std::min<bitsize_t>(bitsize() + 1, ULAM_MAX_INT_SIZE);
        Integer int_val =
            std::max<Integer>(detail::integer_min(size), -uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, Value{RValue{int_val}}};
    }
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        if (uns_val < detail::unsigned_max(bitsize()))
            ++uns_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        if (uns_val > 0)
            --uns_val;
        break;
    default:
        assert(false);
    }
    return {this, Value{RValue{uns_val}}};
}

PrimTypedValue UnsignedType::binary_op(
    Op op,
    RValue&& left_rval,
    Ref<const PrimType> right_type,
    RValue&& right_rval) {
    assert(right_type->is(UnsignedId));
    assert(left_rval.empty() || left_rval.is<Unsigned>());
    assert(right_rval.empty() || right_rval.is<Unsigned>());

    bool is_unknown = left_rval.empty() || right_rval.empty();
    Unsigned left_uns_val = left_rval.empty() ? 0 : left_rval.get<Unsigned>();
    Unsigned right_uns_val =
        right_rval.empty() ? 0 : right_rval.get<Unsigned>();

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns_val == right_uns_val)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns_val != right_uns_val)}};
    }
    case Op::Prod: {
        // Unsigned(a) * Unsigned(b) = Unsigned(a + b)
        auto size = std::min<bitsize_t>(MaxSize, bitsize() + right_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_prod(left_uns_val, right_uns_val);
        return {this, Value{RValue{val}}};
    }
    case Op::Quot: {
        // Unsigned(a) / Unsigned(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_quot(left_uns_val, right_uns_val);
        return {this, Value{RValue{val}}};
    }
    case Op::Rem: {
        // Unsigned(a) % Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_rem(left_uns_val, right_uns_val);
        return {this, Value{RValue{val}}};
    }
    case Op::Sum: {
        // Unsigned(a) + Unsigned(b) = Unsigned(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_sum(left_uns_val, right_uns_val);
        return {type, Value{RValue{val}}};
    }
    case Op::Diff: {
        // Unsigned(a) - Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        Unsigned val = (left_uns_val > right_uns_val) ? left_uns_val - right_uns_val : 0;
        return {this, Value{RValue{val}}};
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns_val < right_uns_val)}};
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns_val <= right_uns_val)}};
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns_val > right_uns_val)}};
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value(type->construct(left_uns_val >= right_uns_val))};
    }
    default:
        assert(false);
    }
}

} // namespace ulam
