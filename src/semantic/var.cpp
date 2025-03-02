#include <libulam/semantic/var.hpp>

namespace ulam {

void Var::set_value(Value&& value) { std::swap(_value, value); }

DataView Var::data_view() {
    assert(_value.is_rvalue());
    return _value.rvalue().accept(
        [&](DataPtr data) { return data->view(); },
        [&](auto& other) -> DataView { assert(false); });
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

} // namespace ulam
