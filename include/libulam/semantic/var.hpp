#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class VarDefList;
class VarDef;
} // namespace ulam::ast

namespace ulam {

class Var {
public:
    Var(ast::Ref<ast::VarDefList> list_node,
        ast::Ref<ast::VarDef> node,
        Ref<Type> type):
        _list_node{list_node}, _node{node}, _type{type} {}

    ast::Ref<ast::VarDefList> list_node() { return _list_node; }
    ast::Ref<ast::VarDef> node() { return _node; }

    Ref<Type> type() { return _type; }

    Value& value() { return _value; }

    bool is_const() const { return _is_const; }
    void set_is_const(bool is_const) { _is_const = is_const; }

private:
    ast::Ref<ast::VarDefList> _list_node;
    ast::Ref<ast::VarDef> _node;
    Ref<Type> _type;
    Value _value;
    bool _is_const{false};
};

} // namespace ulam
