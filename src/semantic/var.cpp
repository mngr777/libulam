#include <libulam/semantic/var.hpp>

namespace ulam {

void Var::set_value(Value&& value) { std::swap(_value, value); }

ObjectView Var::obj_view() {
    assert(_value.is_rvalue());
    return _value.get<RValue>().accept(
        [&](SPtr<Object> obj) { return obj->view(); },
        [&](auto other) -> ObjectView { assert(false); });
}

} // namespace ulam
