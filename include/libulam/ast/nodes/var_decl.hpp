#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

namespace ulam::ast {

class VarDecl : public Tuple<Stmt, ExprList, Expr>,
                public Named,
                public DefNode {
    ULAM_AST_REF_ATTR(Var, var)
    ULAM_AST_SIMPLE_ATTR(bool, is_ref, false)
    ULAM_AST_SIMPLE_ATTR(loc_id_t, assign_loc_id, NoLocId)
public:
    VarDecl(Str name, Ptr<ExprList>&& array_dims, Ptr<Expr>&& default_value):
        Tuple{std::move(array_dims), std::move(default_value)}, Named{name} {}

    ULAM_AST_TUPLE_PROP(array_dims, 0)
    ULAM_AST_TUPLE_PROP(default_value, 1)
};

} // namespace ulam::ast
