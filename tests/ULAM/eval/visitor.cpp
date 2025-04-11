#include "./visitor.hpp"
#include "../type_str.hpp"
#include "./cast.hpp"
#include "./expr_visitor.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include <libulam/sema/eval/except.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalVisitor] "
#endif
#include "src/debug.hpp"

// TODO: throw when missing data

void EvalVisitor::visit(ulam::Ref<ulam::ast::Block> node) {
    if (in_main())
        append("{");
    ulam::sema::EvalVisitor::visit(node);
    if (in_main())
        append("}");
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Return> node) {
    auto res = ret_res(node);
    if (in_main()) {
        if (res.has_data()) {
            append(res.data<std::string>());
            append("return");
        }
    }
    throw ulam::sema::EvalExceptReturn(node, std::move(res));
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::ExprStmt> node) {
    debug() << __FUNCTION__ << " ExprStmt\n";
    if (!node->has_expr())
        return;
    auto res = eval_expr(node->expr());
    if (in_main()) {
        if (res.has_data())
            append(res.data<std::string>());
    }
}

ulam::Ptr<ulam::sema::EvalExprVisitor>
EvalVisitor::expr_visitor(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalExprVisitor>(*this, _stringifier, scope);
}

ulam::Ptr<ulam::sema::EvalInit>
EvalVisitor::init_helper(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalInit>(*this, diag(), _program->str_pool(), scope);
}

ulam::Ptr<ulam::sema::EvalCast>
EvalVisitor::cast_helper(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalCast>(*this, diag(), scope);
}

ulam::Ptr<ulam::sema::EvalFuncall>
EvalVisitor::funcall_helper(ulam::Ref<ulam::Scope> scope) {
    return ulam::make<EvalFuncall>(*this, diag(), _program->str_pool(), scope);
}

ulam::Ref<ulam::AliasType>
EvalVisitor::type_def(ulam::Ref<ulam::ast::TypeDef> node) {
    auto type = ulam::sema::EvalVisitor::type_def(node);
    if (type && in_main()) {
        auto aliased = type->aliased();
        append("typedef");
        append(type_base_name(aliased));
        append(type->name() + type_dim_str(aliased) + "; ");
    }
    return type;
}

ulam::Ref<ulam::Var> EvalVisitor::var_def(
    ulam::Ref<ulam::ast::TypeName> type_name,
    ulam::Ref<ulam::ast::VarDef> node) {
    auto var = ulam::sema::EvalVisitor::var_def(type_name, node);
    if (var && in_main()) {
        std::string name{_program->str_pool().get(var->name_id())};
        append(type_base_name(var->type()));
        append(name + type_dim_str(var->type()) + "; ");
    }
    return var;
}

ulam::sema::ExprRes EvalVisitor::eval_expr(ulam::Ref<ulam::ast::Expr> expr) {
    auto res = ulam::sema::EvalVisitor::eval_expr(expr);
    assert(res);
    debug() << "expr: "
            << (res.has_data() ? res.data<std::string>()
                               : std::string{"no data"})
            << "\n";
    return res;
}

void EvalVisitor::append(std::string data) {
    if (!_data.empty())
        _data += " ";
    _data += std::move(data);
}
