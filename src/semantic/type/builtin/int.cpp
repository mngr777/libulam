#include "libulam/semantic/type/builtin_type_id.hpp"
#include "libulam/semantic/value/types.hpp"
#include "src/semantic/detail/integer.hpp"
#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

TypedValue IntType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf:
        return {this, Value{RValue{detail::integer_min(bitsize())}}};
    case TypeOp::MaxOf:
        return {this, Value{RValue{detail::integer_max(bitsize())}}};
    default:
        return _PrimType::type_op(op);
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

TypedValue IntType::cast_to(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));
    assert(!rval.empty());
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

RValue IntType::cast_to(Ref<const PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Integer>());

    auto int_val = rval.get<Integer>();
    switch (type->bi_type_id()) {
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
    case BitsId: {
        auto bits_rval = type->construct();
        // TODO: adjust value?
        bits_rval.get<Bits>().bits().write_right(
            type->bitsize(), to_datum(rval));
        return bits_rval;
    }
    default:
        assert(false);
    }
}

TypedValue IntType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{RValue{}}};

    auto int_val = rval.get<Integer>();
    switch (op) {
    case Op::UnaryMinus:
        int_val = (int_val == detail::min<Integer>()) ? detail::max<Integer>()
                                                      : -int_val;
        break;
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        if (int_val < detail::integer_max(bitsize()))
            ++int_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        if (int_val > detail::integer_min(bitsize()))
            --int_val;
        break;
    default:
        assert(false);
    }
    return {this, Value{RValue{int_val}}};
}

TypedValue IntType::binary_op(
    Op op,
    RValue&& left_rval,
    Ref<const PrimType> right_type,
    RValue&& right_rval) {
    assert(right_type->is(IntId));
    assert(left_rval.empty() || left_rval.is<Integer>());
    assert(right_rval.empty() || right_rval.is<Integer>());

    bool is_unknown = left_rval.empty() || right_rval.empty();
    Integer left_int = left_rval.empty() ? 0 : left_rval.get<Integer>();
    Integer right_int = right_rval.empty() ? 0 : right_rval.get<Integer>();

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        return {type, Value{type->construct(left_int == right_int)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        return {type, Value{type->construct(left_int != right_int)}};
    }
    case Op::AssignProd: {
        // (Int(a) += Int(b)) == Int(a)
        auto [val, _] = detail::safe_prod(left_int, right_int);
        return {this, Value{RValue{detail::truncate(val, bitsize())}}};
    }
    case Op::Prod: {
        // Int(a) * Int(b) = Int(a + b)
        auto size =
            std::min<bitsize_t>(MaxSize, bitsize() + right_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_prod(left_int, right_int);
        return {type, Value{RValue{detail::truncate(val, size)}}};
    }
    case Op::AssignQuot:
    case Op::Quot: {
        // Int(a) / Int(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_quot(left_int, right_int);
        return {this, Value{RValue{val}}};
    }
    case Op::AssignRem:
    case Op::Rem: {
        // Int(a) % Int(b) = Int(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto val = detail::safe_rem(left_int, right_int);
        return {this, Value{RValue{val}}};
    }
    case Op::AssignSum: {
        // (Int(a) += Int(b)) = Int(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto [val, _] = detail::safe_sum(left_int, right_int);
        return {this, Value{RValue{detail::truncate(val, bitsize())}}};
    }
    case Op::Sum: {
        // Int(a) + Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_sum(left_int, right_int);
        return {type, Value{RValue{val}}};
    }
    case Op::AssignDiff: {
        // (Int(a) -= Int(b)) = Int(a)
        if (is_unknown)
            return {this, Value{RValue{}}};
        auto [val, _] = detail::safe_diff(left_int, right_int);
        return {this, Value{RValue{detail::truncate(val, bitsize())}}};
    }
    case Op::Diff: {
        // Int(a) - Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto [val, _] = detail::safe_diff(left_int, right_int);
        return {type, Value{RValue{val}}};
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_int < right_int)}};
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_int <= right_int)}};
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value{type->construct(left_int > right_int)}};
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        return {type, Value(type->construct(left_int >= right_int))};
    }
    default:
        assert(false);
    }
}

bool IntType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case IntId:
        return expl || type->bitsize() >= bitsize();
    case UnsignedId:
        return expl;
    case BoolId:
        return false;
    case UnaryId:
        return expl;
    case BitsId:
        return expl;
    case AtomId:
        return false;
        return false;
    case FunId:
    case VoidId:
    default:
        assert(false);
    }
}

bool IntType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
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

bool IntType::is_impl_castable_to_prim(
    Ref<const PrimType> type, const Value& val) const {
    if (!type->is(UnsignedId) && !type->is(UnaryId) && !type->is(BitsId))
        return is_castable_to_prim(type, false);

    auto rval = val.copy_rvalue();
    if (rval.empty() || !rval.is_consteval())
        return is_castable_to_prim(type, false);

    assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    if (int_val < 0)
        return false;

    auto uns_val = (Unsigned)int_val;
    if (type->is(UnaryId))
        return type->bitsize() >= uns_val;
    return type->bitsize() >= detail::bitsize(uns_val);
}

bool IntType::is_impl_castable_to_prim(
    BuiltinTypeId bi_type_id, const Value& val) const {
    if (bi_type_id != UnsignedId && bi_type_id != UnaryId && bi_type_id != BitsId)
        return is_castable_to_prim(bi_type_id, false);

    auto rval = val.copy_rvalue();
    if (rval.empty() || !rval.is_consteval())
        return is_castable_to_prim(bi_type_id, false);

    assert(rval.is<Integer>());
    auto int_val = rval.get<Integer>();
    return int_val >= 0;
}

} // namespace ulam
