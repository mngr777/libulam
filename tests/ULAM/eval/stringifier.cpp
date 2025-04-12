#include "./stringifier.hpp"
#include "../prop_str.hpp"
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
        auto uns_val = rval.get<ulam::Unsigned>();
        return std::to_string(uns_val) + (_is_main ? "u" : "");
    }
    case ulam::UnaryId: {
        auto unary_type = _builtins.unary_type(type->bitsize());
        auto uns_val = unary_type->unsigned_value(rval);
        return std::to_string(uns_val) + (_is_main ? "u" : "");
    }
    case ulam::BoolId: {
        auto bool_type = _builtins.bool_type(type->bitsize());
        return bool_type->is_true(rval) ? "true" : "false";
    }
    case ulam::BitsId: {
        if (type->bitsize() < sizeof(ulam::Integer) * 8) {
            auto& bits = rval.get<ulam::Bits>();
            auto datum = bits.read_right(type->bitsize());
            ulam::Integer int_val =
                ulam::detail::integer_from_datum(datum, type->bitsize());
            return std::to_string(int_val);
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
    auto rval_copy = rval.copy(); // TMP
    for (auto prop : cls->props())
        str += " " + prop_str(_str_pool, *this, prop, rval_copy) + "; ";
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
