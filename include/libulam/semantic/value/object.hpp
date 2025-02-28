#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bit_storage.hpp>

namespace ulam {

class Class;
class ObjectView;
class Type;

class Object : public _BitStorage {
public:
    explicit Object(Ref<Type> cls);
    Object(Ref<Type> cls, BitVector&& bits);

    ~Object();

    SPtr<Object> copy() const;
    void cast(Ref<Type> type);

    ObjectView view();
    const ObjectView view() const;

    Ref<Type> type() const { return _type; }
    Ref<Class> cls() const;

private:
    Ref<Type> _type;
};

class ObjectView : public _BitStorageView {
public:
    explicit ObjectView(Ref<Type> cls, BitVectorView bits);
    ObjectView() {}

    operator bool() const;

    SPtr<Object> copy() const;

    Ref<Type> type() const { return _type; }
    Ref<Class> cls() const;

private:
    Ref<Type> _type;
};

} // namespace ulam
