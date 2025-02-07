#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/layout.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

class Var : public VarBase {
public:
    Var(Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        Ref<Type> type,
        Flag flags = NoFlags):
        VarBase{type_node, node, type, flags} {}

    Var(Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        TypedValue&& tv,
        Flag flags = NoFlags):
        VarBase{type_node, node, tv.type(), flags}, _value{tv.move_value()} {}

    bool requires_value() const {
        return is_const() && !(is(Tpl) && is(ClassParam)) && !is(FunParam);
    }

    Value& value() { return _value; } // TMP
    const Value& value() const { return _value; }
    void set_value(Value&& value);

    // TODO: if reference, get rvalue from refd var
    RValue* rvalue() { return _value.rvalue(); };

    bool has_data_off() const { return _data_off == cls::NoDataOff; }
    cls::data_off_t data_off() const { return _data_off; }
    void set_data_off(cls::data_off_t off) { _data_off = off; }

private:
    Value _value;
    cls::data_off_t _data_off{cls::NoDataOff};
};

} // namespace ulam
