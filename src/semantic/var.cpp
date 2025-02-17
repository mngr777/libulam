#include <libulam/semantic/var.hpp>

namespace ulam {

void Var::set_value(Value&& value) { std::swap(_value, value); }

ArrayView Var::array_view() {
    assert(_value.is_rvalue());
    return _value.rvalue().accept(
        [&](Array& array) { return array.view(); },
        [&](auto& other) -> ArrayView { assert(false); });
}

ObjectView Var::obj_view() {
    assert(_value.is_rvalue());
    return _value.rvalue().accept(
        [&](SPtr<Object> obj) { return obj->view(); },
        [&](auto& other) -> ObjectView { assert(false); });
}

RValue Var::rvalue() { return _value.copy_rvalue(); }

} // namespace ulam
