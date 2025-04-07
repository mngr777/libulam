#include "./stringifier.hpp"
#include <cassert>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

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
    case ulam::BoolId: {
        auto bool_type = _builtins.bool_type(type->bitsize());
        return bool_type->is_true(rval) ? "true" : "false";
    }
    default:
        assert(false); // TODO
    }
}

std::string Stringifier::stringify_class(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    return ""; // TODO
}
