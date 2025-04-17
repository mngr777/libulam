#include "./expr_visitor.hpp"
#include "../type_str.hpp"
#include "./expr_flags.hpp"
#include "./expr_res.hpp"
#include "libulam/semantic/type/builtin_type_id.hpp"
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/type_ops.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalExprVisitor] "
#endif
#include "src/debug.hpp"

#define DBG_LINE(node) debug() << _program->sm().line_at(node->loc_id())

namespace {
constexpr char FuncallPh[] = "{args}{fun}";
}

EvalExprVisitor::ExprRes
EvalExprVisitor::visit(ulam::Ref<ulam::ast::BoolLit> node) {
    auto res = ulam::sema::EvalExprVisitor::visit(node);
    res.set_data(std::string{node->value() ? "true" : "false"});
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::visit(ulam::Ref<ulam::ast::NumLit> node) {
    auto res = ulam::sema::EvalExprVisitor::visit(node);

    const auto& num = node->value();
    ulam::Number dec_num;
    if (num.is_signed()) {
        dec_num = {ulam::Radix::Decimal, num.value<ulam::Integer>()};
    } else {
        dec_num = {ulam::Radix::Decimal, num.value<ulam::Unsigned>()};
    }
    res.set_data(dec_num.str());
    res.set_flag(exp::NumLit);
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::visit(ulam::Ref<ulam::ast::StrLit> node) {
    auto res = ulam::sema::EvalExprVisitor::visit(node);
    res.set_data(std::string{text(node->value().id)});
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::type_op(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ulam::Ref<ulam::Type> type) {
    auto res = ulam::sema::EvalExprVisitor::type_op(node, type);
    if (res.value().is_consteval()) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            res.set_data(_stringifier.stringify(res.type(), rval));
        });
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::type_op_expr_default(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ExprRes&& arg) {
    auto data = exp::data(arg);
    auto res = Base::type_op_expr_default(node, std::move(arg));
    if (res.type()->is_prim() && res.value().is_consteval()) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            res.set_data(_stringifier.stringify(res.type(), rval));
        });
    } else {
        exp::set_data(res, data);
        exp::append(res, ulam::ops::str(node->op()));
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::apply_binary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ulam::LValue lval,
    ulam::Ref<ulam::ast::Expr> l_node,
    EvalExprVisitor::ExprRes&& left,
    ulam::Ref<ulam::ast::Expr> r_node,
    EvalExprVisitor::ExprRes&& right) {

    auto l_type = left.type()->actual();
    auto r_type = right.type()->actual();

    switch (ulam::ops::kind(op)) {
    case ulam::ops::Kind::Assign:
        if (r_type != l_type)
            exp::add_cast(right);
        break;
    case ulam::ops::Kind::Equality:
    case ulam::ops::Kind::Comparison:
        // cast right arg to exact type for comparison
        if (r_type == l_type)
            break;
        if (l_type->is_prim() && ulam::has_bitsize(l_type->bi_type_id())) {
            assert(r_type->is(l_type->bi_type_id()));
            if (l_type->bitsize() < r_type->bitsize()) {
                exp::add_cast(left);
            } else {
                exp::add_cast(right);
            }
        } else {
            exp::add_cast(right);
        }
        break;
    case ulam::ops::Kind::Numeric: {
        if (ulam::ops::is_assign(op)) {
            if (r_type != l_type)
                exp::add_cast(right);

        } else if (!l_type->is_class()) {
            // cast to 32 or 64 common bit width
            auto l_size = l_type->bitsize();
            auto r_size = r_type->bitsize();
            auto size = std::max(l_size, r_size);
            assert(size < 64);
            size = (size > 32) ? 64 : 32;
            if (l_size != size)
                exp::add_cast(left);
            if (r_size != size)
                exp::add_cast(right);
        }
        break;
    }
    default: {
    } // do nothing
    }

    std::string op_str{ulam::ops::str(op)};
    if (op == ulam::Op::Sum || op == ulam::Op::Diff)
        op_str += "b";
    auto data = exp::data_combine(exp::data(left), exp::data(right), op_str);

    auto res = ulam::sema::EvalExprVisitor::apply_binary_op(
        node, op, lval, l_node, std::move(left), r_node, std::move(right));

    const auto& val = res.value();
    if (val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            res.set_data(_stringifier.stringify(res.type(), rval));
        });
    } else {
        res.set_data(data);
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::apply_unary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ulam::LValue lval,
    ulam::Ref<ulam::ast::Expr> arg_node,
    EvalExprVisitor::ExprRes&& arg,
    ulam::Ref<ulam::Type> type) {

    auto data = exp::data(arg);
    if (arg.has_flag(exp::NumLit) &&
        (op == ulam::Op::UnaryMinus || op == ulam::Op::UnaryPlus)) {
        // +<num> | -<num>
        std::string op_str{ulam::ops::str(op)};
        data = op_str + data;

    } else if (op == ulam::Op::PreInc || op == ulam::Op::PreDec) {
        // x 1 [cast] += | x 1 [cast] -=
        std::string op_str{(op == ulam::Op::PreInc) ? "+=" : "-="};
        std::string inc_str{arg.type()->is(ulam::IntId) ? "1" : "1 cast"};
        data = exp::data_combine(data, inc_str, op_str);

    } else {
        // x <op> | x Type is
        std::string op_str{ulam::ops::str(op)};
        if (type)
            data = exp::data_combine(data, type_str(type));
        data = exp::data_combine(data, op_str);
    }

    auto res = ulam::sema::EvalExprVisitor::apply_unary_op(
        node, op, lval, arg_node, std::move(arg), type);

    res.set_data(data);
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::ident_self(ulam::Ref<ulam::ast::Ident> node) {
    auto res = ulam::sema::EvalExprVisitor::ident_self(node);
    exp::set_self(res);
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::ident_super(ulam::Ref<ulam::ast::Ident> node) {
    auto res = ulam::sema::EvalExprVisitor::ident_super(node);
    exp::set_data(res, "super");
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::ident_var(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Var> var) {
    auto res = ulam::sema::EvalExprVisitor::ident_var(node, var);
    exp::set_data(res, str(var->name_id()));
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::ident_prop(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Prop> prop) {
    auto res = ulam::sema::EvalExprVisitor::ident_prop(node, prop);
    exp::set_self(res);
    exp::add_member_access(res, str(prop->name_id()), true);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::ident_fset(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::FunSet> fset) {
    auto res = ulam::sema::EvalExprVisitor::ident_fset(node, fset);
    exp::set_self(res);
    exp::add_member_access(res, FuncallPh, true);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_class(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    bool before_member_access = obj.has_flag(exp::MemberAccess);
    if (before_member_access)
        exp::remove_member_access_op(obj);

    auto res = ulam::sema::EvalExprVisitor::array_access_class(
        node, std::move(obj), std::move(idx));

    if (before_member_access)
        exp::append(res, ".");
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_string(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    auto data = exp::data(obj);
    bool before_member_access = obj.has_flag(exp::MemberAccess);

    auto res = ulam::sema::EvalExprVisitor::array_access_string(
        node, std::move(obj), std::move(idx));

    exp::set_data(res, std::move(data));
    exp::add_array_access(res, exp::data(idx), before_member_access);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_array(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    auto data = exp::data(obj);
    auto idx_data = exp::data(idx);
    bool before_member_access = obj.has_flag(exp::MemberAccess);

    auto res = ulam::sema::EvalExprVisitor::array_access_array(
        node, std::move(obj), std::move(idx));

    exp::set_data(res, std::move(data));
    exp::add_array_access(res, idx_data, before_member_access);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_op(
    ulam::Ref<ulam::ast::MemberAccess> node, EvalExprVisitor::ExprRes&& obj) {
    if (!obj)
        return std::move(obj);
    auto data = exp::data(obj);

    auto res =
        ulam::sema::EvalExprVisitor::member_access_op(node, std::move(obj));

    auto op_str = ulam::ops::str(node->op());
    exp::set_data(res, data);
    exp::append(res, exp::data_combine("operator", op_str, "."));
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_var(
    ulam::Ref<ulam::ast::MemberAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    ulam::Ref<ulam::Var> var) {
    if (!obj)
        return std::move(obj);
    auto data = exp::data(obj);
    bool is_self = obj.has_flag(exp::Self);

    auto res = ulam::sema::EvalExprVisitor::member_access_var(
        node, std::move(obj), var);

    auto name = str(var->name_id());
    exp::set_data(res, data);
    exp::add_member_access(res, name, is_self);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_prop(
    ulam::Ref<ulam::ast::MemberAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    ulam::Ref<ulam::Prop> prop) {
    if (!obj)
        return std::move(obj);
    auto data = exp::data(obj);
    bool is_self = obj.has_flag(exp::Self);

    auto res = ulam::sema::EvalExprVisitor::member_access_prop(
        node, std::move(obj), prop);

    auto name = str(prop->name_id());
    exp::set_data(res, data);
    exp::add_member_access(res, name, is_self);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_fset(
    ulam::Ref<ulam::ast::MemberAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    ulam::Ref<ulam::FunSet> fset) {
    if (!obj)
        return std::move(obj);
    auto data = exp::data(obj);
    bool is_self = obj.has_flag(exp::Self);

    auto res = ulam::sema::EvalExprVisitor::member_access_fset(
        node, std::move(obj), fset);

    exp::set_data(res, data);
    exp::add_member_access(res, FuncallPh, is_self);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::bind(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Ref<ulam::FunSet> fset,
    ulam::sema::ExprRes&& obj) {
    assert(obj);
    auto data = exp::data(obj);
    bool is_self = obj.has_flag(exp::Self);

    auto res = ulam::sema::EvalExprVisitor::bind(node, fset, std::move(obj));

    exp::set_data(res, data);
    exp::add_member_access(res, FuncallPh, is_self);
    return res;
}
