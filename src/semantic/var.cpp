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

LValue Var::lvalue() {
    return _value.accept(
        [&](LValue& lval) { return LValue{lval}; },
        [&](auto& other) {
            LValue lval(this);
            lval.set_scope_lvl(_scope_lvl);
            return lval;
        });
}

RValue Var::rvalue() { return _value.copy_rvalue(); }

} // namespace ulam
