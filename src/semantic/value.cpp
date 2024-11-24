#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

RValue* LValue::rvalue() {
    if (is_unknown())
        return nullptr;
    if (is<Ref<Var>>()) {
        auto var = get<Ref<Var>>();
        return var ? var->rvalue() : nullptr;
    }
    assert(false);
}

} // namespace ulam
