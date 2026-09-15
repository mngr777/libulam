#include "./expr_visitor.hpp"
#include "../out.hpp"
#include "../utils.hpp"
#include "./expr_flags.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include "./util.hpp"
#include "libulam/semantic/value/flags.hpp"
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalExprVisitor] "
#endif
#include "src/debug.hpp"

#define DBG_LINE(node) debug() << _program->src_man().line_at(node->loc_id())

namespace {

using ExprRes = EvalExprVisitor::ExprRes;

constexpr char FuncallPh[] = "{args}{fun}";

} // namespace

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::Cast> node) {
    auto res = Base::visit(node);
    if (!has_flag(eval::NoCodegen)) {
        if (!res.has_flag(expr::ExplCast) && !res.has_flag(expr::ImplCast))
            expr::add_cast(res, true);
    }
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::Ternary> node) {
    if (has_flag(eval::NoCodegen))
        return Base::visit(node);

    auto cond_res = ternary_eval_cond(node);
    if (!cond_res)
        return cond_res;

    auto [if_true_res, if_false_res] = ternary_eval_branches_noexec(node);
    if (!if_true_res)
        return std::move(if_true_res);
    if (!if_false_res)
        return std::move(if_false_res);

    auto type = common_type(if_true_res, if_false_res);
    if (!type) {
        diag().error(node, "no common type");
        return {ExprError::TernaryNonMatchingTypes};
    }

    {
        auto fr = env().add_flags_raii(ulam::sema::eval::NoDerefCast);
        if_true_res = env().cast(node->if_true(), type, std::move(if_true_res));
        if_false_res =
            env().cast(node->if_false(), type, std::move(if_false_res));
    }
    if (!if_true_res)
        return std::move(if_true_res);
    if (!if_false_res)
        return std::move(if_false_res);
    ulam_assert(if_true_res.type()->is_same(if_false_res.type()));

    auto cond_data = expr::data(cond_res);
    auto if_true_data = expr::data(if_true_res);
    auto if_false_data = expr::data(if_false_res);

    auto res = ternary_eval(
        node, std::move(cond_res), type, std::move(if_true_res),
        std::move(if_false_res));

    expr::append(res, cond_data);
    expr::append(res, "? ");
    expr::append(res, if_true_data);
    expr::append(res, ": ");
    expr::append(res, if_false_data);
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::BoolLit> node) {
    auto res = Base::visit(node);
    if (!has_flag(eval::NoCodegen))
        expr::set_data(res, std::string{node->value() ? "true" : "false"});
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::NumLit> node) {
    auto res = Base::visit(node);
    if (has_flag(eval::NoCodegen))
        return res;

    ulam_assert(res.value().is_consteval());
    res.value().with_rvalue([&](const auto& rval) {
        auto strf = gen().make_strf();
        strf.options.unary_as_unsigned_lit = true;
        strf.options.bool_as_unsigned_lit = true;
        expr::set_data(res, strf.stringify(res.type(), rval));
    });
    res.set_flag(expr::NumLit);
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::StrLit> node) {
    auto res = Base::visit(node);
    if (has_flag(eval::NoCodegen))
        return res;

    res.value().with_rvalue([&](const auto& rval) {
        auto strf = gen().make_strf();
        expr::set_data(res, strf.stringify(res.type(), rval));
    });
    return res;
}

ExprRes EvalExprVisitor::apply_binary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ExprRes&& lval_res,
    ulam::Ref<ulam::ast::Expr> l_node,
    ExprRes&& left,
    ulam::Ref<ulam::ast::Expr> r_node,
    ExprRes&& right) {
    if (has_flag(eval::NoCodegen)) {
        return Base::apply_binary_op(
            node, op, std::move(lval_res), l_node, std::move(left), r_node,
            std::move(right));
    }

    auto l_type = left.type()->actual();
    auto r_type = right.type()->actual();
    bool l_is_ref = left.type()->is_ref();
    bool r_is_ref = right.type()->is_ref();

    bool is_class_op = false;
    if (l_type->is_class()) {
        auto cls = l_type->as_class();
        is_class_op = cls->has_op(op);
        if (!is_class_op) {
            auto neg_op = ulam::ops::negation(op);
            if (neg_op != ulam::Op::None &&
                program()->eval_options().implicit_class_negation_op) {
                is_class_op = cls->has_op(neg_op);
            }
        }
    }
    if (is_class_op) {
        // class op is a funcall, no need to update data
        return Base::apply_binary_op(
            node, op, std::move(lval_res), l_node, std::move(left), r_node,
            std::move(right));
    }

    bool no_fold = has_flag(eval::NoConstFold) ||
                   left.has_flag(expr::NoConstFold) ||
                   right.has_flag(expr::NoConstFold);

    switch (ulam::ops::kind(op)) {
    case ulam::ops::Kind::Assign:
        if (r_type != l_type && !right.value().is_consteval())
            expr::add_cast(right);
        break;
    case ulam::ops::Kind::Equality:
    case ulam::ops::Kind::Comparison:
        if (l_is_ref || r_is_ref) {
            if (l_is_ref != r_is_ref) {
                // cast an arg to exact type for comparison
                // deref, t3695
                expr::add_cast(l_is_ref ? left : right);
            } else {
                expr::add_cast(left);
                expr::add_cast(right);
            }
        }

        if (l_type->is(ulam::BoolId) && r_type->is(ulam::BoolId)) {
            // Bool types are converted to Bool(1)
            if (l_type->bitsize() != 1)
                expr::add_cast(left);
            if (r_type->bitsize() != 1)
                expr::add_cast(right);
            break;
        }

        if (l_type == r_type)
            break;

        if (l_type->is_prim() && ulam::has_bitsize(l_type->bi_type_id())) {
            ulam_assert(r_type->is(l_type->bi_type_id()));
            if (l_type->bitsize() < r_type->bitsize()) {
                expr::add_cast(left);
            } else {
                expr::add_cast(right);
            }
        } else {
            expr::add_cast(right);
        }
        break;
    case ulam::ops::Kind::Numeric: {
        if (ulam::ops::is_assign(op)) {
            if (r_type != l_type)
                expr::add_cast(right);

        } else if (!l_type->is_class()) {
            // cast to 32 or 64 common bit width
            auto l_size = l_type->bitsize();
            auto r_size = r_type->bitsize();
            auto size = std::max(l_size, r_size);
            ulam_assert(size <= 64);
            size = (size > 32) ? 64 : 32;
            if (l_size != size)
                expr::add_cast(left);
            if (r_size != size)
                expr::add_cast(right);
        }
        break;
    }
    default: {
    } // do nothing
    }

    std::string op_str{ulam::ops::str(op)};
    if (op == ulam::Op::Sum || op == ulam::Op::Diff)
        op_str += "b";
    auto data = expr::data_combine(expr::data(left), expr::data(right), op_str);

    auto res = Base::apply_binary_op(
        node, op, std::move(lval_res), l_node, std::move(left), r_node,
        std::move(right));

    const auto& val = res.value();
    if (!no_fold && val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            auto strf = gen().make_strf();
            expr::set_data(res, strf.stringify(res.type(), rval));
        });
    } else if (!empty(data)) {
        expr::set_data(res, data);
    }
    if (no_fold)
        res.set_flag(expr::NoConstFold);
    return res;
}

ExprRes EvalExprVisitor::apply_unary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ExprRes&& lval_res,
    ulam::Ref<ulam::ast::Expr> arg_node,
    ExprRes&& arg,
    ulam::Ref<ulam::Type> type) {
    if (has_flag(eval::NoCodegen)) {
        return Base::apply_unary_op(
            node, op, std::move(lval_res), arg_node, std::move(arg), type);
    }

    auto arg_type = arg.type()->actual();
    bool is_class_op = arg_type->is_class() && arg_type->as_class()->has_op(op);
    if (is_class_op) {
        // class op is a funcall, no need to update data
        return Base::apply_unary_op(
            node, op, std::move(lval_res), arg_node, std::move(arg), type);
    }

    auto data = expr::data(arg);
    bool no_fold = has_flag(eval::NoConstFold) || arg.has_flag(expr::NoConstFold);

    if (arg.has_flag(expr::NumLit) &&
        (op == ulam::Op::UnaryMinus || op == ulam::Op::UnaryPlus)) {
        // +<num> | -<num>
        std::string op_str{ulam::ops::str(op)};
        data = op_str + data;

    } else if (op == ulam::Op::PreInc || op == ulam::Op::PreDec) {
        // x 1 [cast] += | x 1 [cast] -=
        bool is_int = arg.type()->deref()->is_same(builtins().int_type());
        std::string inc_str{is_int ? "1" : "1 cast"};
        std::string op_str{(op == ulam::Op::PreInc) ? "+=" : "-="};
        data = expr::data_combine(data, inc_str, op_str);

    } else if (op == ulam::Op::PostInc || op == ulam::Op::PostDec) {
        // x 1 [cast] ++ | x 1 [cast] --
        bool is_int = arg.type()->deref()->is_same(builtins().int_type());
        std::string inc_str{is_int ? "1" : "1 cast"};
        std::string op_str{ulam::ops::str(op)};
        data = expr::data_combine(data, inc_str, op_str);

    } else {
        // x <op> | x Type is
        std::string op_str{ulam::ops::str(op)};
        if (type) {
            auto strf = gen().make_strf();
            data = expr::data_combine(data, out::type_str(strf, type));
        }
        data = expr::data_combine(data, op_str);
    }

    auto res = Base::apply_unary_op(
        node, op, std::move(lval_res), arg_node, std::move(arg), type);
    const auto& val = res.value();
    if (!no_fold && val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            auto strf = gen().make_strf();
            expr::set_data(res, strf.stringify(res.type(), rval));
        });
    } else {
        expr::set_data(res, data);
    }
    if (no_fold)
        res.set_flag(expr::NoConstFold);
    return res;
}

ExprRes EvalExprVisitor::post_inc_dec_dummy() {
    auto res = Base::post_inc_dec_dummy();
    if (!has_flag(eval::NoCodegen))
        expr::set_data(res, "1");
    return res;
}

ExprRes EvalExprVisitor::type_op_construct(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ulam::Ref<ulam::Class> cls) {
    auto res = Base::type_op_construct(node, cls);
    if (!has_flag(eval::NoCodegen)) {
        // Class.instanceof ( args )Self .
        auto strf = gen().make_strf();
        auto op_str = out::type_str(strf, cls) + ".instanceof";
        expr::set_data(res, expr::data_combine(op_str, expr::data(res), "."));
    }
    return res;
}

ExprRes EvalExprVisitor::type_op_default(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ulam::Ref<ulam::Type> type) {
    auto res = Base::type_op_default(node, type);
    if (has_flag(eval::NoCodegen))
        return res;

    auto strf = gen().make_strf();
    if (res.type()->is_prim() && res.value().is_consteval()) {
        res.value().with_rvalue([&](const auto& rval) {
            strf.options.unary_as_unsigned_lit = true;
            strf.options.bool_as_unsigned_lit = true;
            expr::set_data(res, strf.stringify(res.type(), rval));
        });
    } else {
        expr::append(res, out::type_str(strf, type));
        expr::append(res, std::string{"."} + ulam::ops::str(node->op()), "");
    }
    return res;
}

ExprRes EvalExprVisitor::type_op_expr_construct(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ExprRes&& arg) {
    std::string data;
    if (!has_flag(eval::NoCodegen))
        data = expr::data(arg);
    auto res = Base::type_op_expr_construct(node, std::move(arg));
    if (!data.empty()) {
        auto op_str = data + ".instanceof";
        expr::set_data(res, expr::data_combine(op_str, expr::data(res), "."));
    }
    return res;
}

ExprRes EvalExprVisitor::type_op_expr_default(
    ulam::Ref<ulam::ast::TypeOpExpr> node,
    ExprRes&& arg,
    ulam::Ref<ulam::Class> base) {

    std::string data;
    if (!has_flag(eval::NoCodegen)) {
        if (node->op() == ulam::TypeOp::AtomOf &&
            !arg.type()->deref()->is_atom()) {
            // NOTE: a hack to remove _single_ redundand member access before
            // calling
            // `.atomof`, t3905
            if (arg.has_flag(expr::MemberAccess) ||
                arg.has_flag(expr::SelfMemberAccess))
                expr::remove_member_access_op(arg, true);
        }
        data = expr::data(arg);
    }
    auto res = Base::type_op_expr_default(node, std::move(arg), base);

    if (!data.empty()) {
        if (util::can_fold(res)) {
            res.value().with_rvalue([&](const ulam::RValue& rval) {
                auto strf = gen().make_strf();
                strf.options.unary_as_unsigned_lit = true;
                strf.options.bool_as_unsigned_lit = true;
                expr::set_data(res, strf.stringify(res.type(), rval));
            });
        } else {
            expr::set_data(res, data);
            expr::append(res, std::string{"."} + ulam::ops::str(node->op()), "");
        }
    }
    return res;
}

ExprRes EvalExprVisitor::ident_self(ulam::Ref<ulam::ast::Ident> node) {
    auto res = Base::ident_self(node);
    if (!has_flag(eval::NoCodegen))
        expr::set_self(res);
    return res;
}

ExprRes EvalExprVisitor::ident_super(ulam::Ref<ulam::ast::Ident> node) {
    auto res = Base::ident_super(node);
    if (!has_flag(eval::NoCodegen))
        expr::set_data(res, "super");
    return res;
}

ExprRes EvalExprVisitor::ident_var(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Var> var) {
    auto res = Base::ident_var(node, var);
    if (has_flag(eval::NoCodegen))
        return res;

    bool no_fold = has_flag(eval::NoConstFold);
    if (!no_fold && util::can_fold(res)) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            auto strf = gen().make_strf();
            strf.options.unary_as_unsigned_lit = true;
            strf.options.bool_as_unsigned_lit = true;
            expr::set_data(res, strf.stringify(res.type(), rval));
        });
    } else {
        expr::set_data(res, str(var->name_id()));
    }
    return res;
}

ExprRes EvalExprVisitor::ident_prop(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Prop> prop) {
    auto res = Base::ident_prop(node, prop);
    if (!has_flag(eval::NoCodegen)) {
        expr::set_self(res);
        expr::add_member_access(res, str(prop->name_id()), true);
    }
    return res;
}

ExprRes EvalExprVisitor::ident_fset(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::FunSet> fset) {
    auto res = Base::ident_fset(node, fset);
    if (!has_flag(eval::NoCodegen)) {
        expr::set_self(res);
        expr::add_member_access(res, FuncallPh, true);
    }
    return res;
}

ExprRes EvalExprVisitor::array_access_class(
    ulam::Ref<ulam::ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    bool before_member_access = false;
    if (!has_flag(eval::NoCodegen)) {
        before_member_access = obj.has_flag(expr::MemberAccess);
        if (before_member_access)
            expr::remove_member_access_op(obj);
    }

    auto res = Base::array_access_class(node, std::move(obj), std::move(idx));
    if (before_member_access)
        expr::append(res, ".");
    return res;
}

ExprRes EvalExprVisitor::array_access_string(
    ulam::Ref<ulam::ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    std::string data;
    bool before_member_access = false;
    if (!has_flag(eval::NoCodegen)) {
        data = expr::data(obj);
        before_member_access = obj.has_flag(expr::MemberAccess);
    }
    auto res = Base::array_access_string(node, std::move(obj), std::move(idx));
    if (!data.empty()) {
        expr::set_data(res, std::move(data));
        expr::add_array_access(res, expr::data(idx), before_member_access);
        res.set_flag(expr::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::array_access_array(
    ulam::Ref<ulam::ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    std::string data, idx_data;
    bool before_member_access = false;
    if (!has_flag(eval::NoCodegen)) {
        idx_data = expr::data(idx);
        data = expr::data(obj);
        before_member_access = obj.has_flag(expr::MemberAccess);
    }
    auto res = Base::array_access_array(node, std::move(obj), std::move(idx));
    if (!data.empty()) {
        expr::set_data(res, std::move(data));
        expr::add_array_access(res, idx_data, before_member_access);
        res.set_flag(expr::NoConstFold); // do not fold result of [], t3881
    }
    return res;
}

ExprRes EvalExprVisitor::member_access_var(
    ulam::Ref<ulam::ast::MemberAccess> node,
    ExprRes&& obj,
    ulam::Ref<ulam::Var> var) {
    if (!obj)
        return std::move(obj);

    std::string data;
    bool no_fold = false;
    bool is_self = false;
    if (!has_flag(eval::NoCodegen)) {
        data = expr::data(obj);
        no_fold = has_flag(eval::NoConstFold) || obj.has_flag(expr::NoConstFold);
        is_self = obj.has_flag(expr::Self);
    }
    auto res = Base::member_access_var(node, std::move(obj), var);
    if (!data.empty()) {
        expr::set_data(res, data);
        if (!no_fold && util::can_fold(res)) {
            res.value().with_rvalue([&](const ulam::RValue& rval) {
                auto strf = gen().make_strf();
                strf.options.unary_as_unsigned_lit = true;
                strf.options.bool_as_unsigned_lit = true;
                auto val_str = strf.stringify(res.type(), rval);
                if (res.type()->is(ulam::StringId)) {
                    // t41273
                    expr::set_data(res, val_str);
                } else {
                    expr::add_member_access(res, val_str, is_self);
                }
                res.set_flag(expr::NoConstFold); // e.g. t41221
            });
        } else {
            auto name = str(var->name_id());
            expr::add_member_access(res, name, is_self);
        }
        if (no_fold)
            res.set_flag(expr::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::member_access_prop(
    ulam::Ref<ulam::ast::MemberAccess> node,
    ExprRes&& obj,
    ulam::Ref<ulam::Prop> prop) {

    if (!obj)
        return std::move(obj);

    std::string data;
    bool no_fold = false;
    bool is_self = false;
    if (!has_flag(eval::NoCodegen)) {
        data = expr::data(obj);
        no_fold = has_flag(eval::NoConstFold) || obj.has_flag(expr::NoConstFold);
        is_self = obj.has_flag(expr::Self);
    }
    auto res = Base::member_access_prop(node, std::move(obj), prop);
    if (!data.empty()) {
        auto name = str(prop->name_id());
        expr::set_data(res, data);
        expr::add_member_access(res, name, is_self);
        if (no_fold)
            res.set_flag(expr::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::member_access_fset(
    ulam::Ref<ulam::ast::MemberAccess> node,
    ExprRes&& obj,
    ulam::Ref<ulam::FunSet> fset,
    ulam::Ref<ulam::Class> base) {
    if (!obj)
        return std::move(obj);

    std::string data;
    bool no_fold = false;
    bool is_self = false;
    if (!has_flag(eval::NoCodegen)) {
        data = expr::data(obj);
        no_fold = has_flag(eval::NoConstFold) || obj.has_flag(expr::NoConstFold);
        is_self = obj.has_flag(expr::Self);
    }
    auto res = Base::member_access_fset(node, std::move(obj), fset, base);
    if (!data.empty()) {
        expr::set_data(res, data);
        expr::add_member_access(res, FuncallPh, is_self);
        if (no_fold)
            res.set_flag(expr::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::class_const_access(
    ulam::Ref<ulam::ast::ClassConstAccess> node, ulam::Ref<ulam::Var> var) {
    std::string data;
    auto res = Base::class_const_access(node, var);
    if (!has_flag(eval::NoCodegen)) {
        auto type = var->type();
        if (!util::can_fold(type)) {
            expr::set_data(res, str(var->name_id()));
            res.set_flag(expr::NoConstFold);
        } else {
            ulam_assert(!res.value().empty());
            ulam_assert(res.value().is_consteval());
            res.value().with_rvalue([&](const auto& rval) {
                auto strf = gen().make_strf();
                expr::set_data(res, strf.stringify(type, rval));
            });
        }
    }
    return res;
}

ExprRes EvalExprVisitor::bind(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Ref<ulam::FunSet> fset,
    ulam::sema::ExprRes&& obj,
    ulam::Ref<ulam::Class> base) {
    ulam_assert(obj);
    std::string data;
    bool is_self = false;
    if (!has_flag(eval::NoCodegen)) {
        data = expr::data(obj);
        is_self = obj.has_flag(expr::Self);
    }
    auto res = Base::bind(node, fset, std::move(obj), base);
    if (!data.empty()) {
        expr::set_data(res, data);
        expr::add_member_access(res, FuncallPh, is_self);
    }
    return res;
}

ExprRes
EvalExprVisitor::negate(ulam::Ref<ulam::ast::Expr> node, ExprRes&& res) {
    res = Base::negate(node, std::move(res));
    if (!has_flag(eval::NoCodegen)) {
        expr::remove_member_access_op(res);
        expr::append(res, "!");
        expr::append(res, ".");
    }
    return std::move(res);
}

ExprRes EvalExprVisitor::class_name(
    ulam::Ref<ulam::ast::ClassName> node, ulam::Ref<ulam::Class> cls) {
    auto res = (node->kind() == ulam::ClassNameMangled)
                   ? class_name_mangled(node, cls)
                   : Base::class_name(node, cls);
    if (!has_flag(eval::NoCodegen)) {
        ulam_assert(res.type()->is(ulam::StringId));
        ulam_assert(res.value().is_rvalue());
        ulam_assert(res.value().rvalue().is<ulam::String>());
        auto strf = gen().make_strf();
        auto data = strf.stringify(res.type(), res.value().rvalue());
        expr::set_data(res, data);
    }
    return res;
}

ExprRes EvalExprVisitor::class_name_mangled(
    ulam::Ref<ulam::ast::ClassName> node, ulam::Ref<ulam::Class> cls) {
    auto name = ::class_name_mangled(program(), cls);
    auto str_id = text_pool().put(name);
    auto str_type = builtins().string_type();
    auto rval =
        ulam::RValue::make(ulam::String{str_id}, ulam::value::IsConsteval);
    return {str_type, ulam::Value{std::move(rval)}};
}

ulam::Ref<ulam::Class> EvalExprVisitor::class_base_ident(
    ulam::Ref<ulam::ast::Expr> node,
    ExprRes& obj,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::ast::TypeIdent> ident) {
    if (!has_flag(eval::NoCodegen)) {
        expr::append(obj, std::string{str(ident->name_id())});
        expr::append(obj, ".");
    }
    return Base::class_base_ident(node, obj, cls, ident);
}

ulam::Ref<ulam::Class> EvalExprVisitor::class_base_type_spec(
    ulam::Ref<ulam::ast::Expr> node,
    ExprRes& obj,
    ulam::Ref<ulam::Class> cls,
    ulam::Ref<ulam::ast::TypeSpec> type_spec) {
    if (!has_flag(eval::NoCodegen)) {
        // NOTE: ident string only, t41384
        expr::append(obj, std::string{str(type_spec->ident()->name_id())});
        expr::append(obj, ".");
    }
    return Base::class_base_type_spec(node, obj, cls, type_spec);
}

ulam::Ref<ulam::Class> EvalExprVisitor::class_base_classid(
    ulam::Ref<ulam::ast::Expr> expr,
    ExprRes& obj,
    ulam::Ref<ulam::Class> cls,
    ExprRes&& classid) {
    if (!has_flag(eval::NoCodegen)) {
        expr::remove_member_access_op(obj);
        expr::append(obj, expr::data(classid));
        expr::append(obj, ".[]");
    }
    return Base::class_base_classid(expr, obj, cls, std::move(classid));
}
