#pragma once
#include <cassert>
#include <libulam/ast/context.hpp>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <utility>

namespace ulam {
class Program;
class Module;
class Class;
class ClassTpl;
class Var;
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

class ClassDef : public Tuple<Stmt, ParamList, ClassDefBody>, public Named {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(Class, type)
    ULAM_AST_REF_ATTR(ClassTpl, type_tpl)
public:
    ClassDef(ClassKind kind, ast::Str name, Ptr<ParamList>&& params):
        Tuple{std::move(params), make<ClassDefBody>()},
        Named{name},
        _kind{kind} {}

    ULAM_AST_TUPLE_PROP(params, 0)
    ULAM_AST_TUPLE_PROP(body, 1)

    ClassKind kind() const { return _kind; }

private:
    ClassKind _kind;
};

class TypeDef : public Tuple<Stmt, TypeName>, public Named {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(AliasType, alias_type)
public:
    TypeDef(Ptr<TypeName>&& type, Str name):
        Tuple{std::move(type)}, Named{name} {}

    ULAM_AST_TUPLE_PROP(type_name, 0)
};

class FunDefBody : public Block {
    ULAM_AST_NODE
};

class FunDef : public Tuple<Stmt, TypeName, ParamList, FunDefBody>,
               public Named {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(FunOverload, overload)
public:
    FunDef(
        Str name,
        Ptr<TypeName>&& ret_type_name,
        Ptr<ParamList>&& params,
        Ptr<FunDefBody>&& body):
        Tuple{std::move(ret_type_name), std::move(params), std::move(body)},
        Named{name} {}

    ULAM_AST_TUPLE_PROP(ret_type_name, 0)
    ULAM_AST_TUPLE_PROP(params, 1)
    ULAM_AST_TUPLE_PROP(body, 2)
};

class VarDef : public VarDecl {
    ULAM_AST_NODE
public:
    VarDef(Str name, Ptr<Expr>&& expr): VarDecl{name, std::move(expr)} {}
};

class VarDefList : public Tuple<List<Stmt, VarDef>, TypeName> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_const, false)
public:
    explicit VarDefList(Ptr<TypeName>&& type): Tuple{std::move(type)} {}

    unsigned def_num() const { return List::child_num(); }

    Ref<VarDef> def(unsigned n) { return List::get(n); }
    const Ref<VarDef> def(unsigned n) const { return List::get(n); }

    unsigned child_num() const override {
        return Tuple::child_num() + List::child_num();
    }

    ULAM_AST_TUPLE_PROP(type_name, 0);

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
    const Ref<Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
};

} // namespace ulam::ast
