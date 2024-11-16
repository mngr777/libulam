#pragma once
#include <cassert>
#include <libulam/ast/context.hpp>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/str_pool.hpp>
#include <string>
#include <utility>

namespace ulam {
class Class;
class Module;
class Program;
} // namespace ulam

namespace ulam::ast {

class Context;
class ModuleDef;
class TypeDef;
class ClassDef;
class FunDef;
class VarDefList;

class Root : public List<Node, ModuleDef> {
    ULAM_AST_NODE
    ULAM_AST_PTR_ATTR(Program, program)
public:
    Root();
    ~Root();

    Context& ctx() { return _ctx; }
    const Context& ctx() const { return _ctx; }

private:
    Context _ctx;
};

class ModuleDef : public ListOf<Stmt, TypeDef, VarDefList, ClassDef> {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(Module, module)
};

class ClassDefBody : public ListOf<Stmt, TypeDef, FunDef, VarDefList> {
    ULAM_AST_NODE
};

class ClassDef : public Tuple<Stmt, ParamList, ClassDefBody>, public Named_ {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(Class, type)
    ULAM_AST_SIMPLE_ATTR(loc_id_t, loc_id)
    ULAM_AST_SIMPLE_ATTR(loc_id_t, name_loc_id)
public:
    ClassDef(Class::Kind kind, str_id_t name_id, Ptr<ParamList>&& params):
        Tuple{std::move(params), make<ClassDefBody>()},
        Named_{name_id},
        _kind{kind} {}

    ULAM_AST_TUPLE_PROP(params, 0)
    ULAM_AST_TUPLE_PROP(body, 1)

    Class::Kind kind() const { return _kind; }

private:
    Class::Kind _kind;
};

class TypeDef : public Tuple<Stmt, TypeName> {
    ULAM_AST_NODE
public:
    TypeDef(Ptr<TypeName>&& expr, std::string&& alias):
        Tuple{std::move(expr)}, _alias{alias} {}

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

    unsigned def_num() const { return List::child_num(); }

    Ref<VarDef> def(unsigned n) { return List::get(n); }
    const Ref<VarDef> def(unsigned n) const { return List::get(n); }

    unsigned child_num() const override {
        return Tuple::child_num() + List::child_num();
    }

    ULAM_AST_TUPLE_PROP(base_type, 0);

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
    const Ref<Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
};

} // namespace ulam::ast
