#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::sema {

class Cast {
public:
    Cast(Ref<Program> program): _program{program} {}

    ExprRes cast(ExprRes&& res, Ref<ast::Node> node, Ref<Type> type, bool is_impl);

    ExprRes to_boolean(ExprRes&& res, Ref<ast::Node> node, bool is_impl);

private:
    Ref<Program> _program;
};

} // namespace ulam::sema
