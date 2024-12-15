#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

RValue* LValue::rvalue() {
    return const_cast<RValue*>(std::as_const(*this).rvalue());
}

const RValue* LValue::rvalue() const {
    if (is_unknown())
        return nullptr;
    if (is<Ref<Var>>()) {
        auto var = get<Ref<Var>>();
        return var ? var->rvalue() : nullptr;
    }
    assert(false);
}

} // namespace ulam
