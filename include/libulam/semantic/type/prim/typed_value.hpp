#pragma once
#include <libulam/semantic/typed_value.hpp>
#include <list>

namespace ulam {

class PrimType;

class PrimTypedValue : public _TypedValue<PrimType> {
public:
    using _TypedValue::_TypedValue;
};

using PrimTypedValueList = std::list<PrimTypedValue>;

} // namespace ulam
