#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>

namespace ulam {

BoundFunSet::BoundFunSet(DataView self, Ref<FunSet> fset):
    _self{self}, _fset{fset} {
    assert(!self || self.type()->is_class());
}

bool BoundFunSet::has_self() const { return _self; }

DataView BoundFunSet::self() const {
    assert(_self);
    return _self;
}

} // namespace ulam
