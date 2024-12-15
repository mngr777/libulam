#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value.hpp>
#include <vector>

namespace ulam {

class Class;

class Object {
public:
    Object(Ref<Class> cls): _cls{cls} {}

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

    // TODO: data

private:
    Ref<Class> _cls;
    std::vector<RValue> _values;
};

} // namespace ulam
