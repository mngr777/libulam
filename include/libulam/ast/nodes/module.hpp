#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/lang/class.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace ulam::ast {

class TypeDef;
class ClassDef;
class FunDef;
class VarDef;

class Module : public Node {
    ULAM_AST_NODE
public:
    template <typename N> void add(Ptr<N>&& member) {
        _members.push_back(std::move(member));
    }

    unsigned child_num() const override { return _members.size(); }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return as_node(_members[n]);
    }

private:
    ListOf<ClassDef> _members;
};

class ClassDef : public Node {
    ULAM_AST_NODE
public:
    ClassDef(Class::Kind kind, std::string name):
        _kind{kind}, _name(std::move(name)) {}

    ClassDef(Class::Kind kind, const std::string_view name):
        ClassDef{kind, std::string(name)} {}

    template <typename N> void add(Ptr<N>&& member) {
        _members.push_back(std::move(member));
    }

    const std::string& name() const { return _name; }

    unsigned child_num() const override { return _members.size(); }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return as_node(_members[n]);
    }

private:
    Class::Kind _kind;
    std::string _name;
    ListOf<TypeDef, FunDef, VarDef> _members;
};

class TypeDef : public Node {
    ULAM_AST_NODE
public:
    TypeDef(std::string alias, Ptr<Expr>&& expr):
        _alias{std::move(alias)}, _expr{std::move(expr)} {}

    TypeDef(const std::string_view alias, Ptr<Expr>&& expr):
        TypeDef{std::string(alias), std::move(expr)} {}

    const std::string& alias() const { return _alias; }

    Expr* expr() { return _expr.get(); }

    unsigned child_num() const override { return 1; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return _expr.get();
    }

private:
    std::string _alias;
    Ptr<Expr> _expr;
};

class FunDef : public Node {
    ULAM_AST_NODE
};

class VarDef : public Node {
    ULAM_AST_NODE
};

} // namespace ulam::ast
