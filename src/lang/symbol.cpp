#include "libulam/lang/symbol.hpp"

namespace ulam {

Type::~Type() {}

Ref<Var> Var::deref() {
    Ref<Var> var{this};
    while (var->_refd)
        var = var->_refd;
    return var;
}

} // namespace ulam
