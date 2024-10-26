#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/lang/class.hpp>
#include <string>
#include <utility>

namespace ulam::ast {

class TypeDef;
class ClassDef;
class FunDef;
class VarDefList;

class Module : public ListOf<Node, TypeDef, VarDefList, ClassDef> {
    ULAM_AST_NODE
};

class ClassDefBody : public ListOf<Node, TypeDef, FunDef, VarDefList> {
    ULAM_AST_NODE
};

class ClassDef : public Tuple<Stmt, ClassDefBody>, public Named {
    ULAM_AST_NODE
public:
    ClassDef(Class::Kind kind, std::string&& name):
        Tuple{make<ClassDefBody>()}, Named{std::move(name)}, _kind{kind} {}

    const Class::Kind kind() const { return _kind; }

    ULAM_AST_TUPLE_PROP(body, 0)

private:
    Class::Kind _kind;
};

class TypeDef : public Tuple<Node, Expr> {
    ULAM_AST_NODE
public:
    TypeDef(std::string alias, Ptr<Expr>&& expr):
        Tuple{std::move(expr)}, _alias{std::move(alias)} {}

    const std::string& alias() const { return _alias; }

    ULAM_AST_TUPLE_PROP(expr, 0)

private:
    std::string _alias;
};

class FunDef : public Tuple<Stmt, Expr, ParamList, Block>, public Named {
    ULAM_AST_NODE
public:
    FunDef(
        std::string&& name,
        Ptr<Expr>&& ret_type,
        Ptr<ParamList>&& params,
        Ptr<Block>(block)):
        Tuple{std::move(ret_type), std::move(params), std::move(block)},
        Named{std::move(name)} {}

    ULAM_AST_TUPLE_PROP(ret_type, 0)
    ULAM_AST_TUPLE_PROP(params, 1)
    ULAM_AST_TUPLE_PROP(body, 2)
};

class VarDef : public Tuple<Node, Expr, Expr>, public Named {
    ULAM_AST_NODE
public:
    VarDef(Ref<Expr>&& type_ref, std::string&& name, Ptr<Expr>&& value):
        Tuple{nullptr, std::move(value)},
        Named{std::move(name)},
        _type_ref{type_ref} {}

    VarDef(Ptr<Expr>&& type, std::string name, Ptr<Expr>&& value):
        Tuple{std::move(type), std::move(value)},
        Named{std::move(name)},
        _type_ref{get<0>()} {}

    ULAM_AST_TUPLE_PROP(type, 0);
    ULAM_AST_TUPLE_PROP(value, 1);

private:
    Ref<Expr> _type_ref;
};

class VarDefList : public Tuple<List<Stmt, VarDef>, Expr> {
    ULAM_AST_NODE
public:
    explicit VarDefList(Ptr<Expr>&& base_type): Tuple{std::move(base_type)} {}

    ULAM_AST_TUPLE_PROP(base_type, 0);

    unsigned def_num() const { return List::child_num(); }

    Ref<VarDef> def(unsigned n) { return List::get(n); }
    const Ref<VarDef> def(unsigned n) const { return List::get(n); }

    unsigned child_num() const override {
        return Tuple::child_num() + List::child_num();
    }

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(n) : List::child(n - 1);
    }
    const Ref<Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(n) : List::child(n - 1);
    }
};

} // namespace ulam::ast
