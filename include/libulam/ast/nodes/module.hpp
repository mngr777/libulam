#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
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

    template <typename N> void add(Ptr<N>&& member) {
        _members.push_back(std::move(member));
    }

    bool is(Class::Kind kind) const { return _kind == kind; }
    Class::Kind kind() const { return _kind; }

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

    const std::string& alias() const { return _alias; }

    Expr* expr() { return _expr.get(); }

    unsigned child_num() const override { return 1; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return expr();
    }

private:
    std::string _alias;
    Ptr<Expr> _expr;
};

class FunDef : public Node {
    ULAM_AST_NODE
public:
    FunDef(
        std::string name,
        Ptr<Expr>&& ret_type,
        Ptr<ParamList>&& params,
        Ptr<Block>&& body):
        _name{std::move(name)},
        _ret_type{std::move(ret_type)},
        _params{std::move(params)},
        _body{std::move(body)} {}

    const std::string& name() const { return _name; }

    Expr* ret_type() { return _ret_type.get(); }
    ParamList* params() { return _params.get(); }
    Block* body() { return _body.get(); }

    unsigned child_num() const override { return 3; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        if (n == 0)
            return ret_type();
        if (n == 1)
            return params();
        return body();
        ;
    }

private:
    std::string _name;
    Ptr<Expr> _ret_type;
    Ptr<ParamList> _params;
    Ptr<Block> _body;
};

class VarDef : public Node {
    ULAM_AST_NODE
public:
    VarDef(std::string name, Ptr<Expr>&& type, Ptr<Expr>&& expr):
        _name{std::move(name)},
        _type{std::move(type)},
        _expr{std::move(expr)} {}

    const std::string& name() const { return _name; }

    Expr* type() { return _type.get(); }
    Expr* expr() { return _expr.get(); }

    unsigned child_num() const override { return 2; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        if (n == 0)
            return type();
        return expr();
    }

private:
    std::string _name;
    Ptr<Expr> _type;
    Ptr<Expr> _expr;
};

} // namespace ulam::ast
