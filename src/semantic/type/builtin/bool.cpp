#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/utils/integer.hpp>

namespace ulam {

TypedValue BoolType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::MinOf: {
        auto rval = construct(false);
        ;
        rval.set_is_consteval(true);
        return {this, Value{std::move(rval)}};
    }
    case TypeOp::MaxOf: {
        auto rval = construct(true);
        rval.set_is_consteval(true);
        return {this, Value{std::move(rval)}};
    }
    default:
        return _PrimType::type_op(op);
    }
}

RValue BoolType::construct(bool value) {
    return RValue{value ? utils::ones(bitsize()) : 0};
}

bool BoolType::is_true(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    auto uns_val = rval.get<Unsigned>();
    return utils::count_ones(uns_val) >= (bitsize() + 1u) / 2;
}

RValue BoolType::from_datum(Datum datum) {
    RValue rval{(Unsigned)datum};
    return construct(is_true(rval));
}

Datum BoolType::to_datum(const RValue& rval) {
    assert(rval.is<Unsigned>());
    auto rval_upd = construct(is_true(rval));
    auto uns_val = rval_upd.get<Unsigned>();
    return uns_val;
}

TypedValue BoolType::unary_op(Op op, RValue&& rval) {
    if (rval.empty())
        return {this, Value{std::move(rval)}};

    bool is_truth = is_true(rval);
    bool is_consteval = rval.is_consteval();

    switch (op) {
    case Op::Negate: {
        auto rval = construct(!is_truth);
        rval.set_is_consteval(is_consteval);
        return {this, Value{std::move(rval)}};
    }
    default:
        assert(false);
    }
}

TypedValue BoolType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(BoolId));
    assert(l_rval.empty() || l_rval.is<Unsigned>());
    assert(r_rval.empty() || r_rval.is<Unsigned>());

    auto type = builtins().bool_type(std::max(bitsize(), bitsize()));
    bool is_unknown = l_rval.empty() || r_rval.empty();
    if (is_unknown)
        return {type, Value{RValue{}}};

    auto left = is_true(l_rval);
    auto right = builtins().bool_type(r_type->bitsize())->is_true(r_rval);

    switch (op) {
    case Op::Equal:
        return {type, Value{type->construct(left == right)}};
    case Op::NotEqual:
        return {type, Value{type->construct(left != right)}};
    case Op::And:
        return {type, Value{type->construct(left && right)}};
    case Op::Or:
        return {type, Value{type->construct(left || right)}};
    default:
        assert(false);
    }
}

bool BoolType::is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case BoolId:
        return true;
    case BitsId:
        return type->bitsize() >= bitsize();
    default:
        return is_castable_to(type->bi_type_id(), expl);
    }
}

bool BoolType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case IntId:
        return expl;
    case UnsignedId:
        return expl;
    case BoolId:
        return true;
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

TypedValue BoolType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    assert(is_expl_castable_to(id));

    switch (id) {
    case IntId: {
        auto type = builtins().int_type(2);
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    case UnsignedId: {
        auto type = builtins().unsigned_type(1);
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    case UnaryId: {
        auto type = builtins().unary_type(1);
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    case BoolId: {
        assert(false);
        return {this, Value{std::move(rval)}};
    }
    case BitsId: {
        auto type = builtins().bits_type(bitsize());
        return {type, Value{cast_to_prim(type, std::move(rval))}};
    }
    default:
        assert(false);
    }
}

RValue BoolType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    assert(is_expl_castable_to(type));
    if (rval.empty())
        return std::move(rval);

    bool is_consteval = rval.is_consteval();
    bool is_truth = is_true(rval);

    switch (type->bi_type_id()) {
    case IntId: {
        return RValue{(Integer)(is_truth ? 1 : 0), is_consteval};
    }
    case UnsignedId: {
        return RValue{(Unsigned)(is_truth ? 1 : 0), is_consteval};
    }
    case UnaryId: {
        return RValue{(Unsigned)(is_truth ? 1 : 0), is_consteval};
    }
    case BoolId: {
        Unsigned uns_val = is_truth ? utils::ones(type->bitsize()) : 0;
        return RValue{uns_val, is_consteval};
    }
    case BitsId: {
        auto size = std::min(bitsize(), type->bitsize());
        auto boolean = builtins().bool_type(size);
        auto datum = boolean->to_datum(boolean->construct(is_truth));
        auto bits_rval = type->construct();
        bits_rval.get<Bits>().write_right(size, datum);
        bits_rval.set_is_consteval(is_consteval);
        return bits_rval;
    }
    default:
        assert(false);
    }
}

} // namespace ulam
