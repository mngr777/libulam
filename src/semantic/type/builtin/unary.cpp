#include <libulam/semantic/type/builtin/unary.hpp>

namespace ulam {

RValue UnaryType::from_datum(Datum datum) const { return (Unsigned)datum; }

Datum UnaryType::to_datum(const RValue& rval) const {
    assert(rval.is<Unsigned>());
    return rval.get<Unsigned>();
}

} // namespace ulam
