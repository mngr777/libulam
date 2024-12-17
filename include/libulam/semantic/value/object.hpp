#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam {

class Class;
class Value;

class Object {
public:
    Object(Ref<Class> cls);
    ~Object();

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

private:
    class Data;

    Ref<Class> _cls;
    BitVector _bits;
};

} // namespace ulam
