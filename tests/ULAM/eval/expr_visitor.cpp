#include "./expr_visitor.hpp"
#include "../out.hpp"
#include "./expr_flags.hpp"
#include "./expr_res.hpp"
#include "./flags.hpp"
#include "./stringifier.hpp"
#include "libulam/sema/eval/flags.hpp"
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalExprVisitor] "
#endif
#include "src/debug.hpp"

#define DBG_LINE(node) debug() << _program->sm().line_at(node->loc_id())

namespace {

using ExprRes = EvalExprVisitor::ExprRes;

constexpr char FuncallPh[] = "{args}{fun}";

} // namespace

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::Cast> node) {
    auto res = Base::visit(node);
    if (!has_flag(evl::NoCodegen)) {
        if (!res.has_flag(exp::ExplCast) && !res.has_flag(exp::ImplCast))
            exp::add_cast(res, true);
    }
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::Ternary> node) {
    if (has_flag(evl::NoCodegen))
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

    auto cast_flags = flags() | ulam::sema::evl::NoDerefCast;
    auto cast = eval().cast_helper(scope(), cast_flags);
    if_true_res = cast->cast(node->if_true(), type, std::move(if_true_res));
    if_false_res = cast->cast(node->if_false(), type, std::move(if_false_res));
    if (!if_true_res)
        return std::move(if_true_res);
    if (!if_false_res)
        return std::move(if_false_res);
    assert(if_true_res.type()->is_same(if_false_res.type()));

    auto cond_data = exp::data(cond_res);
    auto if_true_data = exp::data(if_true_res);
    auto if_false_data = exp::data(if_false_res);

    auto res = ternary_eval(
        node, std::move(cond_res), type, std::move(if_true_res),
        std::move(if_false_res));

    exp::append(res, cond_data);
    exp::append(res, "? ");
    exp::append(res, if_true_data);
    exp::append(res, ": ");
    exp::append(res, if_false_data);
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::BoolLit> node) {
    auto res = Base::visit(node);
    if (!has_flag(evl::NoCodegen))
        exp::set_data(res, std::string{node->value() ? "true" : "false"});
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::NumLit> node) {
    auto res = Base::visit(node);
    if (has_flag(evl::NoCodegen))
        return res;

    assert(res.value().is_consteval());
    res.value().with_rvalue([&](const auto& rval) {
        auto stringifier = make_stringifier();
        stringifier.options.unary_as_unsigned_lit = true;
        stringifier.options.bool_as_unsigned_lit = true;
        exp::set_data(res, stringifier.stringify(res.type(), rval));
    });
    res.set_flag(exp::NumLit);
    return res;
}

ExprRes EvalExprVisitor::visit(ulam::Ref<ulam::ast::StrLit> node) {
    auto res = Base::visit(node);
    if (has_flag(evl::NoCodegen))
        return res;

    res.value().with_rvalue([&](const auto& rval) {
        auto stringifier = make_stringifier();
        exp::set_data(res, stringifier.stringify(res.type(), rval));
    });
    return res;
}

ExprRes EvalExprVisitor::apply_binary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ulam::LValue lval,
    ulam::Ref<ulam::ast::Expr> l_node,
    ExprRes&& left,
    ulam::Ref<ulam::ast::Expr> r_node,
    ExprRes&& right) {
    if (has_flag(evl::NoCodegen)) {
        return Base::apply_binary_op(
            node, op, lval, l_node, std::move(left), r_node, std::move(right));
    }

    auto l_type = left.type()->actual();
    auto r_type = right.type()->actual();
    bool l_is_ref = left.type()->is_ref();
    bool r_is_ref = right.type()->is_ref();

    bool is_class_op = l_type->is_class() && l_type->as_class()->has_op(op);
    if (is_class_op) {
        // class op is a funcall, no need to update data
        return Base::apply_binary_op(
            node, op, lval, l_node, std::move(left), r_node, std::move(right));
    }

    bool no_fold =
        left.has_flag(exp::NoConstFold) || right.has_flag(exp::NoConstFold);

    switch (ulam::ops::kind(op)) {
    case ulam::ops::Kind::Assign:
        if (r_type != l_type)
            exp::add_cast(right);
        break;
    case ulam::ops::Kind::Equality:
    case ulam::ops::Kind::Comparison:
        if (l_is_ref || r_is_ref) {
            if (l_is_ref != r_is_ref) {
                // cast an arg to exact type for comparison
                // deref, t3695
                exp::add_cast(l_is_ref ? left : right);
            } else {
                exp::add_cast(left);
                exp::add_cast(right);
            }
        }

        if (l_type->is(ulam::BoolId) && r_type->is(ulam::BoolId)) {
            // Bool types are converted to Bool(1)
            if (l_type->bitsize() != 1)
                exp::add_cast(left);
            if (r_type->bitsize() != 1)
                exp::add_cast(right);
            break;
        }

        if (l_type == r_type)
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
            assert(size <= 64);
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

    auto res = Base::apply_binary_op(
        node, op, lval, l_node, std::move(left), r_node, std::move(right));

    const auto& val = res.value();
    if (!no_fold && val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            exp::set_data(res, make_stringifier().stringify(res.type(), rval));
        });
    } else if (!empty(data)) {
        exp::set_data(res, data);
    }
    if (no_fold)
        res.set_flag(exp::NoConstFold);
    return res;
}

ExprRes EvalExprVisitor::apply_unary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ulam::LValue lval,
    ulam::Ref<ulam::ast::Expr> arg_node,
    ExprRes&& arg,
    ulam::Ref<ulam::Type> type) {
    if (has_flag(evl::NoCodegen)) {
        return Base::apply_unary_op(
            node, op, lval, arg_node, std::move(arg), type);
    }

    auto arg_type = arg.type()->actual();
    bool is_class_op = arg_type->is_class() && arg_type->as_class()->has_op(op);
    if (is_class_op) {
        // class op is a funcall, no need to update data
        return Base::apply_unary_op(
            node, op, lval, arg_node, std::move(arg), type);
    }

    auto data = exp::data(arg);
    bool no_fold = arg.has_flag(exp::NoConstFold);

    if (arg.has_flag(exp::NumLit) &&
        (op == ulam::Op::UnaryMinus || op == ulam::Op::UnaryPlus)) {
        // +<num> | -<num>
        std::string op_str{ulam::ops::str(op)};
        data = op_str + data;

    } else if (op == ulam::Op::PreInc || op == ulam::Op::PreDec) {
        // x 1 [cast] += | x 1 [cast] -=
        bool is_int = arg.type()->deref()->is_same(builtins().int_type());
        std::string inc_str{is_int ? "1" : "1 cast"};
        std::string op_str{(op == ulam::Op::PreInc) ? "+=" : "-="};
        data = exp::data_combine(data, inc_str, op_str);

    } else if (op == ulam::Op::PostInc || op == ulam::Op::PostDec) {
        // x 1 [cast] ++ | x 1 [cast] --
        bool is_int = arg.type()->deref()->is_same(builtins().int_type());
        std::string inc_str{is_int ? "1" : "1 cast"};
        std::string op_str{ulam::ops::str(op)};
        data = exp::data_combine(data, inc_str, op_str);

    } else {
        // x <op> | x Type is
        std::string op_str{ulam::ops::str(op)};
        if (type) {
            auto stringifier = make_stringifier();
            data = exp::data_combine(data, out::type_str(stringifier, type));
        }
        data = exp::data_combine(data, op_str);
    }

    auto res =
        Base::apply_unary_op(node, op, lval, arg_node, std::move(arg), type);
    const auto& val = res.value();
    if (!no_fold && val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            exp::set_data(res, make_stringifier().stringify(res.type(), rval));
        });
    } else {
        exp::set_data(res, data);
    }
    if (no_fold)
        res.set_flag(exp::NoConstFold);
    return res;
}

ExprRes EvalExprVisitor::post_inc_dec_dummy() {
    auto res = Base::post_inc_dec_dummy();
    if (!has_flag(evl::NoCodegen))
        exp::set_data(res, "1");
    return res;
}

ExprRes EvalExprVisitor::type_op_construct(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ulam::Ref<ulam::Class> cls) {
    auto res = Base::type_op_construct(node, cls);
    if (!has_flag(evl::NoCodegen)) {
        // Class.instanceof ( args )Self .
        auto stringifier = make_stringifier();
        auto op_str = out::type_str(stringifier, cls) + ".instanceof";
        exp::set_data(res, exp::data_combine(op_str, exp::data(res), "."));
    }
    return res;
}

ExprRes EvalExprVisitor::type_op_default(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ulam::Ref<ulam::Type> type) {
    auto res = Base::type_op_default(node, type);
    if (has_flag(evl::NoCodegen))
        return res;

    auto stringifier = make_stringifier();
    if (res.type()->is_prim() && res.value().is_consteval()) {
        res.value().with_rvalue([&](const auto& rval) {
            stringifier.options.unary_as_unsigned_lit = true;
            stringifier.options.bool_as_unsigned_lit = true;
            exp::set_data(res, stringifier.stringify(res.type(), rval));
        });
    } else {
        bool is_self = node->type_name()->is_self();
        exp::append(res, is_self ? "Self" : out::type_str(stringifier, type));
        exp::append(res, std::string{"."} + ulam::ops::str(node->op()), "");
    }
    return res;
}

ExprRes EvalExprVisitor::type_op_expr_construct(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ExprRes&& arg) {
    std::string data;
    if (!has_flag(evl::NoCodegen))
        data = exp::data(arg);
    auto res = Base::type_op_expr_construct(node, std::move(arg));
    if (!data.empty()) {
        auto op_str = data + ".instanceof";
        exp::set_data(res, exp::data_combine(op_str, exp::data(res), "."));
    }
    return res;
}

ExprRes EvalExprVisitor::type_op_expr_default(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ExprRes&& arg) {
    std::string data;
    if (!has_flag(evl::NoCodegen)) {
        if (node->op() == ulam::TypeOp::AtomOf &&
            !arg.type()->deref()->is_atom()) {
            // NOTE: a hack to remove _single_ redundand member access before
            // calling
            // `.atomof`, t3905
            if (arg.has_flag(exp::MemberAccess) ||
                arg.has_flag(exp::SelfMemberAccess))
                exp::remove_member_access_op(arg, true);
        }
        data = exp::data(arg);
    }
    auto res = Base::type_op_expr_default(node, std::move(arg));

    if (!data.empty()) {
        if (res.type()->is_prim() && res.value().is_consteval()) {
            res.value().with_rvalue([&](const ulam::RValue& rval) {
                auto stringifier = make_stringifier();
                stringifier.options.unary_as_unsigned_lit = true;
                stringifier.options.bool_as_unsigned_lit = true;
                exp::set_data(res, stringifier.stringify(res.type(), rval));
            });
        } else {
            exp::set_data(res, data);
            exp::append(res, std::string{"."} + ulam::ops::str(node->op()), "");
        }
    }
    return res;
}

ExprRes EvalExprVisitor::ident_self(ulam::Ref<ulam::ast::Ident> node) {
    auto res = Base::ident_self(node);
    if (!has_flag(evl::NoCodegen))
        exp::set_self(res);
    return res;
}

ExprRes EvalExprVisitor::ident_super(ulam::Ref<ulam::ast::Ident> node) {
    auto res = Base::ident_super(node);
    if (!has_flag(evl::NoCodegen))
        exp::set_data(res, "super");
    return res;
}

ExprRes EvalExprVisitor::ident_var(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Var> var) {
    auto res = Base::ident_var(node, var);
    if (has_flag(evl::NoCodegen))
        return res;

    if (res.value().is_consteval() && !var->type()->is_array() &&
        !var->type()->is_class()) {
        res.value().with_rvalue([&](const ulam::RValue& rval) {
            auto stringifier = make_stringifier();
            stringifier.options.unary_as_unsigned_lit = true;
            stringifier.options.bool_as_unsigned_lit = true;
            exp::set_data(res, stringifier.stringify(res.type(), rval));
        });
    } else {
        exp::set_data(res, str(var->name_id()));
    }
    return res;
}

ExprRes EvalExprVisitor::ident_prop(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Prop> prop) {
    auto res = Base::ident_prop(node, prop);
    if (!has_flag(evl::NoCodegen)) {
        exp::set_self(res);
        exp::add_member_access(res, str(prop->name_id()), true);
    }
    return res;
}

ExprRes EvalExprVisitor::ident_fset(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::FunSet> fset) {
    auto res = Base::ident_fset(node, fset);
    if (!has_flag(evl::NoCodegen)) {
        exp::set_self(res);
        exp::add_member_access(res, FuncallPh, true);
    }
    return res;
}

ExprRes EvalExprVisitor::array_access_class(
    ulam::Ref<ulam::ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    bool before_member_access = false;
    if (!has_flag(evl::NoCodegen)) {
        before_member_access = obj.has_flag(exp::MemberAccess);
        if (before_member_access)
            exp::remove_member_access_op(obj);
    }

    auto res = Base::array_access_class(node, std::move(obj), std::move(idx));
    if (before_member_access)
        exp::append(res, ".");
    return res;
}

ExprRes EvalExprVisitor::array_access_string(
    ulam::Ref<ulam::ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    std::string data;
    bool before_member_access = false;
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(obj);
        before_member_access = obj.has_flag(exp::MemberAccess);
    }
    auto res = Base::array_access_string(node, std::move(obj), std::move(idx));
    if (!data.empty()) {
        exp::set_data(res, std::move(data));
        exp::add_array_access(res, exp::data(idx), before_member_access);
        res.set_flag(exp::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::array_access_array(
    ulam::Ref<ulam::ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    std::string data, idx_data;
    bool before_member_access = false;
    if (!has_flag(evl::NoCodegen)) {
        idx_data = exp::data(idx);
        data = exp::data(obj);
        before_member_access = obj.has_flag(exp::MemberAccess);
    }
    auto res = Base::array_access_array(node, std::move(obj), std::move(idx));
    if (!data.empty()) {
        exp::set_data(res, std::move(data));
        exp::add_array_access(res, idx_data, before_member_access);
        res.set_flag(exp::NoConstFold); // do not folt result of [], t3881
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
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(obj);
        no_fold = obj.has_flag(exp::NoConstFold);
        is_self = obj.has_flag(exp::Self);
    }
    auto res = Base::member_access_var(node, std::move(obj), var);
    if (!data.empty()) {
        exp::set_data(res, data);
        if (!no_fold && res.value().is_consteval() &&
            !var->type()->is_array() && !var->type()->is_class()) {
            res.value().with_rvalue([&](const ulam::RValue& rval) {
                auto stringifier = make_stringifier();
                stringifier.options.unary_as_unsigned_lit = true;
                stringifier.options.bool_as_unsigned_lit = true;
                auto val_str = stringifier.stringify(res.type(), rval);
                exp::add_member_access(res, val_str, is_self);
            });
        } else {
            auto name = str(var->name_id());
            exp::add_member_access(res, name, is_self);
        }
        if (no_fold)
            res.set_flag(exp::NoConstFold);
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
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(obj);
        no_fold = obj.has_flag(exp::NoConstFold);
        is_self = obj.has_flag(exp::Self);
    }
    auto res = Base::member_access_prop(node, std::move(obj), prop);
    if (!data.empty()) {
        auto name = str(prop->name_id());
        exp::set_data(res, data);
        exp::add_member_access(res, name, is_self);
        if (no_fold)
            res.set_flag(exp::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::member_access_fset(
    ulam::Ref<ulam::ast::MemberAccess> node,
    ExprRes&& obj,
    ulam::Ref<ulam::FunSet> fset) {
    if (!obj)
        return std::move(obj);

    std::string data;
    bool no_fold = false;
    bool is_self = false;
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(obj);
        no_fold = obj.has_flag(exp::NoConstFold);
        is_self = obj.has_flag(exp::Self);
    }
    auto res = Base::member_access_fset(node, std::move(obj), fset);
    if (!data.empty()) {
        exp::set_data(res, data);
        exp::add_member_access(res, FuncallPh, is_self);
        if (no_fold)
            res.set_flag(exp::NoConstFold);
    }
    return res;
}

ExprRes EvalExprVisitor::class_const_access(
    ulam::Ref<ulam::ast::ClassConstAccess> node, ulam::Ref<ulam::Var> var) {
    auto res = Base::class_const_access(node, var);
    if (!has_flag(evl::NoCodegen)) {
        assert(!res.value().empty());
        assert(res.value().is_consteval());
        res.value().with_rvalue([&](const auto& rval) {
            auto stringifier = make_stringifier();
            exp::set_data(res, stringifier.stringify(var->type(), rval));
        });
    }
    return res;
}

ExprRes EvalExprVisitor::bind(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Ref<ulam::FunSet> fset,
    ulam::sema::ExprRes&& obj) {
    assert(obj);
    std::string data;
    bool is_self = false;
    if (!has_flag(evl::NoCodegen)) {
        data = exp::data(obj);
        is_self = obj.has_flag(exp::Self);
    }
    auto res = Base::bind(node, fset, std::move(obj));
    if (!data.empty()) {
        exp::set_data(res, data);
        exp::add_member_access(res, FuncallPh, is_self);
    }
    return res;
}

ExprRes EvalExprVisitor::as_base(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Ref<ulam::ast::TypeIdent> base,
    ExprRes&& obj) {
    std::string data;
    if (!has_flag(evl::NoCodegen))
        data = exp::data(obj);
    auto res = Base::as_base(node, base, std::move(obj));
    if (!data.empty())
        exp::set_data(res, exp::data_combine(data, str(base->name_id()), "."));
    return res;
}

Stringifier EvalExprVisitor::make_stringifier() {
    Stringifier stringifier{program()};
    return stringifier;
}
