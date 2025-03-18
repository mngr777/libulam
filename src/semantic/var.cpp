#include <libulam/semantic/var.hpp>

namespace ulam {

Var::Var(Ref<Type> type, Value&& value, Flag flags):
    Var{{}, {}, type, std::move(value), flags} {
    assert(type);
}

Var::Var(TypedValue&& tv, Flag flags): Var{{}, {}, std::move(tv), flags} {}

Var::Var(
    Ref<ast::TypeName> type_node,
    Ref<ast::VarDecl> node,
    Ref<Type> type,
    Value&& val,
    Flag flags):
    VarBase{type_node, node, type, flags}, _value{std::move(val)} {}

Var::Var(
    Ref<ast::TypeName> type_node,
    Ref<ast::VarDecl> node,
    TypedValue&& tv,
    Flag flags):
    Var{type_node, node, tv.type(), tv.move_value(), flags} {}

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
