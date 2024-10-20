#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/lang/class.hpp>
#include <memory>
#include <string>
#include <utility>

namespace ulam::ast {

class TypeDef;
class ClassDef;
class FunDef;
class VarDef;

class Module : public ListOf<Node, TypeDef, VarDef, ClassDef> {
    ULAM_AST_NODE
};

class ClassDefBody : public ListOf<Node, TypeDef, FunDef, VarDef> {
    ULAM_AST_NODE
};

class ClassDef : public Tuple<Node, ClassDefBody> {
    ULAM_AST_NODE
public:
    ClassDef(Class::Kind kind, std::string name):
        Tuple{std::make_unique<ClassDefBody>()}, _kind{kind}, _name(std::move(name)) {}

    const Class::Kind kind() const { return _kind; }
    const std::string& name() const { return _name; }

    ULAM_AST_TUPLE_PROP(body, 0)

private:
    Class::Kind _kind;
    std::string _name;
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

class FunDef : public Tuple<Node, Expr, ParamList, Block> {
    ULAM_AST_NODE
public:
    FunDef(
        std::string name,
        Ptr<Expr>&& ret_type,
        Ptr<ParamList>&& params,
        Ptr<Block>(block)):
        Tuple{std::move(ret_type), std::move(params), std::move(block)},
        _name{std::move(name)} {}

    const std::string& name() { return _name; }

    ULAM_AST_TUPLE_PROP(ret_type, 0)
    ULAM_AST_TUPLE_PROP(params, 1)
    ULAM_AST_TUPLE_PROP(body, 2)

private:
    std::string _name;
};

class VarDef : public Tuple<Node, Expr, Expr> {
    ULAM_AST_NODE
public:
    VarDef(std::string name, Ptr<Expr>&& type, Ptr<Expr>&& expr):
        Tuple{std::move(type), std::move(expr)}, _name{std::move(name)} {}

    const std::string& name() const { return _name; }

    ULAM_AST_TUPLE_PROP(type, 0)
    ULAM_AST_TUPLE_PROP(expr, 1)


private:
    std::string _name;
    Ptr<Expr> _type;
    Ptr<Expr> _expr;
};

} // namespace ulam::ast
