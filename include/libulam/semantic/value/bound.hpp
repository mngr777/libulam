#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

class FunSet;
class Prop;
class RValue;

template <typename T> class Bound {
public:
    Bound(SPtr<Object> main, ObjectView obj_view, Ref<T> mem):
        _main{main}, _obj_view{obj_view}, _mem{mem} {}

    Bound(SPtr<Object> obj, Ref<T> mem): Bound{obj, obj->view(), mem} {}
    Bound(ObjectView obj_view, Ref<T> mem):
        Bound{SPtr<Object>(), obj_view, mem} {}

    SPtr<Object> main() { return _main; }
    SPtr<const Object> main() const { return _main; }

    ObjectView obj_view() { return _obj_view; }
    const ObjectView obj_view() const { return _obj_view; }

    Ref<T> mem() { return _mem; }
    Ref<const T> mem() const { return _mem; }

    virtual void load(RValue& rval) const { assert(false); }
    virtual void store(const RValue& rval) { assert(false); }

private:
    SPtr<Object> _main;
    ObjectView _obj_view;
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
