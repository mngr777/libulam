#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/lang/class.hpp>
#include <cassert>

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
    ClassDef(Class::Kind kind): _kind{kind} {}

    template <typename N> void add(Ptr<N>&& member) {
        _members.push_back(std::move(member));
    }

    unsigned child_num() const override { return _members.size(); }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return as_node(_members[n]);
    }

private:
    Class::Kind _kind;
    ListOf<TypeDef, FunDef, VarDef> _members;
};

class TypeDef : public Node {
    ULAM_AST_NODE
};

class FunDef : public Node {
    ULAM_AST_NODE
};

class VarDef : public Node {
    ULAM_AST_NODE
};

} // namespace ulam::ast
