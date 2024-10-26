#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <string>
#include <utility>

namespace ulam::ast {

class Param : public Tuple<Expr, Expr, Expr> {
    ULAM_AST_NODE
public:
    Param(std::string name, Ptr<Expr>&& type, Ptr<Expr>&& default_value):
        Tuple{std::move(type), std::move(default_value)},
        _name{std::move(name)} {}

    const std::string& name() const { return _name; }

    ULAM_AST_TUPLE_PROP(type, 0)
    ULAM_AST_TUPLE_PROP(default_value, 1)

private:
    std::string _name;
};

class ParamList : public List<Param> {
    ULAM_AST_NODE
};

class ArgList : public List<Expr> {
    ULAM_AST_NODE
};

} // namespace ulam::ast
