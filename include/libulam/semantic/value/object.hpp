#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bit_vector.hpp>

namespace ulam {

class Class;
class ObjectView;
class Value;

class Object {
public:
    Object(Ref<Class> cls);
    Object(Ref<Class> cls, BitVector&& bits);
    ~Object();

    ObjectView view();
    const ObjectView view() const;

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

    BitVector& bits() { return _bits; }
    const BitVector& bits() const { return _bits; }

private:
    Ref<Class> _cls;
    BitVector _bits;
};

class ObjectView {
public:
    ObjectView(Ref<Class> cls, BitVectorView bits);
    ObjectView() {}

    operator bool() const;

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

    BitVectorView bits() { return _bits; }
    const BitVectorView bits() const { return _bits; }

private:
    Ref<Class> _cls{};
    BitVectorView _bits;
};

} // namespace ulam
