#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/ast/str.hpp>
#include <libulam/src_loc.hpp>
#include <utility>

namespace ulam::ast {

class Param : public Tuple<VarDecl, TypeName> {
    ULAM_AST_NODE
public:
    Param(
        Str name,
        Ptr<TypeName>&& type_name,
        Ptr<ExprList>&& array_dims,
        Ptr<Expr>&& init_value,
        Ptr<InitList>&& init_list):
        Tuple{
            std::move(type_name), name, std::move(array_dims),
            std::move(init_value), std::move(init_list)} {}

    ULAM_AST_TUPLE_PROP(type_name, 0)

    unsigned child_num() const override { return VarDecl::child_num() + 1; }

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : VarDecl::child(n - 1);
    }

    Ref<const Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : VarDecl::child(n - 1);
    }
};

class ParamList : public List<Node, Param> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(loc_id_t, ellipsis_loc_id, NoLocId)
public:
    bool has_ellipsis() const { return ellipsis_loc_id() != NoLocId; }
};

class ArgList : public List<Node, Expr> {
    ULAM_AST_NODE
};

} // namespace ulam::ast
