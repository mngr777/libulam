#include "libulam/semantic/value/bound.hpp"
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

// LValue

Ref<Type> LValue::type() {
    return {}; // TODO
}

RValue* LValue::rvalue() {
    if (empty())
        return {};


    // accept([&](Ref<Var> var) {
    //     return var->rvalue();
    // }, [&](BoundProp& bound_prop) {})
    // if (is<Ref<Var>>()) {
    //     auto var = get<Ref<Var>>();
    //     return var->rvalue();
    // }

    // if (is<BoundProp>()) {
    //     RValue rvalue;
    //     auto& bound_prop = get<BoundProp>();
    //     return bound_prop.load(rvalue);
    // }

    assert(false);
}

} // namespace ulam
