#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

namespace ulam::ast {

class VarDecl : public Tuple<Stmt, Expr>, public Named {
    ULAM_AST_PTR_ATTR(Var, var)
    ULAM_AST_SIMPLE_ATTR(bool, is_const, false)
public:
    VarDecl(Str name, Ptr<Expr>&& value):
        Tuple{std::move(value)}, Named{name} {}

    ULAM_AST_TUPLE_PROP(value, 0);
};

} // namespace ulam::ast
