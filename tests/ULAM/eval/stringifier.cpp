#include "./stringifier.hpp"
#include <cassert>
#include <libulam/semantic/value/types.hpp>

std::string
Stringifier::stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval) {
    if (type->is_prim())
        return stringify_prim(type->as_prim(), rval);
    assert(false); // TODO
}

std::string Stringifier::stringify_prim(
    ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval) {
    assert(!rval.empty());
    switch (type->bi_type_id()) {
    case ulam::IntId: {
        auto int_val = rval.get<ulam::Integer>();
        return std::to_string(int_val);
    }
    default:
        assert(false); // TODO
    }
}
