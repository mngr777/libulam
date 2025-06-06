#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/exprs.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <libulam/types.hpp>
#include <utility>

namespace ulam {
class Module;
class Class;
class ClassTpl;
class Var;
} // namespace ulam

namespace ulam::ast {

class ModuleDef;
class TypeDef;
class ClassDef;
class FunDef;
class VarDefList;

class ModuleDef : public ListOf<Stmt, TypeDef, VarDefList, ClassDef> {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(Module, module)
    ULAM_AST_SIMPLE_ATTR(version_t, ulam_version, 0)
    ULAM_AST_SIMPLE_ATTR(str_id_t, name_id, NoStrId)
};

class ClassDefBody : public ListOf<Stmt, TypeDef, FunDef, VarDefList> {
    ULAM_AST_NODE
};

class ClassDef : public Tuple<Stmt, ParamList, TypeNameList, ClassDefBody>,
                 public Named,
                 public DefNode {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(Class, cls)
    ULAM_AST_REF_ATTR(ClassTpl, cls_tpl)
public:
    ClassDef(
        ClassKind kind,
        ast::Str name,
        Ptr<ParamList>&& params,
        Ptr<TypeNameList>&& ancestors):
        Tuple{std::move(params), std::move(ancestors), make<ClassDefBody>()},
        Named{name},
        _kind{kind} {}

    ULAM_AST_TUPLE_PROP(params, 0)
    ULAM_AST_TUPLE_PROP(ancestors, 1)
    ULAM_AST_TUPLE_PROP(body, 2)

    ClassKind kind() const { return _kind; }

    bool is_tpl() const {
        assert(!has_params() || params()->child_num() > 0);
        return has_params();
    }

    Ref<ClassBase> cls_or_tpl() {
        if (cls())
            return cls();
        return cls_tpl();
    }

private:
    ClassKind _kind;
};

class TypeExpr : public Tuple<Stmt, TypeIdent, ExprList> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_ref, false)
    ULAM_AST_SIMPLE_ATTR(loc_id_t, amp_loc_id, NoLocId)
public:
    TypeExpr(Ptr<TypeIdent>&& ident, Ptr<ExprList>&& array_dims):
        Tuple{std::move(ident), std::move(array_dims)} {}

    ULAM_AST_TUPLE_PROP(ident, 0)
    ULAM_AST_TUPLE_PROP(array_dims, 1)

    bool is_array() const { return has_array_dims(); }
};

class TypeDef : public Tuple<Stmt, TypeName, TypeExpr>, public DefNode {
    ULAM_AST_NODE
public:
    TypeDef(Ptr<TypeName>&& type_name, Ptr<TypeExpr>&& type_expr):
        Tuple{std::move(type_name), std::move(type_expr)} {}

    ULAM_AST_TUPLE_PROP(type_name, 0)
    ULAM_AST_TUPLE_PROP(type_expr, 1)

    Str alias() {
        assert(has_type_expr());
        assert(type_expr()->has_ident());
        return type_expr()->ident()->name();
    }

    str_id_t alias_id() { return alias().str_id(); }
};

class FunDefBody : public Block {
    ULAM_AST_NODE
};

class FunRetType : public Tuple<Stmt, TypeName, ExprList> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_ref, false)
public:
    FunRetType(Ptr<TypeName>&& type_name, Ptr<ExprList>&& array_dims):
        Tuple{std::move(type_name), std::move(array_dims)} {
        set_loc_id(get<0>()->loc_id());
    }

    ULAM_AST_TUPLE_PROP(type_name, 0)
    ULAM_AST_TUPLE_PROP(array_dims, 1)
};

class FunDef : public Tuple<Stmt, FunRetType, ParamList, FunDefBody>,
               public Named,
               public DefNode {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(Fun, fun)
    ULAM_AST_SIMPLE_ATTR(bool, is_constructor, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_op_alias, false)
    ULAM_AST_SIMPLE_ATTR(Op, op, Op::None)
    // NOTE: function can be virtual, but not marked as such
    ULAM_AST_SIMPLE_ATTR(bool, is_marked_virtual, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_marked_override, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_native, false)
public:
    FunDef(
        Str name,
        Ptr<FunRetType>&& ret_type,
        Ptr<ParamList>&& params,
        Ptr<FunDefBody>&& body):
        Tuple{std::move(ret_type), std::move(params), std::move(body)},
        Named{name} {}

    ULAM_AST_TUPLE_PROP(ret_type, 0)
    ULAM_AST_TUPLE_PROP(params, 1)
    ULAM_AST_TUPLE_PROP(body, 2)

    unsigned param_num() const {
        assert(has_params());
        return params()->child_num();
    }

    bool is_op() const { return op() != Op::None; }
};

class VarDef : public VarDecl {
    ULAM_AST_NODE
public:
    VarDef(Str name, Ptr<ExprList>&& array_dims, Ptr<InitValue>&& init):
        VarDecl{name, std::move(array_dims), std::move(init)} {}
    VarDef(Str name): VarDef{name, {}, {}} {}
};

class VarDefList : public Tuple<List<Stmt, VarDef>, TypeName> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_const, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_parameter, false)
public:
    explicit VarDefList(Ptr<TypeName>&& type): Tuple{std::move(type)} {}

    unsigned def_num() const { return List::child_num(); }

    Ref<VarDef> def(unsigned n) { return List::get(n); }
    const Ref<VarDef> def(unsigned n) const { return List::get(n); }

    ULAM_AST_TUPLE_PROP(type_name, 0);

    unsigned child_num() const override {
        return Tuple::child_num() + List::child_num();
    }

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
    Ref<const Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
};

} // namespace ulam::ast
