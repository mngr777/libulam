#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

class Var : public VarBase {
public:
    using VarBase::VarBase;

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

    ObjectView obj_view();

    RValue rvalue();

private:
    Value _value;
};

} // namespace ulam
