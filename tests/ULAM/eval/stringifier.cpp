#include "./stringifier.hpp"
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
    assert(false); // TODO
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
        return std::to_string(uns_val) + 'u';
    }
    case ulam::UnaryId: {
        auto unary_type = _builtins.unary_type(type->bitsize());
        auto uns_val = unary_type->unsigned_value(rval);
        return std::to_string(uns_val) + 'u';
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
    return ""; // TODO
}
