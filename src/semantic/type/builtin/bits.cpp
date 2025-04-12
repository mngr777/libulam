#include <algorithm>
#include <functional>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtins.hpp>

namespace ulam {

RValue BitsType::load(const BitsView data, bitsize_t off) {
    Bits val{data.view(off, bitsize()).copy()};
    return RValue{std::move(val)};
}

void BitsType::store(BitsView data, bitsize_t off, const RValue& rval) {
    assert(rval.is<Bits>());
    data.write(off, rval.get<Bits>().view());
}

RValue BitsType::construct() { return RValue{Bits{bitsize()}}; }

RValue BitsType::construct(Bits&& bits) {
    assert(bits.len());
    return RValue{std::move(bits)};
}

bool BitsType::is_castable_to(Ref<const Type> type, bool expl) const {
    if (type->is_class())
        return expl && type->bitsize();
    return _PrimType::is_castable_to(type, expl);
}

Value BitsType::cast_to(Ref<Type> type, Value&& val) {
    // class?
    if (type->is_class()) {
        assert(type->bitsize() == bitsize());
        auto cls = type->as_class();
        if (val.empty())
            return Value{cls->construct()};

        auto rval = val.move_rvalue();
        assert(rval.is<Bits>());
        bool is_consteval = rval.is_consteval();
        auto& bits = rval.get<Bits>();
        auto new_rval = cls->construct(std::move(bits));
        new_rval.set_is_consteval(is_consteval);
        return Value{std::move(new_rval)};
    }

    // atom?
    if (type->is(AtomId)) {
        assert(bitsize() == ULAM_ATOM_SIZE);
        if (val.empty())
            return Value{type->construct()};

        auto rval = val.move_rvalue();
        assert(rval.is<Bits>());
        bool is_consteval = rval.is_consteval();
        auto& bits = rval.get<Bits>();
        auto new_rval = builtins().atom_type()->construct(std::move(bits));
        new_rval.set_is_consteval(is_consteval);
        return Value{std::move(new_rval)};
    }

    return _PrimType::cast_to(type, std::move(val));
}

conv_cost_t BitsType::conv_cost(Ref<const Type> type, bool allow_cast) const {
    if (type->is_class()) {
        return (allow_cast && type->bitsize() == bitsize())
                   ? BitsToClassConvCost
                   : MaxConvCost;
    }
    if (type->is(AtomId)) {
        return (bitsize() == ULAM_ATOM_SIZE) ? BitsToAtomConvCost : MaxConvCost;
    }
    return _PrimType::conv_cost(type, allow_cast);
}

TypedValue BitsType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{std::move(rval)}};

    switch (op) {
    case Op::BwNot: {
        assert(rval.is<Bits>());
        auto& bits = rval.get<Bits>();
        bits.flip();
        return {this, Value{std::move(rval)}};
    }
    default:
        assert(false);
    }
}

TypedValue BitsType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(BitsId) || r_type->is(UnsignedId));
    assert(l_rval.empty() || l_rval.is<Bits>());
    bool is_unknown = l_rval.empty() || r_rval.empty();

    // &, |, ^
    using BinOp = std::function<Bits(const BitsView, const BitsView)>;
    auto binop = [&](BinOp op, bool assign) -> TypedValue {
        auto type = this;
        if (assign)
            tpl()->type(std::max(bitsize(), r_type->bitsize()));
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto& l_bits = l_rval.get<Bits>();
        auto& r_bits = r_rval.get<Bits>();
        auto l_view = l_bits.view();
        auto r_view = (assign && bitsize() < r_type->bitsize())
                          ? r_bits.view_right(bitsize())
                          : r_bits.view();
        Bits bits{op(l_view, r_view)};
        return {type, Value{RValue{std::move(bits)}}};
    };

    auto bw_and = [](auto v1, auto v2) { return v1 & v2; };
    auto bw_or = [](auto v1, auto v2) { return v1 | v2; };
    auto bw_xor = [](auto v1, auto v2) { return v1 ^ v2; };

    switch (op) {
    case Op::Equal:
    case Op::NotEqual: {
        auto boolean = builtins().boolean();
        if (is_unknown)
            return {boolean, Value{RValue{}}};
        auto& l_bits = l_rval.get<Bits>();
        auto& r_bits = l_rval.get<Bits>();
        bool is_equal = (l_bits == r_bits);
        bool val = (is_equal == (op == Op::Equal));
        return {boolean, Value{boolean->construct(val)}};
    }
    case Op::AssignShiftLeft: {
        Unsigned shift = r_rval.get<Unsigned>();
        Bits bits{l_rval.get<Bits>() << shift};
        return {this, Value{RValue{std::move(bits)}}};
    }
    case Op::ShiftLeft: {
        Unsigned shift = r_rval.get<Unsigned>();
        if (shift == 0)
            return {this, Value{std::move(l_rval)}};
        bitsize_t max_size = 32;
        if (bitsize() > max_size)
            max_size = (bitsize() > 64) ? MaxSize : 64;
        bitsize_t size = std::min<bitsize_t>(bitsize() + shift, max_size);
        auto type = builtins().bits_type(size);
        auto rval = type->construct();
        auto& bits = rval.get<Bits>();
        bits |= l_rval.get<Bits>();
        bits <<= shift;
        return {type, Value{std::move(rval)}};
    }
    case Op::AssignShiftRight:
    case Op::ShiftRight: {
        Unsigned shift = r_rval.get<Unsigned>();
        Bits bits{l_rval.get<Bits>() >> shift};
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
    switch (type->bi_type_id()) {
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
        return expl && bitsize() == ULAM_ATOM_SIZE;
    case StringId:
        return false;
    case FunId:
    case VoidId:
    default:
        assert(false);
    }
}

bool BitsType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
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
        return expl && bitsize() == ULAM_ATOM_SIZE;
    case StringId:
        return false;
    case FunId:
    case VoidId:
    default:
        assert(false);
    }
}

TypedValue BitsType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    assert(false && "Bits is not implicitly castable to other types");
}

RValue BitsType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    if (rval.empty())
        return std::move(rval);

    bool is_consteval = rval.is_consteval();
    auto& bits = rval.get<Bits>();

    switch (type->bi_type_id()) {
    case IntId:
    case UnsignedId:
    case BoolId:
    case UnaryId: {
        // TODO: this is probably not how it works, to be caught by ULAM tests
        auto datum = bits.read_right(std::min(bitsize(), type->bitsize()));
        rval = type->from_datum(datum);
        rval.set_is_consteval(is_consteval);
        return std::move(rval);
    }
    case BitsId: {
        Bits copy{type->bitsize()};
        copy |= bits;
        return RValue{std::move(copy), is_consteval};
    }
    default:
        assert(false);
    }
}

} // namespace ulam
