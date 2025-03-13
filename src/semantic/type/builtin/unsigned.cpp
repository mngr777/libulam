#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
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

    bool is_consteval = rval.is_consteval();
    auto uns_val = rval.get<Unsigned>();
    switch (op) {
    case Op::UnaryMinus: {
        bitsize_t max_size = (bitsize() > DefaultSize) ? ULAM_MAX_INT_SIZE
                                                       : IntType::DefaultSize;
        bitsize_t size = std::min<bitsize_t>(bitsize() + 1, max_size);
        Integer int_val =
            std::max<Integer>(detail::integer_min(size), -uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, Value{RValue{int_val, is_consteval}}};
    }
    case Op::UnaryPlus:
        break;
    case Op::PreInc:
    case Op::PostInc:
        is_consteval = false;
        if (uns_val < detail::unsigned_max(bitsize()))
            ++uns_val;
        break;
    case Op::PreDec:
    case Op::PostDec:
        is_consteval = false;
        if (uns_val > 0)
            --uns_val;
        break;
    default:
        assert(false);
    }
    return {this, Value{RValue{uns_val, is_consteval}}};
}

TypedValue UnsignedType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(UnsignedId));
    assert(l_rval.empty() || l_rval.is<Unsigned>());
    assert(r_rval.empty() || r_rval.is<Unsigned>());

    bool is_unknown = l_rval.empty() || r_rval.empty();
    Unsigned l_uns = l_rval.empty() ? 0 : l_rval.get<Unsigned>();
    Unsigned r_uns = r_rval.empty() ? 0 : r_rval.get<Unsigned>();

    bool is_wider =
        (bitsize() > DefaultSize || r_type->bitsize() > DefaultSize);
    bitsize_t max_size = is_wider ? MaxSize : DefaultSize;

    bool is_consteval = !ops::is_assign(op) && !is_unknown &&
                        l_rval.is_consteval() && r_rval.is_consteval();

    auto make_res = [&](Ref<Type> type, RValue&& rval) -> TypedValue {
        rval.set_is_consteval(is_consteval);
        return {type, Value{std::move(rval)}};
    };
    auto make_empty = [&](Ref<Type> type) -> TypedValue {
        return {type, Value{RValue{}}};
    };

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns == r_uns));
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns != r_uns));
    }
    case Op::AssignProd: {
        // (Unsigned(a) *= Unsigned(b)) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        auto [val, _] = detail::safe_prod(l_uns, r_uns);
        return make_res(this, RValue{detail::truncate(val, bitsize())});
    }
    case Op::Prod: {
        // Unsigned(a) * Unsigned(b) = Unsigned(a + b)
        auto size =
            std::min<bitsize_t>(max_size, bitsize() + r_type->bitsize());
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = detail::safe_prod(l_uns, r_uns);
        return make_res(type, RValue{val});
    }
    case Op::AssignQuot:
    case Op::Quot: {
        // Unsigned(a) / Unsigned(b) = Int(a) NOTE: does not match
        // ULAM's max(a, b), TODO: investigate
        if (is_unknown)
            return make_empty(this);
        auto val = detail::safe_quot(l_uns, r_uns);
        return make_res(this, RValue{val});
    }
    case Op::AssignRem:
    case Op::Rem: {
        // Unsigned(a) % Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        return make_res(this, RValue{detail::safe_rem(l_uns, r_uns)});
    }
    case Op::AssignSum: {
        // (Unsigned(a) += Unsigned(b)) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        auto [val, _] = detail::safe_sum(l_uns, r_uns);
        return make_res(this, RValue{detail::truncate(val, bitsize())});
    }
    case Op::Sum: {
        // Unsigned(a) + Unsigned(b) = Unsigned(max(a, b) + 1)
        bitsize_t size = std::max(bitsize(), r_type->bitsize()) + 1;
        size = std::min(size, max_size);
        auto type = tpl()->type(size);
        if (is_unknown)
            return make_empty(type);
        auto [val, _] = detail::safe_sum(l_uns, r_uns);
        return make_res(type, RValue{val});
    }
    case Op::AssignDiff:
    case Op::Diff: {
        // Unsigned(a) - Unsigned(b) = Unsigned(a)
        if (is_unknown)
            return make_empty(this);
        Unsigned val = (l_uns > r_uns) ? l_uns - r_uns : 0;
        return make_res(this, RValue{val});
    }
    case Op::Less: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns < r_uns));
    }
    case Op::LessOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns <= r_uns));
    }
    case Op::Greater: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns > r_uns));
    }
    case Op::GreaterOrEq: {
        auto type = builtins().boolean();
        if (is_unknown)
            return make_empty(type);
        return make_res(type, type->construct(l_uns >= r_uns));
    }
    default:
        assert(false);
    }
}

bool UnsignedType::is_castable_to_prim(
    Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case IntId: {
        bool is_wider =
            (bitsize() > DefaultSize || type->bitsize() > IntType::DefaultSize);
        bitsize_t max_size = is_wider ? IntType::MaxSize : IntType::DefaultSize;
        return expl || type->bitsize() == max_size ||
               type->bitsize() > bitsize();
    }
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
    case IntId:
        if (uns_val == 0)
            return true;
        return detail::bitsize(uns_val) <= detail::integer_max(type->bitsize());
    case UnsignedId:
        if (uns_val == 0)
            return true;
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
    bool is_consteval = rval.is_consteval();
    switch (id) {
    case IntId: {
        auto size = std::min(
            (bitsize_t)ULAM_MAX_INT_SIZE,
            (bitsize_t)(detail::bitsize(uns_val) + 1));
        Integer val = std::min((Unsigned)detail::integer_max(size), uns_val);
        auto type = builtins().prim_type(IntId, size);
        return {type, Value{RValue{val, is_consteval}}};
    }
    case UnsignedId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto boolean = builtins().boolean();
        auto rval = boolean->construct(uns_val > 0);
        rval.set_is_consteval(is_consteval);
        return {boolean, Value{std::move(rval)}};
    }
    case UnaryId: {
        Unsigned val = std::min((Unsigned)UnaryType::DefaultSize, uns_val);
        auto type = builtins().prim_type(UnaryId, detail::ones(uns_val));
        return {type, Value{RValue{val, is_consteval}}};
    }
    case BitsId: {
        auto size = bitsize();
        auto type = builtins().prim_type(BitsId, size);
        Bits val{size};
        store(val.view(), 0, rval);
        return {type, Value{RValue{std::move(val), is_consteval}}};
    }
    default:
        assert(false);
    }
}

RValue UnsignedType::cast_to_prim(Ref<const PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    assert(rval.is<Unsigned>());

    Unsigned uns_val = rval.get<Unsigned>();
    bool is_consteval = rval.is_consteval();
    switch (type->bi_type_id()) {
    case IntId: {
        Unsigned int_max = detail::integer_max(type->bitsize());
        Integer val = std::min(int_max, uns_val);
        return RValue{val, is_consteval};
    }
    case UnsignedId: {
        auto uns_max = detail::unsigned_max(type->bitsize());
        if (uns_val > uns_max)
            return RValue{uns_max, is_consteval};
        return std::move(rval);
    }
    case BoolId: {
        assert(bitsize() == 1);
        auto rval = builtins().bool_type(type->bitsize())->construct();
        rval.set_is_consteval(is_consteval);
        return rval;
    }
    case UnaryId: {
        Unsigned val =
            std::min((Unsigned)type->bitsize(), detail::ones(uns_val));
        return RValue{val, is_consteval};
    }
    case BitsId: {
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().write_right(type->bitsize(), to_datum(rval));
        bits_rval.set_is_consteval(is_consteval);
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam
