#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

class Class;
class FunSet;

class BoundFunSet {
public:
    BoundFunSet(DataView self, Ref<FunSet> fset, Ref<Class> dyn_cls = {});

    bool has_self() const;
    DataView self() const;

    Ref<FunSet> fset() const { return _fset; }

    Ref<Class> dyn_cls() const;

private:
    DataView _self;
    Ref<FunSet> _fset;
    Ref<Class> _dyn_cls;
};

} // namespace ulam
