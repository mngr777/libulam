#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

class Var : public VarBase {
public:
    using VarBase::VarBase;

    // tmp variables, type is required
    Var(Ref<Type> type, Value&& value, Flag flags = NoFlags);
    Var(TypedValue&& tv, Flag flags = NoFlags);

    Var(Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        Ref<Type> type,
        Value&& val,
        Flag flags = NoFlags);

    Var(Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        TypedValue&& tv,
        Flag flags = NoFlags);

    bool requires_value() const {
        return is_const() && !is(Tpl) && !is(ClassParam);
    }

    const Value& value() const { return _value; }
    void set_value(Value&& value);

    DataView data_view();

    LValue lvalue();
    RValue rvalue() const;

    Value move_value();

    bool is_consteval() const { return _value.is_consteval() && !is(FunParam); }

    bool has_scope_lvl() const { return _scope_lvl != NoScopeLvl; }
    scope_lvl_t scope_lvl() const { return _scope_lvl; }
    void set_scope_lvl(scope_lvl_t scope_lvl) { _scope_lvl = scope_lvl; }

private:
    Value _value{RValue{}};
    scope_lvl_t _scope_lvl{NoScopeLvl};
};

} // namespace ulam
