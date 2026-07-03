#include "libulam/semantic/value/flags.hpp"
#include <libulam/assert.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

bitsize_t StringType::bitsize() const {
    // TODO: compiler context option
    // return sizeof(String::id) * 8;
    return 18;
}

TypedValue StringType::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::LengthOf:
        return Type::type_op(TypeOp::SizeOf);
    default:
        return Type::type_op(op);
    }
}

TypedValue StringType::type_op(TypeOp op, Value& val) {
    switch (op) {
    case TypeOp::LengthOf: {
        auto type = builtins().unsigned_type();
        if (!val.has_rvalue())
            return {type, Value::make_r_ph()};
        str_len_t len_ = len(val);
        ulam_assert(len_ != NoStrLen);
        auto rval = type->construct((Unsigned)len_, value::IsConsteval);
        return {type, Value{std::move(rval)}};
    }
    default:
        return Type::type_op(op, val);
    }
}

RValue StringType::construct_default(value::flags_t rval_flags) {
    ulam_assert(_text_pool.has(""));
    return RValue::make(String{_text_pool.id("")}, rval_flags);
}

RValue StringType::from_datum(Datum datum) {
    return RValue::make(String{(str_id_t)datum});
}

Datum StringType::to_datum(const RValue& rval) {
    ulam_assert(rval.is<String>());
    Datum datum = rval.get<String>().id;
    return datum;
}

str_len_t StringType::len(const Value& val) const {
    if (!val.has_rvalue())
        return NoStrLen;
    return text(val).size();
}

std::uint8_t StringType::chr(const Value& val, array_idx_t idx) const {
    if (!val.has_rvalue())
        return '\0';
    ulam_assert(idx < len(val));
    return text(val)[idx];
}

std::string_view StringType::text(const Value& val) const {
    if (!val.has_rvalue())
        return ""; // ??
    RValue rval = val.copy_rvalue();
    auto str = rval.get<String>();
    ulam_assert(str.is_valid());
    return _text_pool.get(str.id);
}

TypedValue StringType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    ulam_assert(r_type->is(StringId));
    ulam_assert(!l_rval.has_rvalue() || l_rval.get<String>().is_valid());
    ulam_assert(!r_rval.has_rvalue() || r_rval.get<String>().is_valid());

    bool is_unknown = !l_rval.has_rvalue() || !r_rval.has_rvalue();
    String l_str = l_rval.has_rvalue() ? l_rval.get<String>() : String{};
    String r_str = r_rval.has_rvalue() ? r_rval.get<String>() : String{};

    bool is_consteval =
        !is_unknown && l_rval.is_consteval() && r_rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_str == r_str, rval_flags)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        return {type, Value{type->construct(l_str != r_str, rval_flags)}};
    }
    default:
        unreachable();
    }
}

bool StringType::is_castable_to_prim(
    Ref<const PrimType> type, bool expl) const {
    switch (type->bi_type_id()) {
    case BoolId:
        return expl;
    case StringId:
        unreachable();
    default:
        return false;
    }
}

bool StringType::is_castable_to_prim(BuiltinTypeId id, bool expl) const {
    switch (id) {
    case BoolId:
        return expl;
    case StringId:
        unreachable();
    default:
        return false;
    }
}

TypedValue StringType::cast_to_prim(BuiltinTypeId id, RValue&& rval) {
    ulam_assert(is_expl_castable_to(id));

    bool is_unknown = !rval.has_rvalue();
    bool is_consteval = !is_unknown && rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (id) {
    case BoolId: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value::make_r_ph()};
        bool is_empty = len(Value{rval.copy()}) == 0;
        return {type, Value{type->construct(!is_empty, rval_flags)}};
    }
    default:
        unreachable();
    }
}

RValue StringType::cast_to_prim(Ref<PrimType> type, RValue&& rval) {
    ulam_assert(is_expl_castable_to(type));
    if (!rval.has_rvalue())
        return std::move(rval);

    bool is_consteval = rval.is_consteval();
    value::flags_t rval_flags = value::IsConsteval * is_consteval;

    switch (type->bi_type_id()) {
    case BoolId: {
        auto boolean = builtins().bool_type(type->bitsize());
        bool is_empty = len(Value{rval.copy()}) == 0;
        return boolean->construct(!is_empty, rval_flags);
    }
    default:
        unreachable();
    }
}

} // namespace ulam
