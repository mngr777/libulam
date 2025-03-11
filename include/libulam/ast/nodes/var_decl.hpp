#pragma once
#include <algorithm>
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

namespace ulam::ast {

class InitList;

class InitList2 : public List<Stmt, InitList> {};

class InitList : public Tuple<Stmt, InitList2, ExprList> {
public:
    InitList(): Tuple{make<InitList2>(), make<ExprList>()} {}

    ULAM_AST_TUPLE_PROP(list2, 0);
    ULAM_AST_TUPLE_PROP(expr_list, 1);

    // NOTE: treating empty list as one-dimensional
    bool is_flat() const { return list2()->child_num() == 0; }

    bool empty() const { return child_num() == 0; }

    unsigned depth() const {
        if (is_flat())
            // {expr1, expr2, ...}
            return 1;
        if (list2()->child_num() == 0)
            // {{}}
            return 2;
        return 1 + list2()->get(0)->depth();
    }

    void add(Ptr<InitList>&& init_list) {
        assert(expr_list()->child_num() == 0);
        list2()->add(std::move(init_list));
    }

    void add(Ptr<Expr>&& expr) {
        assert(list2()->child_num() == 0);
        expr_list()->add(std::move(expr));
    }

    Ref<InitList> sublist(unsigned n) {
        assert(!is_flat());
        return list2()->get(n);
    }

    Ref<const InitList> sublist(unsigned n) const {
        return const_cast<InitList*>(this)->sublist(n);
    }

    Ref<Expr> expr(unsigned n) {
        assert(is_flat());
        return expr_list()->get(n);
    }

    Ref<const Expr> expr(unsigned n) const {
        return const_cast<InitList*>(this)->expr(n);
    }

    unsigned child_num() const override {
        return std::max(expr_list()->child_num(), list2()->child_num());
    }

    Ref<Node> child(unsigned n) override {
        return is_flat() ? expr_list()->child(n) : list2()->child(n);
    }

    Ref<const Node> child(unsigned n) const override {
        return const_cast<InitList*>(this)->child(n);
    }
};

class VarDecl : public Tuple<Stmt, ExprList, Expr, InitList>,
                public Named,
                public DefNode {
    ULAM_AST_REF_ATTR(Var, var)
    ULAM_AST_REF_ATTR(Prop, prop)
    ULAM_AST_SIMPLE_ATTR(bool, is_const, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_ref, false)
public:
    VarDecl(
        Str name,
        Ptr<ExprList>&& array_dims,
        Ptr<Expr>&& init_value,
        Ptr<InitList>&& init_list):
        Tuple{
            std::move(array_dims), std::move(init_value), std::move(init_list)},
        Named{name} {
        assert(!init_value || !init_list);
    }

    ULAM_AST_TUPLE_PROP(array_dims, 0)
    ULAM_AST_TUPLE_PROP(init_value, 1)
    ULAM_AST_TUPLE_PROP(init_list, 2)
};

} // namespace ulam::ast
