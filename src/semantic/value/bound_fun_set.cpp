#include <cassert>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

BoundFunSet::BoundFunSet(DataView self, Ref<FunSet> fset, Ref<Class> eff_cls):
    _self{self}, _fset{fset}, _eff_cls{eff_cls} {
    assert(!_self || _self.type(true)->is_class());
}

bool BoundFunSet::has_self() const { return _self; }

DataView BoundFunSet::self() const {
    assert(_self);
    return _self;
}

Ref<Class> BoundFunSet::dyn_cls() const {
    if (_eff_cls)
        return _eff_cls;
    if (has_self())
        return self().type(true)->as_class();
    return _fset->cls();
}

Ref<Class> BoundFunSet::eff_cls() const { return _eff_cls; }

} // namespace ulam
