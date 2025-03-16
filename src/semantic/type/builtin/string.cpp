#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

bitsize_t StringType::bitsize() const { return sizeof(String::id) * 8; }

TypedValue StringType::type_op(TypeOp op, Value& val) {
    switch (op) {
    case TypeOp::LengthOf: {
        auto type = builtins().unsigned_type();
        if (val.empty())
            return {type, Value{type->construct()}};
        str_len_t len_ = len(val);
        assert(len_ != NoStrLen);
        return {type, Value{RValue{(Unsigned)len_, true}}};
    }
    default:
        return Type::type_op(op, val);
    }
}

RValue StringType::construct() const {
    assert(_text_pool.has(""));
    return RValue{String{_text_pool.id("")}};
}

RValue StringType::from_datum(Datum datum) const {
    return RValue{String{(str_id_t)datum}};
}

Datum StringType::to_datum(const RValue& rval) const {
    assert(rval.is<String>());
    return rval.get<String>().id;
}


str_len_t StringType::len(const Value& val) const {
    if (val.empty())
        return NoStrLen;
    return text(val).size();
}

std::uint8_t StringType::chr(const Value& val, array_idx_t idx) const {
    if (val.empty())
        return '\0';
    assert(idx < len(val));
    return text(val)[idx];
}

std::string_view StringType::text(const Value& val) const {
    if (val.empty())
        return "";
    RValue rval = val.copy_rvalue();
    auto str = rval.get<String>();
    assert(str.is_valid());
    return _text_pool.get(str.id);
}

TypedValue StringType::binary_op(
    Op op, RValue&& l_rval, Ref<const PrimType> r_type, RValue&& r_rval) {
    assert(r_type->is(StringId));
    assert(l_rval.empty() || l_rval.get<String>().is_valid());
    assert(r_rval.empty() || r_rval.get<String>().is_valid());

    bool is_unknown = l_rval.empty() || r_rval.empty();
    String l_str = l_rval.empty() ? String{} : l_rval.get<String>();
    String r_str = r_rval.empty() ? String{} : r_rval.get<String>();

    bool is_consteval =
        !is_unknown && l_rval.is_consteval() && r_rval.is_consteval();

    switch (op) {
    case Op::Equal: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto rval = type->construct(l_str == r_str);
        rval.set_is_consteval(is_consteval);
        return {type, Value{std::move(rval)}};
    }
    case Op::NotEqual: {
        auto type = builtins().boolean();
        if (is_unknown)
            return {type, Value{RValue{}}};
        auto rval = type->construct(l_str != r_str);
        rval.set_is_consteval(is_consteval);
        return {type, Value{std::move(rval)}};
    }
    default:
        assert(false);
    }
}

} // namespace ulam
