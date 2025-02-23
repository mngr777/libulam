#include <algorithm>
#include <functional>
#include <libulam/semantic/type/builtin/bits.hpp>

namespace ulam {

RValue BitsType::load(const BitVectorView data, BitVector::size_t off) const {
    Bits val{data.view(off, bitsize()).copy()};
    return RValue{std::move(val)};
}

void BitsType::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {
    assert(off + bitsize() <= data.len());
    assert(rval.is<Bits>());
    data.write(off, rval.get<Bits>().bits().view());
}

bool BitsType::is_castable_to(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl;
    case BoolId:
        return expl;
    case UnaryId:
        return expl;
    case BitsId:
        return true;
    case AtomId:
        assert(false); // TMP
        return expl;
    case StringId:
        assert(false); // TMP
        return expl;
    case FunId:
    case VoidId:
    default:
        assert(false);
    }
}

TypedValue BitsType::cast_to(BuiltinTypeId id, RValue&& rval) {
    assert(false && "Bits is not implicitly castable to other types");
}

RValue BitsType::cast_to(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Bits>());

    auto& bits = rval.get<Bits>();
    switch (type->builtin_type_id()) {
    case IntId:
    case UnsignedId:
    case BoolId:
    case UnaryId: {
        // TODO: this is probably not how it works, to be caught by ULAM tests
        auto datum =
            bits.bits().read_right(std::min(bitsize(), type->bitsize()));
        return RValue{type->from_datum(datum)};
    }
    case BitsId: {
        Bits copy{type->bitsize()};
        copy.bits() |= bits.bits();
        return RValue{std::move(copy)};
    }
    default:
        assert(false);
    }
}

TypedValue BitsType::binary_op(
    Op op,
    RValue&& left_rval,
    Ref<const PrimType> right_type,
    RValue&& right_rval) {
    assert(right_type->is(BitsId) || right_type->is(UnsignedId));
    assert(left_rval.empty() || left_rval.is<Bits>());
    bool is_unknown = left_rval.empty() || right_rval.empty();

    // &, |, ^
    using BinOp =
        std::function<BitVector(const BitVectorView, const BitVectorView)>;
    auto binop = [&](BinOp op, bool assign) -> TypedValue {
        auto type = this;
        if (assign)
            tpl()->type(std::max(bitsize(), right_type->bitsize()));
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto& left_bits = left_rval.get<Bits>();
        auto& right_bits = right_rval.get<Bits>();
        auto left_view = left_bits.bits().view();
        auto right_view = (assign && bitsize() < right_type->bitsize())
                              ? right_bits.bits().view_right(bitsize())
                              : right_bits.bits().view();
        Bits bits{op(left_view, right_view)};
        return {type, Value{RValue{std::move(bits)}}};
    };

    auto bw_and = [](auto v1, auto v2) { return v1 | v2; };
    auto bw_or = [](auto v1, auto v2) { return v1 | v2; };
    auto bw_xor = [](auto v1, auto v2) { return v1 ^ v2; };

    switch (op) {
    case Op::AssignShiftLeft:
    case Op::ShiftLeft: {
        Unsigned shift = right_rval.get<Unsigned>();
        Bits bits{left_rval.get<Bits>().bits() << shift};
        return {this, Value{RValue{std::move(bits)}}};
    }
    case Op::AssignShiftRight:
    case Op::ShiftRight: {
        Unsigned shift = right_rval.get<Unsigned>();
        Bits bits{left_rval.get<Bits>().bits() >> shift};
        return {this, Value{RValue{std::move(bits)}}};
    }
    case Op::AssignBwAnd:
        return binop(bw_and, true);
    case Op::BwAnd:
        return binop(bw_and, false);
    case Op::AssignBwOr:
        return binop(bw_or, true);
    case Op::BwOr:
        return binop(bw_or, false);
    case Op::AssignBwXor:
        return binop(bw_xor, true);
    case Op::BwXor:
        return binop(bw_xor, false);
    default:
        assert(false);
    }
}

bool BitsType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    switch (type->builtin_type_id()) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl;
    case BoolId:
        return expl;
    case UnaryId:
        return expl;
    case BitsId:
        return expl || type->bitsize() >= bitsize();
    case AtomId:
        assert(false); // TMP
        return expl;
    case StringId:
        assert(false); // TMP
        return expl;
    case FunId:
    case VoidId:
    default:
        assert(false);
    }
}

} // namespace ulam
