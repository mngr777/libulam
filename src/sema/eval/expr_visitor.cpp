#include <cassert>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/fun.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/type/ops.hpp>
#include <libulam/semantic/value/types.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalExprVisitor] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

ExprRes EvalExprVisitor::visit(Ref<ast::TypeOpExpr> node) {
    debug() << __FUNCTION__ << " TypeOpExpr\n" << line_at(node);
    ExprRes res;
    if (node->has_type_name()) {
        auto resolver = env().resolver(true);
        auto type = resolver.resolve_type_name(node->type_name(), true);
        if (!type)
            return {ExprError::UnresolvableType};
        res = type_op(node, type);
    } else {
        assert(node->has_expr());
        auto expr_res = node->expr()->accept(*this);
        if (!expr_res)
            return expr_res;
        res = type_op_expr(node, std::move(expr_res));
    }
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::Ident> node) {
    debug() << __FUNCTION__ << " Ident `" << str(node->name().str_id()) << "`\n"
            << line_at(node);

    // self
    if (node->is_self())
        return check(node, ident_self(node));

    // super
    if (node->is_super())
        return check(node, ident_super(node));

    auto name_id = node->name().str_id();
    Scope::Symbol* sym{};
    {
        Scope::GetParams sgp;
        sgp.local = node->is_local();
        sym = scope()->get(name_id, sgp);
    }
    if (!sym) {
        diag().error(node, "symbol not found");
        return {ExprError::SymbolNotFound};
    }

    using std::placeholders::_1;
    auto res = sym->accept(
        std::bind(std::mem_fn(&EvalExprVisitor::ident_var), this, node, _1),
        std::bind(std::mem_fn(&EvalExprVisitor::ident_prop), this, node, _1),
        std::bind(std::mem_fn(&EvalExprVisitor::ident_fset), this, node, _1),
        [&](auto value) -> ExprRes { assert(false); });
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::ParenExpr> node) {
    debug() << __FUNCTION__ << " ParenExpr\n" << line_at(node);
    return node->inner()->accept(*this);
}

ExprRes EvalExprVisitor::visit(Ref<ast::BinaryOp> node) {
    debug() << __FUNCTION__ << " BinaryOp\n" << line_at(node);
    assert(node->has_lhs() && node->has_rhs());

    Op op = node->op();

    auto left = node->lhs()->accept(*this);
    if (!left)
        return left;

    // short-circuit?
    if (!has_flag(evl::NoExec) && (op == Op::And || op == Op::Or)) {
        auto bool_res = env().to_boolean(node->lhs(), left.copy());
        bool truth = is_true(bool_res);
        if (truth == (op == Op::Or))
            return left;
    }

    auto right = node->rhs()->accept(*this);
    if (!right)
        return right;

    auto res = binary_op(
        node, op, node->lhs(), std::move(left), node->rhs(), std::move(right));
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::UnaryOp> node) {
    debug() << __FUNCTION__ << " UnaryOp\n" << line_at(node);
    auto arg = node->arg()->accept(*this);
    if (!arg)
        return arg;

    auto res = unary_op(
        node, node->op(), node->arg(), std::move(arg), node->type_name());
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::Cast> node) {
    debug() << __FUNCTION__ << " Cast\n" << line_at(node);
    // eval expr
    auto res = node->expr()->accept(*this);
    if (!res)
        return res;

    // resolve target type
    auto resolver = env().resolver(true);
    auto cast_type =
        resolver.resolve_full_type_name(node->full_type_name(), scope());
    if (!cast_type)
        return {ExprError::InvalidCast};

    // (Void) expr
    if (cast_type->is(VoidId))
        return {builtins().void_type(), Value{RValue{}}};

    res = env().cast(node, cast_type, std::move(res), true);
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::Ternary> node) {
    debug() << __FUNCTION__ << " Ternary\n" << line_at(node);
    auto boolean = builtins().boolean();

    auto cond_res = ternary_eval_cond(node);
    if (!cond_res)
        return cond_res;
    assert(cond_res.type()->is_same(boolean));

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

    return ternary_eval(
        node, std::move(cond_res), type, std::move(if_true_res),
        std::move(if_false_res));
}

Ref<Type>
EvalExprVisitor::common_type(const ExprRes& res1, const ExprRes& res2) {
    assert(res1);
    assert(res2);

    auto type1 = res1.type();
    auto type2 = res2.type();
    const auto& val1 = res1.value();
    const auto& val2 = res2.value();
    return (!val1.empty() && !val2.empty()) ? type1->common(val1, type2, val2)
                                            : type1->common(type2);
}

ExprRes EvalExprVisitor::visit(Ref<ast::BoolLit> node) {
    debug() << __FUNCTION__ << " BoolLit\n" << line_at(node);
    // Bool(1)
    auto type = builtins().boolean();
    auto rval = type->construct(node->value());
    rval.set_is_consteval(true);
    return check(node, {type, Value{std::move(rval)}});
}

ExprRes EvalExprVisitor::visit(Ref<ast::NumLit> node) {
    debug() << __FUNCTION__ << " NumLit\n" << line_at(node);
    const auto& number = node->value();
    Ref<Type> type{};
    RValue rval{};
    if (number.is_signed()) {
        // Int(n)
        type = builtins().int_type(number.bitsize());
        rval = RValue{number.value<Integer>(), true};
    } else {
        // Unsigned(n)
        type = builtins().unsigned_type(number.bitsize());
        rval = RValue{number.value<Unsigned>(), true};
    }
    return check(node, {type, Value{std::move(rval)}});
}

ExprRes EvalExprVisitor::visit(Ref<ast::StrLit> node) {
    debug() << __FUNCTION__ << " StrLit\n" << line_at(node);
    auto type = builtins().type(StringId);
    return check(node, ExprRes{type, Value{RValue{node->value(), true}}});
}

ExprRes EvalExprVisitor::visit(Ref<ast::FunCall> node) {
    debug() << __FUNCTION__ << " FunCall\n" << line_at(node);

    // obj.fun | operator<op>
    auto callable = node->has_callable() ? node->callable()->accept(*this)
                                         : callable_op(node);

    // args
    auto args = eval_args(node->args());
    if (!args)
        return {args.error()};

    auto res = env().call(node, std::move(callable), std::move(args));
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::MemberAccess> node) {
    debug() << __FUNCTION__ << " MemberAccess\n" << line_at(node);
    assert(node->has_obj());

    // eval object expr
    auto obj = node->obj()->accept(*this);
    if (!obj)
        return {ExprError::Error};

    // is an object?
    if (!check_is_class(node, obj.type(), true))
        return {ExprError::NotClass};

    Ref<Class> base{};
    if (node->has_base_type()) {
        // foo.Base.bar
        obj = as_base(node, node->base_type(), std::move(obj));
        if (!obj)
            return obj;
        base = obj.type()->as_class();
    } else if (obj.is_super()) {
        // super.bar
        base = obj.type()->as_class();
    }

    // get op fset
    if (node->is_op())
        return check(node, member_access_op(node, std::move(obj), base));

    // get symbol
    auto cls = obj.type()->deref()->as_class();
    auto name = node->ident()->name();
    auto sym = cls->get(name.str_id());
    if (!sym) {
        diag().error(node->ident(), "member not found");
        return {ExprError::MemberNotFound};
    }

    auto res = sym->accept(
        [&](Ref<Var> var) {
            return member_access_var(node, std::move(obj), var);
        },
        [&](Ref<Prop> prop) {
            return member_access_prop(node, std::move(obj), prop);
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            return member_access_fset(node, std::move(obj), fset, base);
        },
        [&](auto other) -> ExprRes { assert(false); });
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::ClassConstAccess> node) {
    debug() << __FUNCTION__ << "ClassConstAccess" << line_at(node);

    auto resolver = env().resolver(true);
    auto type = resolver.resolve_class_name(node->type_name(), false);
    if (!type)
        return {ExprError::UnresolvableType};

    auto cls = type->as_class();
    auto sym = cls->get(node->ident()->name_id());
    if (!sym) {
        diag().error(node->ident(), "class member not found");
        return {ExprError::MemberNotFound};
    }
    if (!sym->is<Var>()) {
        diag().error(node->ident(), "not a class constant");
        return {ExprError::NotClassConst};
    }

    auto var = sym->get<Var>();
    auto res = class_const_access(node, var);
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::ArrayAccess> node) {
    debug() << __FUNCTION__ << " ArrayAccess\n" << line_at(node);
    assert(node->has_array());
    assert(node->has_index());

    // eval array expr
    auto obj = node->array()->accept(*this);
    if (!obj)
        return {ExprError::Error};

    // index
    auto idx = node->index()->accept(*this);
    if (!idx)
        return {ExprError::UnknownArrayIndex};

    // class?
    if (obj.type()->actual()->is_class()) {
        return check(
            node, array_access_class(node, std::move(obj), std::move(idx)));
    }

    // cast to index type
    idx = env().cast_to_idx(node->index(), std::move(idx));
    if (!idx)
        return idx;

    // string?
    if (obj.type()->actual()->is(StringId)) {
        return check(
            node, array_access_string(node, std::move(obj), std::move(idx)));
    }

    // must be an array
    if (!obj.type()->actual()->is_array()) {
        diag().error(node->array(), "not an array");
        return {ExprError::NotArray};
    }
    return check(
        node, array_access_array(node, std::move(obj), std::move(idx)));
}

ExprRes EvalExprVisitor::check(Ref<ast::Expr> node, ExprRes&& res) {
    const auto& val = res.value();
    if (res && !val.is_consteval()) {
        if (has_flag(evl::Consteval)) {
            diag().error(node, "not consteval");
            return {ExprError::NotConsteval};
        }
        if (has_flag(evl::NoExec) &&
            !(val.is_lvalue() && val.lvalue().is<BoundFunSet>())) {
            auto empty = val.is_lvalue() ? Value{val.lvalue().derived()}
                                         : Value{RValue{}};
            return res.derived(res.type(), std::move(empty), true);
        }
    }
    return std::move(res);
}

bool EvalExprVisitor::check_is_assignable(
    Ref<ast::Expr> node, const Value& value) {
    if (value.is_rvalue()) {
        diag().error(node, "cannot assign to rvalue");
        return false;
    }
    if (value.lvalue().is_xvalue()) {
        diag().error(node, "cannot assign to xvalue");
        return false;
    }
    assert(value.is_lvalue() && !value.lvalue().is_xvalue());
    return true;
}

bool EvalExprVisitor::check_is_object(
    Ref<ast::Expr> node, Ref<const Type> type, bool deref) {
    type = deref ? type->actual() : type->canon();
    if (!type->is_object()) {
        diag().error(node, "not a class or Atom");
        return false;
    }
    return true;
}

bool EvalExprVisitor::check_is_class(
    Ref<ast::Expr> node, Ref<const Type> type, bool deref) {
    type = deref ? type->actual() : type->canon();
    if (!type->is_class()) {
        diag().error(node, "not a class");
        return false;
    }
    return true;
}

Ref<Class> EvalExprVisitor::class_super(Ref<ast::Expr> node, Ref<Class> cls) {
    if (!cls->has_super()) {
        diag().error(node, "class does not have a superclass");
        return {};
    }
    return cls->super();
}

Ref<Class> EvalExprVisitor::class_base(
    Ref<ast::Expr> node,
    ExprRes& obj,
    Ref<Class> cls,
    Ref<ast::BaseTypeSelect> base_type) {
    assert(base_type);

    // *.Base1.Base2(p1, p2)
    for (unsigned n = 0; n < base_type->type_spec_num(); ++n) {
        auto type_spec = base_type->type_spec(n);
        if (type_spec->has_args()) {
            // Type(arg1, arg2)
            auto type =
                env().resolver(true).resolve_type_spec(type_spec, false);
            if (!type)
                return {};
            if (!type->is_class()) {
                diag().error(type_spec, "not a class");
                return {};
            }
            cls = type->as_class();
        } else {
            // Type
            cls = class_base_ident(node, obj, cls, type_spec->ident());
        }
    }

    // *.Base1.Base2(p1, p2)[classid]
    if (base_type->has_classid()) {
        auto classid = classid_expr(node, obj, base_type->classid());
        cls = class_base_classid(
            base_type->classid(), obj, cls, std::move(classid));
    }

    return cls;
}

Ref<Class> EvalExprVisitor::class_base_ident(
    Ref<ast::Expr> node,
    ExprRes& obj,
    Ref<Class> cls,
    Ref<ast::TypeIdent> ident) {
    assert(ident);

    if (ident->is_self())
        return cls;
    if (ident->is_super())
        return class_super(node, cls);

    auto name_id = ident->name_id();
    auto sym = scope()->get(name_id);
    if (!sym) {
        // search in class scope
        sym = cls->scope()->get(name_id);
        if (!sym) {
            diag().error(ident, "base type not found");
            return {};
        }
    }
    assert(sym->is<UserType>());
    auto type = sym->get<UserType>();

    // resolve aliases
    if (type->is_alias()) {
        auto resolver = env().resolver(true);
        if (!resolver.resolve(type->as_alias()))
            return {};
    }

    if (!type->is_class()) {
        diag().error(ident, "type is not a class");
        return {};
    }

    auto base = type->as_class();
    if (!base->is_same_or_base_of(cls)) {
        diag().error(
            ident, "not an ancestor of " + std::string{cls->full_name()});
        return {};
    }
    return base;
}

Ref<Class> EvalExprVisitor::class_base_classid(
    Ref<ast::Expr> expr, ExprRes& obj, Ref<Class> cls, ExprRes&& classid) {
    assert(expr);

    if (classid.value().empty() && has_flag(evl::NoExec))
        return cls; // return current class if not executing

    auto int_class_id = classid.value().copy_rvalue().get<Integer>();
    if (int_class_id < 0 || int_class_id == NoClassId) {
        diag().error(expr, "invalid class ID");
        return {};
    }
    auto base_cls_id = static_cast<cls_id_t>(int_class_id);
    auto base = program()->classes().get(base_cls_id);
    if (!cls->is_base_of(base)) {
        auto message = "class `" + std::string{cls->full_name()} +
                       "' is not a base of `" + std::string{base->full_name()};
        diag().error(expr, message);
        return {};
    }
    return base;
}

ExprRes EvalExprVisitor::classid_expr(
    Ref<ast::Expr> node, ExprRes& obj, Ref<ast::Expr> expr) {
    auto res = expr->accept(*this);
    res = env().cast_to_idx(expr, std::move(res));
    if (!res)
        return {};
    return res;
}

ExprRes EvalExprVisitor::binary_op(
    Ref<ast::Expr> node,
    Op op,
    Ref<ast::Expr> l_node,
    ExprRes&& left,
    Ref<ast::Expr> r_node,
    ExprRes&& right) {
    debug() << __FUNCTION__ << "\n" << line_at(node);

    ExprRes lval_res;
    if (ops::is_assign(op)) {
        if (!check_is_assignable(node, left.value()))
            return {ExprError::NotAssignable};
        lval_res = left.derived(left.type(), Value{left.value().lvalue()});
    }

    auto recast = [&](Ref<ast::Expr> expr, TypeError error,
                      ExprRes&& arg) -> ExprRes {
        switch (error.status) {
        case TypeError::Incompatible:
            diag().error(expr, "incompatible type");
            return {ExprError::InvalidOperandType};
        case TypeError::ExplCastRequired: {
            auto rval = arg.move_value().move_rvalue();
            auto message =
                "suggest casting " + std::string{arg.type()->name()} + " to ";
            message +=
                (error.cast_bi_type_id != NoBuiltinTypeId)
                    ? std::string{builtin_type_str(error.cast_bi_type_id)}
                    : error.cast_type->name();
            diag().error(expr, message);
            return {ExprError::InvalidOperandType};
        }
        case TypeError::ImplCastRequired: {
            if (error.cast_bi_type_id != NoBuiltinTypeId)
                return env().cast(expr, error.cast_bi_type_id, std::move(arg));
            assert(error.cast_type);
            return env().cast(expr, error.cast_type, std::move(arg));
        }
        case TypeError::Ok:
            return std::move(arg);
        default:
            assert(false);
        };
    };

    type_check_flags_t type_check_flags = NoTypeCheckFlags;
    if (program()->eval_options().implicit_class_negation_op)
        type_check_flags |= TypeCheckImplicitClassNegationOp;
    auto type_errors = binary_op_type_check(op, left, right, type_check_flags);

    // cast left
    left = recast(l_node, type_errors.first, std::move(left));
    if (!left)
        return std::move(left);

    // cast right
    right = recast(r_node, type_errors.second, std::move(right));
    if (!right)
        return std::move(right);

    return apply_binary_op(
        node, op, std::move(lval_res), l_node, std::move(left), r_node,
        std::move(right));
}

ExprRes EvalExprVisitor::apply_binary_op(
    Ref<ast::Expr> node,
    Op op,
    ExprRes&& lval_res,
    Ref<ast::Expr> l_node,
    ExprRes&& left,
    Ref<ast::Expr> r_node,
    ExprRes&& right) {

    auto l_type = left.type()->actual();
    auto r_type = right.type()->actual();

    if (left.type()->actual()->is_prim()) {
        if (op != Op::Assign) {
            // primitive binary op
            assert(r_type->is_prim());
            auto l_rval = left.move_value().move_rvalue();
            auto r_rval = right.move_value().move_rvalue();
            right = {l_type->as_prim()->binary_op(
                op, std::move(l_rval), r_type->as_prim(), std::move(r_rval))};
        }
    } else if (left.type()->actual()->is_class()) {
        // class op
        auto cls = left.type()->actual()->as_class();
        if (op != Op::Assign || cls->has_op(Op::Assign)) {
            auto cls = left.type()->actual()->as_class();

            // get class op (or negation) fset
            bool is_negation = false;
            auto fset = cls->op(op);
            if (!fset && program()->eval_options().implicit_class_negation_op) {
                auto neg_op = ops::negation(op);
                assert(neg_op != Op::None);
                fset = cls->op(neg_op);
                is_negation = true;
            }
            assert(fset);

            // bind to object
            left = bind(node, fset, std::move(left));
            ExprResList args;
            args.push_back(std::move(right));

            return is_negation
                       ? call_negation_op(
                             node, op, std::move(left), std::move(args))
                       : env().call(node, std::move(left), std::move(args));
        }
    }

    // handle assignment
    if (ops::is_assign(op)) {
        if (lval_res.value().empty() || right.value().empty())
            return std::move(left);
        return assign(node, std::move(lval_res), std::move(right));
    }
    return std::move(right);
}

ExprRes EvalExprVisitor::call_negation_op(
    Ref<ast::Expr> node, Op op, ExprRes&& left, ExprResList&& args) {
    auto res = env().call(node, std::move(left), std::move(args));
    return negate(node, std::move(res));
}

ExprRes EvalExprVisitor::unary_op(
    Ref<ast::Expr> node,
    Op op,
    Ref<ast::Expr> arg_node,
    ExprRes&& arg,
    Ref<ast::TypeName> type_name) {

    TypedValue lval_tv;
    if (ops::is_inc_dec(op)) {
        // store lvalue
        if (!check_is_assignable(node, arg.value()))
            return {ExprError::NotAssignable};
        lval_tv = {arg.type(), Value{arg.value().lvalue()}};
    }

    auto error = unary_op_type_check(op, arg.type());
    switch (error.status) {
    case TypeError::Incompatible:
        diag().error(node, "incompatible type");
        return {ExprError::InvalidOperandType};
    case TypeError::ExplCastRequired:
        if (ops::is_inc_dec(op)) {
            // assert ??
            diag().error(node, "incompatible");
            return {ExprError::InvalidOperandType};
        }
        return {ExprError::CastRequired};
    case TypeError::ImplCastRequired: {
        assert(!ops::is_inc_dec(op));
        arg = env().cast(node, error.cast_bi_type_id, std::move(arg));
        if (!arg)
            return std::move(arg);
    } break;
    case TypeError::Ok:
        break;
    }

    Ref<Type> type{};
    if (type_name) {
        auto resolver = env().resolver(true);
        type = resolver.resolve_type_name(type_name, true);
        if (!type)
            return {ExprError::UnresolvableType};
    }

    return apply_unary_op(
        node, op, std::move(lval_tv), arg_node, std::move(arg), type);
}

ExprRes EvalExprVisitor::apply_unary_op(
    Ref<ast::Expr> node,
    Op op,
    ExprRes&& lval_res,
    Ref<ast::Expr> arg_node,
    ExprRes&& arg,
    Ref<Type> type) {

    auto arg_type = arg.type()->deref();
    if (arg_type->is_prim()) {
        RValue orig_rval;
        if (op == Op::PostInc || op == Op::PostDec)
            orig_rval = arg.value().copy_rvalue();

        auto tv =
            arg_type->as_prim()->unary_op(op, arg.move_value().move_rvalue());

        if (ops::is_inc_dec(op)) {
            if (!lval_res.value().empty() && !tv.value().empty()) {
                ExprRes lval_res_copy = lval_res.derived(
                    lval_res.type(), Value{lval_res.value().lvalue()});
                assign(node, std::move(lval_res_copy), std::move(tv));
            }
            if (ops::is_unary_pre_op(op))
                return {std::move(lval_res)};
            assert(ops::is_unary_post_op(op));
            return lval_res.derived(
                lval_res.type(), Value{std::move(orig_rval)});
        }
        return {std::move(tv)};

    } else if (arg_type->is_object()) {
        if (op == Op::Is) {
            assert(type);
            if (!check_is_object(node, type))
                return {ExprError::NotObject};
            assert(op == Op::Is);
            if (!arg.value().has_rvalue())
                return {builtins().boolean(), Value{RValue{}}};
            // NOTE: using "real" dyn type, see t41365
            auto dyn_type = arg.value().dyn_obj_type(true);
            assert(type->is_class()); // TODO: check upstream
            bool is =
                dyn_type->is_class() &&
                type->as_class()->is_same_or_base_of(dyn_type->as_class());
            auto boolean = builtins().boolean();
            return {boolean, Value{boolean->construct(is)}};

        } else if (arg_type->is_class()) {
            auto cls = arg_type->as_class();
            auto fset = cls->op(op);
            assert(fset);
            ExprResList args;
            if (op == Op::PostInc || op == Op::PostDec)
                args.push_back(post_inc_dec_dummy());
            arg = bind(node, fset, std::move(arg));
            return env().call(node, std::move(arg), std::move(args));
        }
    }
    assert(false);
}

ExprRes EvalExprVisitor::post_inc_dec_dummy() {
    auto int_type = builtins().int_type();
    auto rval = int_type->construct(1);
    rval.set_is_consteval(true);
    return {int_type, Value{std::move(rval)}};
}

ExprRes EvalExprVisitor::ternary_eval_cond(Ref<ast::Ternary> node) {
    assert(node->has_cond());
    auto cond_res = node->cond()->accept(*this);
    if (!cond_res)
        return cond_res;

    // cast bo Bool(1)
    auto boolean = builtins().boolean();
    return env().cast(node, boolean, std::move(cond_res));
}

ExprResPair
EvalExprVisitor::ternary_eval_branches_noexec(Ref<ast::Ternary> node) {
    auto fr = env().flags_raii(flags() | evl::NoExec);
    return {node->if_true()->accept(*this), node->if_false()->accept(*this)};
}

ExprRes EvalExprVisitor::ternary_eval(
    Ref<ast::Ternary> node,
    ExprRes&& cond_res,
    Ref<Type> type,
    ExprRes&& if_true_res,
    ExprRes&& if_false_res) {
    if (cond_res.value().empty()) {
        auto if_true_val = if_true_res.move_value();
        auto if_false_val = if_false_res.move_value();
        auto val = (if_true_val.is_lvalue() && if_false_val.is_lvalue())
                       ? Value{LValue{}}
                       : Value{RValue{}};
        if (val.is_lvalue()) {
            bool is_xvalue = if_true_val.is_tmp() || if_false_val.is_tmp();
            val.lvalue().set_is_xvalue(is_xvalue);
        }
        return {type, std::move(val)};
    }

    // disable dereference casts
    auto fr = env().add_flags_raii(evl::NoDerefCast);

    auto cond_rval = cond_res.move_value().move_rvalue();
    bool cond_bool = builtins().boolean()->is_true(cond_rval);

    // use noexec result if possible
    if (cond_bool && (has_flag(evl::NoExec) || !if_true_res.value().empty())) {
        return env().cast(node->if_true(), type, std::move(if_true_res));
    }
    if (!cond_bool &&
        (has_flag(evl::NoExec) || !if_false_res.value().empty())) {
        return env().cast(node->if_false(), type, std::move(if_false_res));
    }

    // exec branch
    auto branch = cond_bool ? node->if_true() : node->if_false();
    auto res = branch->accept(*this);
    return env().cast(branch, type, std::move(res));
}

ExprRes EvalExprVisitor::type_op(Ref<ast::TypeOpExpr> node, Ref<Type> type) {
    if (type->is_class()) {
        // instanceof with arguments?
        auto cls = type->as_class();
        if (node->op() == TypeOp::InstanceOf && node->has_args())
            return type_op_construct(node, cls);
    }
    return type_op_default(node, type);
}

ExprRes
EvalExprVisitor::type_op_construct(Ref<ast::TypeOpExpr> node, Ref<Class> cls) {
    auto args = eval_args(node->args());
    if (!args)
        return {args.error()};
    return env().construct(node, cls, std::move(args));
}

ExprRes
EvalExprVisitor::type_op_default(Ref<ast::TypeOpExpr> node, Ref<Type> type) {
    auto tv = type->actual()->type_op(node->op());
    if (!tv) {
        diag().error(node, "invalid type operator");
        return {ExprError::InvalidTypeOperator};
    }
    return tv;
}

ExprRes
EvalExprVisitor::type_op_expr(Ref<ast::TypeOpExpr> node, ExprRes&& arg) {
    // obj.Base.<type_op>?
    if (node->has_base_type()) {
        arg = as_base(node, node->base_type(), std::move(arg));
        if (!arg)
            return std::move(arg);
    }

    // class type op may require an evaluator
    if (arg.type()->is_class()) {
        // custom lengthof?
        auto cls = arg.type()->as_class();
        if (node->op() == TypeOp::InstanceOf) {
            if (node->has_args()) {
                return type_op_expr_construct(node, std::move(arg));
            }
        } else if (node->op() == TypeOp::LengthOf) {
            if (cls->has_fun("alengthof")) {
                return type_op_expr_fun(
                    node, cls->fun("alengthof"), std::move(arg));
            }
        }
    }
    return type_op_expr_default(node, std::move(arg));
}

ExprRes EvalExprVisitor::type_op_expr_construct(
    Ref<ast::TypeOpExpr> node, ExprRes&& arg) {
    assert(arg.type()->is_class());
    auto args = eval_args(node->args());
    if (!args)
        return {args.error()};
    return env().construct(node, arg.type()->as_class(), std::move(args));
}

ExprRes EvalExprVisitor::type_op_expr_fun(
    Ref<ast::TypeOpExpr> node, Ref<FunSet> fset, ExprRes&& arg) {
    arg = bind(node, fset, std::move(arg));
    return env().call(node, std::move(arg), {});
}

ExprRes EvalExprVisitor::type_op_expr_default(
    Ref<ast::TypeOpExpr> node, ExprRes&& arg) {
    bool is_consteval = true;
    bool use_dyn_type = false;
    switch (node->op()) {
    case TypeOp::MaxOf:
    case TypeOp::MinOf:
    case TypeOp::SizeOf:
        break;
    case TypeOp::LengthOf:
        is_consteval = arg.type()->is_array() ||
                       (arg.type()->is(StringId) && arg.value().is_consteval());
        break;
    case TypeOp::AtomOf:
        is_consteval = false;
        use_dyn_type = true;
        break;
    case TypeOp::ClassIdOf:
    case TypeOp::InstanceOf:
    case TypeOp::ConstantOf:
        // NOTE: self has known type at compile time
        is_consteval = arg.value().is_consteval() || arg.is_self();
        use_dyn_type = true;
        break;
    default:
        is_consteval = arg.value().is_consteval();
    }

    auto type = arg.type()->deref();
    auto val = arg.move_value();

    // NOTE: instanceof uses dynamic type, but not e.g. sizeof (t3583)
    if (use_dyn_type) {
        if (type->is_object() && !val.empty()) {
            // using "real" type, implied by usage of `super.contstantof` in
            // t41506
            type = val.dyn_obj_type(true);
        }
    }

    auto tv = type->type_op(node->op(), val);
    if (!tv) {
        const auto name = ops::str(node->op());
        assert(name);
        diag().error(
            node->loc_id(), 1, std::string("operation `") + name + "' failed");
        return {ExprError::InvalidTypeOperator};
    }

    type = tv.type();
    val = tv.move_value();
    val.set_is_consteval(is_consteval);
    return {type, std::move(val)};
}

ExprRes EvalExprVisitor::ident_self(Ref<ast::Ident> node) {
    auto self = scope()->self();
    ExprRes res = {scope()->eff_cls()->ref_type(), Value{self}};
    res.set_is_self(true);
    return res;
}

ExprRes EvalExprVisitor::ident_super(Ref<ast::Ident> node) {
    auto self_cls = scope()->eff_cls();
    auto sup = class_super(node, self_cls);
    if (!sup)
        return {ExprError::NoSuper};
    auto self = scope()->self();
    ExprRes res = {sup, Value{self.as(sup)}};
    res.set_is_super(true);
    return res;
}

ExprRes EvalExprVisitor::ident_var(Ref<ast::Ident> node, Ref<Var> var) {
    auto resolver = env().resolver(true);
    if (!var->has_type() && !resolver.resolve(var))
        return {ExprError::UnresolvableVar};
    return {var->type(), Value{var->lvalue()}};
}

ExprRes EvalExprVisitor::ident_prop(Ref<ast::Ident> node, Ref<Prop> prop) {
    return {prop->type(), Value{scope()->self().prop(prop)}};
}

ExprRes EvalExprVisitor::ident_fset(Ref<ast::Ident> node, Ref<FunSet> fset) {
    auto bfset = scope()->self().bound_fset(fset);
    return {builtins().fun_type(), Value{std::move(bfset)}};
}

ExprRes EvalExprVisitor::callable_op(Ref<ast::FunCall> node) {
    assert(node->is_op_call());
    auto lval = scope()->self();
    auto cls = scope()->eff_cls();
    auto fset = cls->op(node->fun_op());
    if (!fset) {
        diag().error(node, "operator not found in class");
        return {ExprError::NoOperator};
    }
    return {builtins().fun_type(), Value{lval.bound_fset(fset)}};
}

ExprRes EvalExprVisitor::array_access_class(
    Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    assert(obj.type()->actual()->is_class());

    // op fset
    auto cls = obj.type()->actual()->as_class();
    auto fset = cls->op(Op::ArrayAccess);
    if (!fset)
        return {ExprError::NoOperator};
    bool is_tmp = obj.value().is_tmp();

    // callable, args
    auto callable = bind(node, fset, std::move(obj));
    ExprResList args;
    args.push_back(std::move(idx));

    ExprRes res = env().call(node, std::move(callable), std::move(args));
    if (is_tmp && res.value().is_lvalue()) {
        LValue lval = res.move_value().lvalue();
        lval.set_is_xvalue(true);
        return {res.type(), Value{std::move(lval)}};
    }
    return res;
}

ExprRes EvalExprVisitor::array_access_string(
    Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    assert(obj.type()->actual()->is(StringId));
    assert(idx.type()->actual()->is(IntId));

    auto char_type = builtins().char_type();

    // empty string or index value?
    if (obj.value().empty() || idx.value().empty()) {
        if (obj.value().is_rvalue())
            return {char_type, Value{RValue{}}};
        auto lval = obj.value().lvalue().derived();
        lval.set_is_consteval(false);
        return {char_type, Value{lval}};
    }

    auto int_idx = idx.value().copy_rvalue().get<Integer>();
    auto type = builtins().string_type();
    auto len = type->len(obj.value());
    if (int_idx < 0 || int_idx + 1 > len) {
        diag().error(node->index(), "char index is out of range");
        return {ExprError::CharIndexOutOfRange};
    }
    auto chr = type->chr(obj.value(), int_idx);
    bool is_consteval = obj.value().is_consteval();
    return {char_type, Value{RValue{(Unsigned)chr, is_consteval}}};
}

ExprRes EvalExprVisitor::array_access_array(
    Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    assert(obj.type()->actual()->is_array());
    assert(idx.type()->actual()->is(IntId));

    auto array_type = obj.type()->non_alias()->deref()->non_alias()->as_array();
    assert(array_type);
    auto item_type = array_type->item_type();
    auto array_val = obj.move_value();

    // empty array or index value?
    if (array_val.empty() || idx.value().empty())
        return {item_type, array_val.array_access(UnknownArrayIdx, false)};

    // check bounds
    bool is_consteval_idx = idx.value().is_consteval();
    auto int_idx = idx.value().copy_rvalue().get<Integer>();
    if (int_idx < 0 || int_idx + 1 > array_type->array_size()) {
        diag().error(node->index(), "array index is out of range");
        return {ExprError::ArrayIndexOutOfRange};
    }
    return {item_type, array_val.array_access(int_idx, is_consteval_idx)};
}

ExprRes EvalExprVisitor::member_access_op(
    Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Class> base) {
    auto cls = obj.type()->deref()->as_class();
    auto fset = cls->op(node->op());
    if (!fset) {
        diag().error(node, "operator not found");
        return {ExprError::NoOperator};
    }
    return bind(node, fset, std::move(obj), base);
}

ExprRes EvalExprVisitor::member_access_var(
    Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Var> var) {
    auto resolved = env().resolver(true).resolve(var);
    if (!var->has_type() && !resolved)
        return {ExprError::UnresolvableVar};
    return {var->type(), Value{var->lvalue()}};
}

ExprRes EvalExprVisitor::member_access_prop(
    Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Prop> prop) {
    return {prop->type(), obj.move_value().prop(prop)};
}

ExprRes EvalExprVisitor::member_access_fset(
    Ref<ast::MemberAccess> node,
    ExprRes&& obj,
    Ref<FunSet> fset,
    Ref<Class> base) {
    return bind(node, fset, std::move(obj), base);
}

ExprRes EvalExprVisitor::class_const_access(
    Ref<ast::ClassConstAccess> node, Ref<Var> var) {
    assert(var->has_cls());
    assert(var->is_const());

    auto resolver = env().resolver(true);
    if (!var->has_type() && !resolver.resolve(var))
        return {ExprError::UnresolvableVar};
    LValue lval{var};
    lval.set_is_xvalue(false);
    lval.set_scope_lvl(NoScopeLvl);
    return {var->type(), Value{LValue{var}}};
}

ExprRes EvalExprVisitor::bind(
    Ref<ast::Expr> node, Ref<FunSet> fset, ExprRes&& obj, Ref<Class> base) {
    assert(obj.type()->actual()->is_class());
    return {builtins().fun_type(), obj.move_value().bound_fset(fset, base)};
}

ExprRes EvalExprVisitor::as_base(
    Ref<ast::Expr> node, Ref<ast::BaseTypeSelect> base_type, ExprRes&& obj) {
    auto cls = obj.type()->deref()->as_class();
    cls = class_base(node, obj, cls, base_type);
    return obj.derived(cls, Value{obj.move_value().as(cls)});
}

ExprRes
EvalExprVisitor::assign(Ref<ast::Expr> node, ExprRes&& to, ExprRes&& from) {
    debug() << __FUNCTION__ << "\n" << line_at(node);

    if (!check_is_assignable(node, to.value()))
        return {ExprError::NotAssignable};

    auto to_type = to.type()->deref();
    auto from_type = from.type()->deref();

    if (!from_type->is_assignable_to(to_type, from.value()))
        from = env().cast(node, to_type, std::move(from), true);

    auto lval = to.move_value().lvalue();
    if (has_flag(evl::NoExec))
        return std::move(to);
    return to.derived(to.type(), lval.assign(from.move_value().move_rvalue()));
}

ExprRes EvalExprVisitor::negate(Ref<ast::Expr> node, ExprRes&& res) {
    assert(res.type()->is(BoolId));
    auto val = res.move_value();
    if (!val.empty()) {
        auto type = dynamic_cast<BoolType*>(res.type());
        assert(type);
        auto rval = type->construct(!type->is_true(val.move_rvalue()));
        val = Value{std::move(rval)};
    }
    return res.derived(res.type(), std::move(val));
}

ExprResList EvalExprVisitor::eval_args(Ref<ast::ArgList> args) {
    debug() << __FUNCTION__ << "\n" << line_at(args);

    ExprResList list;
    for (unsigned n = 0; n < args->child_num(); ++n) {
        // TODO: default arguments
        ExprRes arg_res = args->get(n)->accept(*this);
        list.push_back(std::move(arg_res));
        if (!list)
            return list;
    }
    return list;
}

} // namespace ulam::sema
