#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>

namespace ulam {

class FunSet;
class Object;
class Prop;
class RValue;

template <typename T> class Bound {
public:
    Bound(SPtr<Object> obj, Ref<T> mem): _obj{obj}, _mem{mem} {}

    SPtr<Object> obj() { return _obj; }
    SPtr<const Object> obj() const { return _obj; }

    Ref<T> mem() { return _mem; }
    Ref<const T> mem() const { return _mem; }

    virtual void load(RValue& rval) const { assert(false); }
    virtual void store(const RValue& rval) { assert(false); }

private:
    SPtr<Object> _obj;
    Ref<T> _mem;
};

using BoundFunSet = Bound<FunSet>;

class BoundProp : public Bound<Prop> {
public:
    using Bound::Bound;

    void load(RValue& rval) const override;
    void store(const RValue& rval) override;
};

} // namespace ulam
