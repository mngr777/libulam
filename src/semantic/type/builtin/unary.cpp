#include "src/semantic/detail/integer.hpp"
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

RValue UnaryType::from_datum(Datum datum) const {
    return RValue{(Unsigned)datum};
}

Datum UnaryType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

TypedValue UnaryType::cast_to(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));
    assert(!rval.empty());
    assert(rval.is<Unsigned>());

    auto uns_val = detail::count_ones(rval.get<Unsigned>());
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
        auto size = detail::bitsize(uns_val);
        auto type = builtins().prim_type(UnsignedId, size);
        return {type, Value{RValue{uns_val}}};
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto boolean = builtins().boolean();
        return {boolean, Value{boolean->construct(uns_val > 0)}};
    }
    case UnaryId: {
        assert(false);
        return {this, Value{std::move(rval)}};
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

RValue UnaryType::cast_to(Ref<const PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = detail::count_ones(rval.get<Unsigned>());
    switch (type->bi_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val};
    }
    case UnsignedId: {
        uns_val = std::min(detail::unsigned_max(type->bitsize()), uns_val);
        return RValue{uns_val};
    }
    case BoolId: {
        assert(bitsize() == 1);
        return builtins().boolean(type->bitsize())->construct(uns_val > 0);
    }
    case UnaryId: {
        uns_val = std::min<Unsigned>(type->bitsize(), uns_val);
        return RValue{detail::ones(uns_val)};
    }
    case BitsId: {
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().bits().write_right(
            type->bitsize(), to_datum(rval));
        return bits_rval;
    }
    default:
        assert(false);
    }
}

TypedValue UnaryType::unary_op(Op op, RValue&& rval) {
    assert(false); // not implemented
}

TypedValue UnaryType::binary_op(
    Op op,
    RValue&& left_rval,
    Ref<const PrimType> right_type,
    RValue&& right_rval) {
    assert(right_type->is(UnaryId));
    assert(left_rval.empty() || left_rval.is<Unsigned>());
    assert(right_rval.empty() || right_rval.is<Unsigned>());

    bool is_unknown = left_rval.empty() || right_rval.empty();

    Unsigned left_uns =
        left_rval.empty() ? 0 : detail::count_ones(left_rval.get<Unsigned>());
    Unsigned right_uns =
        right_rval.empty() ? 0 : detail::count_ones(right_rval.get<Unsigned>());

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
    default:
        assert(false);
    }
}

bool UnaryType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    auto size = type->bitsize();
    switch (type->bi_type_id()) {
    case IntId:
        return expl || detail::integer_max(size) >= bitsize();
    case UnsignedId:
        return expl || detail::unsigned_max(size) >= bitsize();
    case BoolId:
        return bitsize() == 1;
    case UnaryId:
        return expl || size >= bitsize();
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
        assert(false);
    }
}

} // namespace ulam
