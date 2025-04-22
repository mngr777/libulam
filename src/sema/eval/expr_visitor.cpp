#include "libulam/sema/eval/flags.hpp"
#include <cassert>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
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
        auto type = eval()
                        .resolver(true, flags())
                        ->resolve_type_name(node->type_name(), scope(), true);
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
    auto sym =
        node->is_local() ? scope()->get_local(name_id) : scope()->get(name_id);
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

    // TODO: special case for short-circuiting
    auto left = node->lhs()->accept(*this);
    if (!left)
        return left;

    auto right = node->rhs()->accept(*this);
    if (!right)
        return right;

    auto res = binary_op(
        node, node->op(), node->lhs(), std::move(left), node->rhs(),
        std::move(right));
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
    auto cast_type =
        eval()
            .resolver(true, flags())
            ->resolve_full_type_name(node->full_type_name(), scope());
    if (!cast_type)
        return {ExprError::InvalidCast};

    // (Void) expr
    if (cast_type->is(VoidId))
        return {builtins().void_type(), Value{RValue{}}};

    auto cast = eval().cast_helper(scope(), flags());
    res = cast->cast(node, cast_type, std::move(res), true);
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::Ternary> node) {
    debug() << __FUNCTION__ << " Ternary\n" << line_at(node);

    // eval condition
    assert(node->has_cond());
    auto cond_res = node->cond()->accept(*this);
    if (!cond_res)
        return cond_res;

    // cast bo Bool(1)
    auto boolean = builtins().boolean();
    auto cast = eval().cast_helper(scope(), flags());
    cond_res = cast->cast(node, boolean, cond_res.move_typed_value());
    if (!cond_res)
        return cond_res;

    // select and eval expr
    if (boolean->is_true(cond_res.move_value().move_rvalue()))
        return node->if_true()->accept(*this);
    return node->if_false()->accept(*this);
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

    auto funcall = eval().funcall_helper(scope(), flags());
    auto res = funcall->funcall(node, std::move(callable), std::move(args));
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

    // obj.Base.bar?
    if (node->has_base()) {
        obj = as_base(node, node->base(), std::move(obj));
        if (!obj)
            return obj;
    }

    // get op fset
    if (node->is_op())
        return check(node, member_access_op(node, std::move(obj)));

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
            return member_access_fset(node, std::move(obj), fset);
        },
        [&](auto other) -> ExprRes { assert(false); });
    return check(node, std::move(res));
}

ExprRes EvalExprVisitor::visit(Ref<ast::ClassConstAccess> node) {
    debug() << __FUNCTION__ << "ClassConstAccess" << line_at(node);

    auto resolver = eval().resolver(true, flags());
    auto type = resolver->resolve_class_name(node->type_name(), scope(), false);
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
    // TODO: base class prop/fun
    // TODO: move resolution to class?
    auto var = sym->get<Var>();
    auto scope = cls->scope();
    auto scope_view = scope->view(var->scope_version());
    // if (!resolver->resolve(var, ref(scope_view)))
    //     return {ExprError::UnresolvableClassConst};
    return check(node, {var->type(), Value{var->rvalue()}});
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
    auto cast = eval().cast_helper(scope(), flags());
    idx = cast->cast(node->index(), IntId, std::move(idx));
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
    if (res && !res.value().is_consteval()) {
        if (has_flag(evl::Consteval)) {
            diag().error(node, "not consteval");
            return {ExprError::NotConsteval};
        }
        if (has_flag(evl::NoExec)) {
            const auto& val = res.value();
            auto empty = val.is_lvalue() ? Value{val.lvalue().derived()}
                                         : Value{RValue{}};
            return res.derived(res.type(), std::move(empty));
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
    Ref<ast::Expr> node, Ref<Class> cls, Ref<ast::TypeIdent> ident) {
    if (ident->is_super())
        return class_super(node, cls);
    assert(!ident->is_self());

    // search in class scope
    auto sym = cls->scope()->get(ident->name_id());
    if (!sym) {
        diag().error(ident, "base type not found");
        return {};
    }
    assert(sym->is<UserType>());
    auto type = sym->get<UserType>();

    // resolve aliases
    if (type->is_alias()) {
        auto resolver = eval().resolver(true, flags());
        if (!resolver->resolve(type->as_alias()))
            return {};
    }

    if (!type->is_class()) {
        diag().error(ident, "type is not a class");
        return {};
    }

    auto base = type->as_class();
    if (!base->is_same_or_base_of(cls)) {
        diag().error(ident, std::string{"not an ancestor of "} + cls->name());
        return {};
    }
    return base;
}

std::pair<bool, bool> EvalExprVisitor::match(
    Ref<ast::Expr> var_expr, Ref<Var> var, Ref<ast::Expr> expr) {
    ExprRes l_res{var->type(), Value{var->lvalue()}};
    ExprRes r_res = expr->accept(*this);
    if (!r_res)
        return {false, false};

    // apply == op
    ExprRes res = binary_op(
        expr, Op::Equal, var_expr, std::move(l_res), expr, std::move(r_res));
    if (!res)
        return {false, false};

    // cast to Bool(1) just in case
    auto boolean = builtins().boolean();
    auto cast = eval().cast_helper(scope(), flags());
    res = cast->cast(expr, boolean, res.move_typed_value());
    if (!res)
        return {false, false};

    return {boolean->is_true(res.move_value().move_rvalue()), true};
}

bitsize_t
EvalExprVisitor::bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id) {
    debug() << __FUNCTION__ << "\n" << line_at(expr);
    assert(bi_type_id != NoBuiltinTypeId);

    // can have bitsize?
    if (!has_bitsize(bi_type_id)) {
        diag().error(
            expr, std::string{builtin_type_str(bi_type_id)} +
                      " does not have bitsize parameter");
        return NoBitsize;
    }

    // eval
    ExprRes res = expr->accept(*this);
    if (!res)
        return NoBitsize;

    // cast to default Unsigned
    auto uns_type = builtins().unsigned_type();
    auto cast = eval().cast_helper(scope(), flags());
    res = cast->cast(expr, uns_type, std::move(res), true);
    if (!res)
        return NoBitsize;
    auto rval = res.move_value().move_rvalue();
    auto size = rval.get<Unsigned>();

    // check range
    auto tpl = builtins().prim_type_tpl(bi_type_id);
    if (size < tpl->min_bitsize()) {
        auto message = std::string{"min size for "} +
                       std::string{builtin_type_str(bi_type_id)} + " is " +
                       std::to_string(tpl->min_bitsize());
        diag().error(expr, message);
        return NoBitsize;
    }
    if (size > tpl->max_bitsize()) {
        auto message = std::string{"max size for "} +
                       std::string{builtin_type_str(bi_type_id)} + " is " +
                       std::to_string(tpl->max_bitsize());
        diag().error(expr, message);
        return NoBitsize;
    }
    return size;
}

array_size_t EvalExprVisitor::array_size(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n" << line_at(expr);
    ExprRes res = expr->accept(*this);
    if (!res)
        return UnknownArraySize;

    // cast to default Int
    auto int_type = builtins().int_type();
    auto cast = eval().cast_helper(scope(), flags());
    res = cast->cast(expr, int_type, std::move(res));
    if (!res)
        return UnknownArraySize;

    auto rval = res.move_value().move_rvalue();
    if (rval.empty())
        return UnknownArraySize;
    assert(rval.is_consteval());

    auto int_val = rval.get<Integer>();
    if (int_val < 0) {
        diag().error(expr, "array index is < 0");
        return UnknownArraySize;
    }
    return (array_size_t)int_val;
}

std::pair<TypedValueList, bool>
EvalExprVisitor::eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl) {
    debug() << __FUNCTION__ << "\n" << line_at(args);

    // tmp param eval scope
    auto param_scope_view = tpl->param_scope()->view(0);
    auto param_scope = make<BasicScope>(ref(param_scope_view));
    std::list<Ref<Var>> cls_params;

    auto resolver = eval().resolver(true, flags());
    auto cast = eval().cast_helper(scope(), flags());
    std::pair<TypedValueList, bool> res;
    res.second = false;
    unsigned n = 0;
    for (auto param : tpl->params()) {
        // param type
        auto type =
            resolver->resolve_type_name(param->type_node(), ref(param_scope));
        if (!type)
            return res;

        // value
        ExprRes arg_res;
        if (n < args->child_num()) {
            // argument provided
            auto arg = args->get(n);
            arg_res = arg->accept(*this);
            if (arg_res)
                arg_res = cast->cast(arg, type, std::move(arg_res), false);

        } else if (param->node()->has_init()) {
            // default argument
            auto init = eval().init_helper(ref(param_scope), flags());
            arg_res = init->eval_init(type, param->node()->init(), true);

        } else {
            diag().error(args, "not enough arguments");
            return res;
        }
        ++n;
        if (!arg_res)
            return res;

        // create var in tmp scope
        auto var = make<Var>(
            param->type_node(), param->node(), type, arg_res.move_value());
        cls_params.push_back(ref(var)); // add to list
        param_scope->set(param->name_id(), std::move(var));
    }
    res.second = true;

    for (auto param : cls_params)
        res.first.emplace_back(param->type(), param->move_value());
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

    LValue lval;
    if (ops::is_assign(op)) {
        if (!check_is_assignable(node, left.value()))
            return {ExprError::NotAssignable};
        lval = left.value().lvalue();
    }

    auto cast = eval().cast_helper(scope(), flags());
    auto type_errors =
        binary_op_type_check(op, left.type(), right.typed_value());
    auto recast = [&](Ref<ast::Expr> expr, TypeError error,
                      ExprRes&& arg) -> ExprRes {
        switch (error.status) {
        case TypeError::Incompatible:
            diag().error(expr, "incompatible type");
            return {ExprError::InvalidOperandType};
        case TypeError::ExplCastRequired: {
            auto rval = arg.move_value().move_rvalue();
            auto message =
                std::string{"suggest casting "} + arg.type()->name() + " to ";
            message +=
                (error.cast_bi_type_id != NoBuiltinTypeId)
                    ? std::string{builtin_type_str(error.cast_bi_type_id)}
                    : error.cast_type->name();
            diag().error(expr, message);
            return {ExprError::InvalidOperandType};
        }
        case TypeError::ImplCastRequired: {
            if (error.cast_bi_type_id != NoBuiltinTypeId)
                return cast->cast(expr, error.cast_bi_type_id, std::move(arg));
            assert(error.cast_type);
            return cast->cast(expr, error.cast_type, std::move(arg));
        }
        case TypeError::Ok:
            return std::move(arg);
        default:
            assert(false);
        };
    };

    // cast left
    left = recast(l_node, type_errors.first, std::move(left));
    if (!left)
        return std::move(left);

    // cast right
    right = recast(r_node, type_errors.second, std::move(right));
    if (!right)
        return std::move(right);

    return apply_binary_op(
        node, op, lval, l_node, std::move(left), r_node, std::move(right));
}

ExprRes EvalExprVisitor::apply_binary_op(
    Ref<ast::Expr> node,
    Op op,
    LValue lval,
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
            auto fset = cls->op(op);
            assert(fset);
            left = bind(node, fset, std::move(left));
            ExprResList args;
            args.push_back(std::move(right));
            auto funcall = eval().funcall_helper(scope(), flags());
            return funcall->funcall(node, std::move(left), std::move(args));
        }
    }

    // handle assignment
    if (ops::is_assign(op)) {
        if (lval.empty() || right.value().empty())
            return std::move(left);
        return assign(node, {l_type, Value{lval}}, right.move_typed_value());
    }
    return std::move(right);
}

ExprRes EvalExprVisitor::unary_op(
    Ref<ast::Expr> node,
    Op op,
    Ref<ast::Expr> arg_node,
    ExprRes&& arg,
    Ref<ast::TypeName> type_name) {

    LValue lval;
    if (ops::is_inc_dec(op)) {
        // store lvalue
        if (!check_is_assignable(node, arg.value()))
            return {ExprError::NotAssignable};
        lval = arg.value().lvalue();
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
        auto cast = eval().cast_helper(scope(), flags());
        arg = cast->cast(node, error.cast_bi_type_id, std::move(arg));
        if (!arg)
            return std::move(arg);
    } break;
    case TypeError::Ok:
        break;
    }

    Ref<Type> type{};
    if (type_name) {
        auto resolver = eval().resolver(true, flags());
        type = resolver->resolve_type_name(type_name, scope());
        if (!type)
            return {ExprError::UnresolvableType};
    }

    return apply_unary_op(node, op, lval, arg_node, std::move(arg), type);
}

ExprRes EvalExprVisitor::apply_unary_op(
    Ref<ast::Expr> node,
    Op op,
    LValue lval,
    Ref<ast::Expr> arg_node,
    ExprRes&& arg,
    Ref<Type> type) {

    auto arg_type = arg.type()->deref();
    if (arg_type->is_prim()) {
        auto tv =
            arg_type->as_prim()->unary_op(op, arg.move_value().move_rvalue());
        RValue orig_rval;
        if (ops::is_unary_pre_op(op))
            orig_rval = tv.value().copy_rvalue();

        if (ops::is_inc_dec(op)) {
            if (!lval.empty() && !tv.value().empty())
                assign(node, TypedValue{tv.type(), Value{lval}}, std::move(tv));
            if (ops::is_unary_pre_op(op))
                return {tv.type(), Value{lval}};
            assert(ops::is_unary_post_op(op));
            return {tv.type(), Value{std::move(orig_rval)}};
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
            auto dyn_type = arg.value().dyn_obj_type();
            bool is =
                dyn_type->is_same(type) || dyn_type->is_impl_castable_to(type);
            auto boolean = builtins().boolean();
            return {boolean, Value{boolean->construct(is)}};

        } else if (arg_type->is_class()) {
            auto cls = arg_type->as_class();
            auto fset = cls->op(op);
            assert(fset);
            ExprResList args;
            if (op == Op::PostInc || op == Op::PostDec) {
                // dummy argument for post inc/dec
                auto int_type = builtins().int_type();
                auto rval = int_type->construct();
                rval.set_is_consteval(true);
                args.push_back({int_type, Value{std::move(rval)}});
            }
            arg = bind(node, fset, std::move(arg));
            auto funcall = eval().funcall_helper(scope(), flags());
            return funcall->funcall(node, std::move(arg), std::move(args));
        }
    }
    assert(false);
}

ExprRes EvalExprVisitor::type_op(Ref<ast::TypeOpExpr> node, Ref<Type> type) {
    if (type->is_class()) {
        // instanceof with arguments?
        auto cls = type->as_class();
        if (node->op() == TypeOp::InstanceOf && node->has_args()) {
            auto args = eval_args(node->args());
            if (!args)
                return {args.error()};
            auto funcall = eval().funcall_helper(scope(), flags());
            return funcall->construct(node, cls, std::move(args));
        }
    }

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
    if (node->has_base()) {
        arg = as_base(node, node->base(), std::move(arg));
        if (!arg)
            return std::move(arg);
    }

    // class type op may require an evaluator
    if (arg.type()->is_class()) {
        // custom lengthof?
        auto cls = arg.type()->as_class();
        if (node->op() == TypeOp::LengthOf && cls->has_fun("alengthof")) {
            return type_op_expr_fun(
                node, cls->fun("alengthof"), std::move(arg));
        }
    }
    return type_op_expr_default(node, std::move(arg));
}

ExprRes EvalExprVisitor::type_op_expr_fun(
    Ref<ast::TypeOpExpr> node, Ref<FunSet> fset, ExprRes&& arg) {
    arg = bind(node, fset, std::move(arg));
    auto funcall = eval().funcall_helper(scope(), flags());
    return funcall->funcall(node, std::move(arg), {});
}

ExprRes EvalExprVisitor::type_op_expr_default(
    Ref<ast::TypeOpExpr> node, ExprRes&& arg) {
    auto val = arg.move_value();
    auto tv = arg.type()->actual()->type_op(node->op(), val);
    if (!tv)
        return {ExprError::InvalidTypeOperator};
    return tv;
}

ExprRes EvalExprVisitor::ident_self(Ref<ast::Ident> node) {
    auto self = scope()->self();
    return {scope()->eff_self_cls(), Value{self}};
}

ExprRes EvalExprVisitor::ident_super(Ref<ast::Ident> node) {
    auto self_cls = scope()->self_cls();
    auto sup = class_super(node, self_cls);
    if (!sup)
        return {ExprError::NoSuper};
    auto self = scope()->self();
    return {sup, Value{self.as(sup)}};
}

ExprRes EvalExprVisitor::ident_var(Ref<ast::Ident> node, Ref<Var> var) {
    if (!var->has_type() &&
        !eval().resolver(true, flags())->resolve(var, scope()))
        return {ExprError::UnresolvableVar};
    return {var->type(), Value{var->lvalue()}};
}

ExprRes EvalExprVisitor::ident_prop(Ref<ast::Ident> node, Ref<Prop> prop) {
    return {prop->type(), Value{scope()->self().prop(prop)}};
}

ExprRes EvalExprVisitor::ident_fset(Ref<ast::Ident> node, Ref<FunSet> fset) {
    return {builtins().fun_type(), Value{scope()->self().bound_fset(fset)}};
}

ExprRes EvalExprVisitor::callable_op(Ref<ast::FunCall> node) {
    assert(node->is_op_call());
    auto lval = scope()->self();
    auto cls = scope()->eff_self_cls();
    auto fset = cls->op(node->fun_op());
    if (!fset) {
        diag().error(node, "operator not found in class");
        return {ExprError::NoOperator};
    }
    return {builtins().fun_type(), Value{lval.bound_fset(fset)}};
}

ExprRes EvalExprVisitor::array_access_class(
    Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    assert(obj.type()->is_class());

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

    auto funcall = eval().funcall_helper(scope(), flags());
    ExprRes res = funcall->funcall(node, std::move(callable), std::move(args));
    if (is_tmp && res.value().is_lvalue()) {
        LValue lval = res.move_value().lvalue();
        lval.set_is_xvalue(true);
        return {res.type(), Value{std::move(lval)}};
    }
    return res;
}

ExprRes EvalExprVisitor::array_access_string(
    Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    assert(obj.type()->is(StringId));
    assert(idx.type()->is(IntId));

    auto int_idx = idx.value().copy_rvalue().get<Integer>();
    auto type = builtins().string_type();
    auto len = type->len(obj.value());
    if (int_idx < 0 || int_idx + 1 > len) {
        diag().error(node->index(), "char index is out of range");
        return {ExprError::CharIndexOutOfRange};
    }
    auto chr = type->chr(obj.value(), int_idx);
    bool is_consteval = obj.value().is_consteval();
    return {builtins().char_type(), Value{RValue{(Unsigned)chr, is_consteval}}};
}

ExprRes EvalExprVisitor::array_access_array(
    Ref<ast::ArrayAccess> node, ExprRes&& obj, ExprRes&& idx) {
    assert(obj.type()->is_array());
    assert(idx.type()->is(IntId));

    auto array_type = obj.type()->non_alias()->deref()->non_alias()->as_array();
    assert(array_type);
    auto item_type = array_type->item_type();
    auto array_val = obj.move_value();

    // empty array or index value?
    if (array_val.empty() || idx.value().empty())
        return {item_type, array_val.array_access(UnknownArrayIdx)};

    // check bounds
    auto int_idx = idx.value().copy_rvalue().get<Integer>();
    if (int_idx < 0 || int_idx + 1 > array_type->array_size()) {
        diag().error(node->index(), "array index is out of range");
        return {ExprError::ArrayIndexOutOfRange};
    }
    return {item_type, array_val.array_access(int_idx)};
}

ExprRes
EvalExprVisitor::member_access_op(Ref<ast::MemberAccess> node, ExprRes&& obj) {
    auto cls = obj.type()->as_class();
    auto fset = cls->op(node->op());
    if (!fset) {
        diag().error(node, "operator not found");
        return {ExprError::NoOperator};
    }
    return bind(node, fset, std::move(obj));
}

ExprRes EvalExprVisitor::member_access_var(
    Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Var> var) {
    if (!var->has_type() && !eval().resolver(true, flags())->resolve(var))
        return {ExprError::UnresolvableVar};
    return {var->type(), Value{var->lvalue()}};
}

ExprRes EvalExprVisitor::member_access_prop(
    Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<Prop> prop) {
    return {prop->type(), obj.move_value().prop(prop)};
}

ExprRes EvalExprVisitor::member_access_fset(
    Ref<ast::MemberAccess> node, ExprRes&& obj, Ref<FunSet> fset) {
    return {builtins().fun_type(), obj.move_value().bound_fset(fset)};
}

ExprRes
EvalExprVisitor::bind(Ref<ast::Expr> node, Ref<FunSet> fset, ExprRes&& obj) {
    assert(obj.type()->is_class());
    return {builtins().fun_type(), obj.move_value().bound_fset(fset)};
}

ExprRes EvalExprVisitor::as_base(
    Ref<ast::Expr> node, Ref<ast::TypeIdent> base, ExprRes&& obj) {
    auto cls = obj.type()->as_class();
    cls = class_base(node, cls, base);
    if (!cls) {
        auto error =
            base->is_super() ? ExprError::NoSuper : ExprError::BaseNotFound;
        return {error};
    }
    return {cls, Value{obj.move_value().as(cls)}};
}

ExprRes
EvalExprVisitor::assign(Ref<ast::Expr> node, TypedValue&& to, TypedValue&& tv) {
    debug() << __FUNCTION__ << "\n" << line_at(node);

    if (!check_is_assignable(node, to.value()))
        return {ExprError::NotAssignable};

    auto cast = eval().cast_helper(scope(), flags());
    auto res = cast->cast(node, to.type()->deref(), std::move(tv));
    if (!res)
        return res;

    auto lval = to.move_value().lvalue();
    if (flags() & evl::NoExec) {
        return {to.type(), Value{lval}};
    }
    return {to.type(), lval.assign(res.move_value().move_rvalue())};
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
