#include "src/semantic/detail/integer.hpp"
#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

bool IntType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return true;
    case UnsignedId:
        return expl;
    case BoolId:
        return true;
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

bool IntType::is_castable_to(Ref<PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->builtin_type_id()) {
    case IntId:
        return expl || size <= bitsize();
    case UnsignedId:
        return expl || size <= bitsize() + 1;
    case BoolId:
        return true;
    case UnaryId:
        return expl || detail::unary_unsigned_bitsize(size) <= bitsize() + 1;
    case BitsId:
        return expl || size <= bitsize();
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

PrimTypedValue IntType::cast_to(BuiltinTypeId id, Value&& value) {
    assert(is_expl_castable_to(id));
    assert(!value.is_nil());
    assert(value.rvalue()->is<Integer>());
    assert(!value.rvalue()->empty());

    auto rval = value.rvalue();
    auto intval = rval->get<Integer>();
    switch (id) {
    case IntId: {
        assert(false);
        return {this, std::move(value)};
    }
    case UnsignedId: {
        intval = std::max((Integer)0, intval);
        auto size = detail::bitsize(value);
        auto type = builtins().prim_type(UnsignedId, size);
        return {type, RValue{(Unsigned)intval}};
    }
    case BoolId: {
        Unsigned val = (intval == 0) ? 0 : 1;
        auto type = builtins().prim_type(BoolId, 1);
        return {type, RValue{val}};
    }
    case UnaryId: {
        Unsigned val = std::max((Integer)0, intval);
        val = std::min((Unsigned)ULAM_MAX_INT_SIZE, val);
        auto type = builtins().prim_type(UnaryId, value);
        return {type, RValue{val}};
    }
    case BitsId: {
        auto size = detail::bitsize(intval);
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        // TODO: write `intval` bits
        return {type, RValue{std::move(val)}};
    }
    default:
        assert(false);
    }
}

Value IntType::cast_to(Ref<PrimType> type, Value&& value) {
    assert(is_expl_castable_to(type));
    assert(!value.is_nil());
    assert(value.rvalue()->is<Integer>());
    assert(!value.rvalue()->empty());

    auto rval = value.rvalue();
    Integer intval = rval->get<Integer>();
    switch (type->builtin_type_id()) {
    case IntId: {
        assert(false);
        return std::move(value);
    }
    case UnsignedId: {
        Unsigned val = std::max((Integer)0, intval);
        val = std::min(detail::unsigned_max(type->bitsize()), val);
        return RValue{val};
    }
    case BoolId: {
        Unsigned val =
            (intval == 0) ? (Unsigned)0 : detail::unsigned_max(type->bitsize());
        return RValue{val};
    }
    case UnaryId: {
        Unsigned val = std::max((Integer)0, intval);
        val = std::min((Unsigned)type->bitsize(), val);
        return RValue{val};
    }
    default:
        assert(false);
    }
}

PrimTypedValue IntType::binary_op(
    Op op,
    const Value& left_val,
    Ref<const PrimType> right_type,
    const Value& right_val) {
    assert(right_type->is(IntId));

    auto left_rval = left_val.rvalue();
    auto right_rval = right_val.rvalue();
    assert(left_rval->empty() || left_rval->is<Integer>());
    assert(right_rval->empty() || right_rval->is<Integer>());;

    bool is_unknown = left_rval->empty() || right_rval->empty();
    Integer left_intval = left_rval->empty() ? 0 : left_rval->get<Integer>();
    Integer right_intval = right_rval->empty() ? 0 : right_rval->get<Integer>();

    switch (op) {
    case Op::Prod: {
        // Int(a) * Int(b) = Int(a + b)
        auto size = std::min(MaxSize, (bitsize_t)(bitsize() + right_type->bitsize()));
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, RValue{}};
        auto [val, _] = detail::safe_prod(left_intval, right_intval);
        return {type, RValue{detail::truncate(val, size)}};
    }
    case Op::Quot: {
        // Int(a) / Int(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return {this, RValue{}};
        auto val = detail::safe_quot(left_intval, right_intval);
        return {this, RValue{val}};
    }
    case Op::Rem: {
        // Int(a) % Int(b) = Int(a)
        if (is_unknown)
            return {this, RValue{}};
        auto val = detail::safe_rem(left_intval, right_intval);
        return {this, RValue{val}};
    }
    case Op::Sum: {
        // Int(a) + Int(b) + Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, RValue{}};
        auto [val, _] = detail::safe_sum(left_intval, right_intval);
        return {type, RValue{val}};
    }
    case Op::Diff: {
        // Int(a) - Int(b) = Int(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), right_type->bitsize()) + 1;
        size = std::min(size, MaxSize);
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, RValue{}};
        auto [val, _] = detail::safe_diff(left_intval, right_intval);
        return {type, RValue{val}};
    }
    default:
        assert(false);
    }
}

} // namespace ulam
