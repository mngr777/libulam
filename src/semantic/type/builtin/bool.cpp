#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam {

RValue BoolType::from_datum(Datum datum) const { return (Unsigned)datum; }

Datum BoolType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

} // namespace ulam
