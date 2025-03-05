#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam {

TypedValue UnsignedType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{RValue{(Unsigned)0, true}}};
    case TypeOp::MaxOf:
        return {this, Value{RValue{detail::unsigned_max(bitsize()), true}}};
    default:
        return _PrimType::type_op(op);
    }
}

RValue UnsignedType::construct() const { return RValue{Unsigned{}}; }

RValue UnsignedType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum UnsignedType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

TypedValue UnsignedType::unary_op(Op op, RValue&& rval) {
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

TypedValue UnsignedType::binary_op(
    Op op,
    RValue&& left_rval,
    Ref<const PrimType> right_type,
    RValue&& right_rval) {
    assert(right_type->is(UnsignedId));
    assert(left_rval.empty() || left_rval.is<Unsigned>());
    assert(right_rval.empty() || right_rval.is<Unsigned>());

    bool is_unknown = left_rval.empty() || right_rval.empty();
    Unsigned left_uns = left_rval.empty() ? 0 : left_rval.get<Unsigned>();
    Unsigned right_uns = right_rval.empty() ? 0 : right_rval.get<Unsigned>();

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns == right_uns)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns != right_uns)}};
    }
    case Op::AssignProd: {
        // (Unsigned(a) *= Unsigned(b)) = Unsigned(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto [val, _] = detail::safe_prod(left_uns, right_uns);
        return {this, Value{RValue{detail::truncate(val, bitsize())}}};
    }
    case Op::Prod: {
        // Unsigned(a) * Unsigned(b) = Unsigned(a + b)
        auto size =
            std::min<bitsize_t>(MaxSize, bitsize() + right_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_prod(left_uns, right_uns);
        return {this, Value{RValue{val}}};
    }
    case Op::AssignQuot:
    case Op::Quot: {
        // Unsigned(a) / Unsigned(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_quot(left_uns, right_uns);
        return {this, Value{RValue{val}}};
    }
    case Op::AssignRem:
    case Op::Rem: {
        // Unsigned(a) % Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_rem(left_uns, right_uns);
        return {this, Value{RValue{val}}};
    }
    case Op::AssignSum: {
        // (Unsigned(a) += Unsigned(b)) = Unsigned(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto [val, _] = detail::safe_sum(left_uns, right_uns);
        return {this, Value{RValue{detail::truncate(val, bitsize())}}};
    }
    case Op::Sum: {
        // Unsigned(a) + Unsigned(b) = Unsigned(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_sum(left_uns, right_uns);
        return {type, Value{RValue{val}}};
    }
    case Op::AssignDiff:
    case Op::Diff: {
        // Unsigned(a) - Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        Unsigned val = (left_uns > right_uns) ? left_uns - right_uns : 0;
        return {this, Value{RValue{val}}};
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns < right_uns)}};
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns <= right_uns)}};
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_uns > right_uns)}};
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value(type->construct(left_uns >= right_uns))};
    }
    default:
        assert(false);
    }
}

bool UnsignedType::is_castable_to_prim(
    Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case IntId:
        return expl || type->bitsize() == ULAM_MAX_INT_SIZE ||
               type->bitsize() > bitsize();
    case UnsignedId:
        return expl || type->bitsize() >= bitsize();
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return expl || detail::unsigned_max(bitsize()) <= type->bitsize();
    case BitsId:
        return expl || type->bitsize() >= bitsize();
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

bool UnsignedType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
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
        assert(false);
    }
}

bool UnsignedType::is_impl_castable_to_prim(
    Ref<const PrimType> type, const Value& val) const {
    auto rval = val.copy_rvalue();
    if (rval.empty() || !rval.is_consteval())
        return is_castable_to_prim(type, false);

    assert(rval.is<Unsigned>());
    auto uns_val = rval.get<Unsigned>();

    switch (type->bi_type_id()) {
    case UnsignedId:
        return detail::bitsize(uns_val) <= type->bitsize();
    case UnaryId:
        return uns_val <= type->bitsize();
    default:
        return is_castable_to_prim(type, false);
    }
}

TypedValue UnsignedType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
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
    case BoolId: {
        assert(bitsize() == 1);
        auto boolean = builtins().boolean();
        return {boolean, Value{boolean->construct(uns_val > 0)}};
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
        store(val.view(), 0, rval);
        return {type, Value{RValue{std::move(val)}}};
    }
    default:
        assert(false);
    }
}

RValue UnsignedType::cast_to_prim(Ref<const PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = rval.get<Unsigned>();
    switch (type->bi_type_id()) {
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
    case BoolId: {
        assert(bitsize() == 1);
        return builtins().bool_type(type->bitsize())->construct();
    }
    case UnaryId: {
        Unsigned val =
            std::min((Unsigned)type->bitsize(), detail::ones(uns_val));
        return RValue{val};
    }
    case BitsId: {
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().write_right(type->bitsize(), to_datum(rval));
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam
