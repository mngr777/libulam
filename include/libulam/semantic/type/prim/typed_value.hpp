#pragma once
#include <libulam/semantic/typed_value.hpp>
#include <list>

namespace ulam {

class PrimType;

using PrimTypedValue = _TypedValue<PrimType>;
using PrimTypedValueList = std::list<PrimTypedValue>;

} // namespace ulam
