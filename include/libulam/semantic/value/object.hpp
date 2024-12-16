#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>

namespace ulam {

class Class;
class Value;

class Object {
public:
    Object(Ref<Class> cls);
    ~Object();

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

    Value& get(unsigned idx);
    const Value& get(unsigned idx) const;
    void set(unsigned idx, Value&& value);

private:
    class Data;

    Ref<Class> _cls;
    Ptr<Data> _data;
};

} // namespace ulam
