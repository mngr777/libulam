#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

class ArrayView;
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

    virtual ~Bound() {}

    SPtr<Object> main() { return _main; }
    SPtr<const Object> main() const { return _main; }

    ObjectView obj_view() { return _obj_view; }
    const ObjectView obj_view() const { return _obj_view; }

    Ref<T> mem() { return _mem; }
    Ref<const T> mem() const { return _mem; }

private:
    // keeping tmp object alive in e.g.`make_foo()` in `make_foo().bar.baz()`
    SPtr<Object> _main;
    ObjectView _obj_view;
    Ref<T> _mem;
};

class BoundFunSet : public Bound<FunSet> {
public:
    using Bound::Bound;
};

class BoundProp : public Bound<Prop> {
public:
    using Bound::Bound;

    BoundProp mem_obj_bound_prop(Ref<Prop> prop);
    BoundFunSet mem_obj_bound_fset(Ref<FunSet> fset);

    ArrayView mem_array_view();
    ObjectView mem_obj_view();
    BitVectorView bits_view();

    RValue load() const;
    void store(const RValue& rval);
};

} // namespace ulam
