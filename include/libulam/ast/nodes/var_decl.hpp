#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/init.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

namespace ulam::ast {

class VarDecl : public Tuple<Stmt, ExprList, InitValue>,
                public Named,
                public DefNode {
    ULAM_AST_REF_ATTR(Var, var)
    ULAM_AST_REF_ATTR(Prop, prop)
    ULAM_AST_SIMPLE_ATTR(bool, is_const, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_ref, false)
public:
    VarDecl(Str name, Ptr<ExprList>&& array_dims, Ptr<InitValue>&& init):
        Tuple{std::move(array_dims), std::move(init)}, Named{name} {}

    ULAM_AST_TUPLE_PROP(array_dims, 0)
    ULAM_AST_TUPLE_PROP(init, 1)
};

} // namespace ulam::ast
