#include <libulam/semantic/var.hpp>

namespace ulam {

void Var::set_value(Value&& value) { std::swap(_value, value); }

DataView Var::data_view() {
    if (_value.is_lvalue())
        return _value.lvalue().data_view(); // ??
    return _value.rvalue().accept(
        [&](DataPtr data) { return data->view(); },
        [&](auto& other) { return DataView{}; });
}

LValue Var::lvalue() {
    LValue lval = _value.accept(
        [&](LValue& lval) { return LValue{lval}; },
        [&](auto& other) { return LValue{this}; });
    lval.set_scope_lvl(_scope_lvl);
    lval.set_is_xvalue(false);
    return lval;
}

RValue Var::rvalue() const { return _value.copy_rvalue(); }

Value Var::move_value() {
    auto val = _value.is_lvalue() ? Value{LValue{}} : Value{RValue{}};
    std::swap(val, _value);
    return val;
}

} // namespace ulam
