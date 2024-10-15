#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <string>
#include <utility>

namespace ulam::ast {

class Param : public Node {
    ULAM_AST_NODE
public:
    Param(std::string name, Ptr<Expr>&& type, Ptr<Expr>&& default_value):
        _name{std::move(name)},
        _type{std::move(type)},
        _default_value{std::move(default_value)} {}

    const std::string& name() const { return _name; }

    Expr* type() { return _type.get(); }
    Expr* default_value() { return _default_value.get(); }

    unsigned child_num() const override { return 2; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        if (n == 0)
            return type();
        return default_value();
    }

private:
    std::string _name;
    Ptr<Expr> _type;
    Ptr<Expr> _default_value;
};

class ParamList : public List<Param> {
    ULAM_AST_NODE
};

class ArgList : public Node {
public:
};

} // namespace ulam::ast
