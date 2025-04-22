#include "./stringifier.hpp"
#include "../out.hpp"
#include <cassert>
#include <libulam/semantic/detail/integer.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/value/types.hpp>

std::string
Stringifier::stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval) {
    assert(!rval.empty());
    if (type->is_prim())
        return stringify_prim(type->as_prim(), rval);
    if (type->is_class())
        return stringify_class(type->as_class(), rval);
    if (type->is_array())
        return stringify_array(type->as_array(), rval);
    if (type->is_atom())
        return "";
    assert(false);
}

std::string Stringifier::stringify_prim(
    ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval) {
    switch (type->bi_type_id()) {
    case ulam::IntId: {
        auto int_val = rval.get<ulam::Integer>();
        return std::to_string(int_val);
    }
    case ulam::UnsignedId: {
        return unsigned_to_str(rval.get<ulam::Unsigned>());
    }
    case ulam::UnaryId: {
        if (options.unary_as_unsigned_lit)
            return unsigned_to_str(rval.get<ulam::Unsigned>());
        auto unary_type = _builtins.unary_type(type->bitsize());
        auto uns_val = unary_type->unsigned_value(rval);
        return unsigned_to_str(uns_val);
    }
    case ulam::BoolId: {
        if (options.bool_as_unsigned_lit)
            return unsigned_to_str(rval.get<ulam::Unsigned>());
        auto bool_type = _builtins.bool_type(type->bitsize());
        return bool_type->is_true(rval) ? "true" : "false";
    }
    case ulam::BitsId: {
        if (type->bitsize() < sizeof(ulam::Datum) * 8) {
            return bits_to_str(rval.get<ulam::Bits>());
        } else {
            assert(false); // TODO
        }
    }
    default:
        assert(false); // TODO
    }
}

std::string Stringifier::stringify_class(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    std::string str;

    // typedefs
    for (auto type_def : cls->type_defs())
        str += " " + out::type_def_str(*this, type_def) + "; ";

    // consts
    for (auto var : cls->params())
        str += " " + out::var_str(_str_pool, *this, var) + "; ";

    // props
    auto rval_copy = rval.copy(); // TMP
    for (auto prop : cls->props())
        str += " " + out::prop_str(_str_pool, *this, prop, rval_copy) + "; ";
    return str;
}

std::string Stringifier::stringify_array(
    ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval) {
    if (array_type->array_size() == 0)
        return " ";

    std::string str;
    auto item_type = array_type->item_type();
    auto data = rval.data_view();
    for (ulam::array_idx_t idx = 0; idx < array_type->array_size(); ++idx) {
        if (idx > 0)
            str += item_type->is_class() ? "" : ",";
        auto item_rval = data.array_item(idx).load();
        str += stringify(item_type, item_rval);
    }
    return str;
}

std::string Stringifier::unsigned_to_str(ulam::Unsigned val) {
    auto str = std::to_string(val);
    if (options.use_unsigned_suffix)
        str += "u";
    return str;
}

std::string Stringifier::bits_to_str(const ulam::Bits& bits) {
    assert(bits.len() <= sizeof(ulam::Datum) * 8);
    auto datum = bits.read(0, bits.len());
    auto str = std::to_string((ulam::Unsigned)datum);
    if (options.bits_use_unsigned_suffix)
        str += "u";
    return str;
}
