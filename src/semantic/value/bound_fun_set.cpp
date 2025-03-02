#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>

namespace ulam {

BoundFunSet::BoundFunSet(DataView self, Ref<FunSet> fset):
    _self{self}, _fset{fset} {
    assert(self.type()->is_class());
}

} // namespace ulam
