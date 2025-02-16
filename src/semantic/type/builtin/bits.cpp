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

bool BitsType::is_castable_to(Ref<PrimType> type, bool expl) const {
    switch (type->builtin_type_id()) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl;
    case BoolId:
        return expl;
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

PrimTypedValue BitsType::cast_to(BuiltinTypeId id, Value&& value) {
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

PrimTypedValue BitsType::binary_op(
    Op op,
    Value&& left_val,
    Ref<const PrimType> right_type,
    Value&& right_val) {
    assert(right_type->is(BitsId));

    // TODO: avoid copying?
    auto left_rval = left_val.move_rvalue();
    auto right_rval = right_val.move_rvalue();
    assert(left_rval.empty() || left_rval.is<Bits>());
    bool is_unknown = left_rval.empty() || right_rval.empty();

    // &, |, ^
    using BinOp = std::function<BitVector(const BitVector&, const BitVector&)>;
    auto binop = [&](BinOp op) -> PrimTypedValue {
        auto size = std::max(bitsize(), right_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto& left_bits = left_rval.get<Bits>();
        auto& right_bits = right_rval.get<Bits>();
        Bits bits{op(left_bits.bits(), right_bits.bits())};
        return {type, Value{RValue{std::move(bits)}}};
    };

    // TODO: shift

    switch (op) {
    case Op::ShiftLeft:
        assert(false);
    case Op::ShiftRight:
        assert(false);
    case Op::BwAnd:
        return binop(std::bit_and<BitVector>{});
    case Op::BwOr:
        return binop(std::bit_or<BitVector>{});
    case Op::BwXor:
        return binop(std::bit_xor<BitVector>{});
    default:
        assert(false);
    }
}

} // namespace ulam
