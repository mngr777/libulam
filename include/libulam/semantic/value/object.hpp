#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bit_storage.hpp>

namespace ulam {

class Class;
class ObjectView;
class Value;

class Object : public _BitStorage {
public:
    Object(Ref<Class> cls);
    Object(Ref<Class> cls, BitVector&& bits);
    ~Object();

    SPtr<Object> copy() const;

    ObjectView view();
    const ObjectView view() const;

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

private:
    Ref<Class> _cls;
};

class ObjectView : public _BitStorageView {
public:
    ObjectView(Ref<Class> cls, BitVectorView bits);
    ObjectView() {}

    operator bool() const;

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

private:
    Ref<Class> _cls{};
};

} // namespace ulam
