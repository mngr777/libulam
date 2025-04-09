#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/fun.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type/ops.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::EvalExprVisitor] "
#endif
#include "src/debug.hpp"

#define DBG_LINE(node) debug() << _program->sm().line_at(node->loc_id())

namespace ulam::sema {

EvalExprVisitor::EvalExprVisitor(EvalVisitor& eval, Ref<Scope> scope):
    _eval{eval}, _program{eval._program}, _scope{scope} {}

ExprRes EvalExprVisitor::visit(Ref<ast::TypeOpExpr> node) {
    debug() << __FUNCTION__ << " TypeOpExpr\n";
    DBG_LINE(node);
    if (node->has_type_name()) {
        auto type = _eval.resolver()->resolve_type_name(
            node->type_name(), _scope, true);
        if (!type)
            return {ExprError::UnresolvableType};
        return type_op(node, type);
    } else {
        assert(node->has_expr());
        auto expr_res = node->expr()->accept(*this);
        if (!expr_res)
            return expr_res;
        return type_op(node, std::move(expr_res));
    }
}

ExprRes EvalExprVisitor::visit(Ref<ast::Ident> node) {
    debug() << __FUNCTION__ << " Ident `" << str(node->name().str_id())
            << "`\n";
    DBG_LINE(node);

    // self
    if (node->is_self())
        return ident_self(node);

    // super
    if (node->is_super())
        return ident_super(node);

    auto name_id = node->name().str_id();
    auto sym =
        node->is_local() ? _scope->get_local(name_id) : _scope->get(name_id);
    if (!sym) {
        diag().error(node, "symbol not found");
        return {ExprError::SymbolNotFound};
    }

    using std::placeholders::_1;
    return sym->accept(
        std::bind(std::mem_fn(&EvalExprVisitor::ident_var), this, node, _1),
        std::bind(std::mem_fn(&EvalExprVisitor::ident_prop), this, node, _1),
        std::bind(std::mem_fn(&EvalExprVisitor::ident_fset), this, node, _1),
        [&](auto value) -> ExprRes { assert(false); });
}

ExprRes EvalExprVisitor::visit(Ref<ast::ParenExpr> node) {
    debug() << __FUNCTION__ << " ParenExpr\n";
    DBG_LINE(node);
    return node->inner()->accept(*this);
}

ExprRes EvalExprVisitor::visit(Ref<ast::BinaryOp> node) {
    debug() << __FUNCTION__ << " BinaryOp\n";
    DBG_LINE(node);
    assert(node->has_lhs() && node->has_rhs());

    // TODO: special case for short-circuiting
    auto left = node->lhs()->accept(*this);
    if (!left)
        return left;

    auto right = node->rhs()->accept(*this);
    if (!right)
        return right;

    return binary_op(
        node, node->op(), node->lhs(), std::move(left), node->rhs(),
        std::move(right));
}

ExprRes EvalExprVisitor::visit(Ref<ast::UnaryOp> node) {
    debug() << __FUNCTION__ << " UnaryOp\n";
    DBG_LINE(node);
    auto arg = node->arg()->accept(*this);
    if (!arg)
        return arg;

    return unary_op(node, node->op(), node->arg(), std::move(arg));
}

ExprRes EvalExprVisitor::visit(Ref<ast::Cast> node) {
    debug() << __FUNCTION__ << " Cast\n";
    DBG_LINE(node);
    // eval expr
    auto res = node->expr()->accept(*this);
    if (!res)
        return res;

    // resolve target type
    auto cast_type = _eval.resolver()->resolve_full_type_name(
        node->full_type_name(), _scope);
    if (!cast_type)
        return {ExprError::InvalidCast};

    // (Void) expr
    if (cast_type->is(VoidId))
        return {builtins().void_type(), Value{RValue{}}};

    auto cast = _eval.cast_helper(_scope);
    res = cast->cast(node, cast_type, std::move(res), true);
    return res;
}

ExprRes EvalExprVisitor::visit(Ref<ast::Ternary> node) {
    debug() << __FUNCTION__ << " Ternary\n";
    DBG_LINE(node);

    // eval condition
    assert(node->has_cond());
    auto cond_res = node->cond()->accept(*this);
    if (!cond_res)
        return cond_res;

    // cast bo Bool(1)
    auto boolean = builtins().boolean();
    auto cast = _eval.cast_helper(_scope);
    cond_res = cast->cast(node, boolean, cond_res.move_typed_value());
    if (!cond_res)
        return cond_res;

    // select and eval expr
    if (boolean->is_true(cond_res.move_value().move_rvalue()))
        return node->if_true()->accept(*this);
    return node->if_false()->accept(*this);
}

ExprRes EvalExprVisitor::visit(Ref<ast::BoolLit> node) {
    debug() << __FUNCTION__ << " BoolLit\n";
    DBG_LINE(node);
    // Bool(1)
    auto type = builtins().boolean();
    auto rval = type->construct(node->value());
    rval.set_is_consteval(true);
    return {type, Value{std::move(rval)}};
}

ExprRes EvalExprVisitor::visit(Ref<ast::NumLit> node) {
    debug() << __FUNCTION__ << " NumLit\n";
    DBG_LINE(node);
    const auto& number = node->value();
    if (number.is_signed()) {
        // Int(n)
        auto type = builtins().int_type(number.bitsize());
        return {type, Value{RValue{number.value<Integer>(), true}}};
    } else {
        // Unsigned(n)
        auto type = builtins().unsigned_type(number.bitsize());
        return {type, Value{RValue{number.value<Unsigned>(), true}}};
    }
}

ExprRes EvalExprVisitor::visit(Ref<ast::StrLit> node) {
    debug() << __FUNCTION__ << " StrLit\n";
    DBG_LINE(node);
    auto type = builtins().type(StringId);
    return {type, Value{RValue{node->value(), true}}};
}

ExprRes EvalExprVisitor::visit(Ref<ast::FunCall> node) {
    debug() << __FUNCTION__ << " FunCall\n";
    DBG_LINE(node);

    // obj.fun | operator<op>
    auto callable = node->has_callable() ? node->callable()->accept(*this)
                                         : callable_op(node);

    // args
    auto args = eval_args(node->args());
    if (!args)
        return {args.error()};

    auto funcall = _eval.funcall_helper(_scope);
    return funcall->funcall(node, std::move(callable), std::move(args));
}

ExprRes EvalExprVisitor::visit(Ref<ast::MemberAccess> node) {
    debug() << __FUNCTION__ << " MemberAccess\n";
    DBG_LINE(node);
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
        return member_access_op(node, std::move(obj));

    // get symbol
    auto cls = obj.type()->as_class();
    auto name = node->ident()->name();
    auto sym = cls->get(name.str_id());
    if (!sym) {
        diag().error(node->ident(), "member not found");
        return {ExprError::MemberNotFound};
    }

    return sym->accept(
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
}

ExprRes EvalExprVisitor::visit(Ref<ast::ClassConstAccess> node) {
    debug() << __FUNCTION__ << "ClassConstAccess";
    DBG_LINE(node);
    auto resolver = _eval.resolver();
    auto type = resolver->resolve_class_name(node->type_name(), _scope, false);
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
    // TODO: move resolution to class
    auto var = sym->get<Var>();
    auto scope = cls->scope();
    auto scope_view = scope->view(var->scope_version());
    if (!resolver->resolve(var, ref(scope_view)))
        return {ExprError::UnresolvableClassConst};
    return {var->type(), Value{var->rvalue()}};
}

ExprRes EvalExprVisitor::visit(Ref<ast::ArrayAccess> node) {
    debug() << __FUNCTION__ << " ArrayAccess\n";
    DBG_LINE(node);
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
    if (obj.type()->actual()->is_class())
        return array_access_class(node, std::move(obj), std::move(idx));

    // cast to index type
    auto cast = _eval.cast_helper(_scope);
    idx = cast->cast(node->index(), IntId, std::move(idx));
    if (!idx)
        return idx;

    // string?
    if (obj.type()->actual()->is(StringId))
        return array_access_string(node, std::move(obj), std::move(idx));

    // must be an array
    if (!obj.type()->actual()->is_array()) {
        diag().error(node->array(), "not an array");
        return {ExprError::NotArray};
    }
    return array_access_array(node, std::move(obj), std::move(idx));
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
        auto resolver = _eval.resolver();
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
    auto cast = _eval.cast_helper(_scope);
    res = cast->cast(expr, boolean, res.move_typed_value());
    if (!res)
        return {false, false};

    return {boolean->is_true(res.move_value().move_rvalue()), true};
}

bitsize_t
EvalExprVisitor::bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(expr);
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
    auto cast = _eval.cast_helper(_scope);
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
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(expr);
    ExprRes res = expr->accept(*this);
    if (!res)
        return UnknownArraySize;

    // cast to default Int
    auto int_type = builtins().int_type();
    auto cast = _eval.cast_helper(_scope);
    res = cast->cast(expr, int_type, res.move_typed_value());
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
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(args);

    // tmp param eval scope
    auto param_scope_view = tpl->param_scope()->view(0);
    auto scope = make<BasicScope>(ref(param_scope_view));
    std::list<Ref<Var>> cls_params;

    auto resolver = _eval.resolver();
    auto cast = _eval.cast_helper(_scope);
    std::pair<TypedValueList, bool> res;
    res.second = false;
    unsigned n = 0;
    for (auto param : tpl->params()) {
        // param type
        auto type = resolver->resolve_type_name(param->type_node(), ref(scope));
        if (!type)
            return res;

        // value
        Value val{RValue{}};
        if (n < args->child_num()) {
            // argument provided
            auto arg = args->get(n);
            auto arg_res = arg->accept(*this);
            if (arg_res)
                arg_res =
                    cast->cast(arg, type, arg_res.move_typed_value(), false);
            if (!arg_res)
                return res;
            val = arg_res.move_value();

        } else if (param->node()->has_init()) {
            // default argument
            auto init = _eval.init_helper(ref(scope));
            bool ok{};
            std::tie(val, ok) = init->eval(type, param->node()->init());
            if (!ok)
                return res;

        } else {
            diag().error(args, "not enough arguments");
            return res;
        }
        ++n;

        // create var in tmp scope
        auto var =
            make<Var>(param->type_node(), param->node(), type, std::move(val));
        cls_params.push_back(ref(var)); // add to list
        scope->set(param->name_id(), std::move(var));
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
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(node);

    LValue lval;
    if (ops::is_assign(op)) {
        if (!check_is_assignable(node, left.value()))
            return {ExprError::NotAssignable};
        lval = left.value().lvalue();
    }

    auto cast = _eval.cast_helper(_scope);
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
    auto l_tv = left.move_typed_value();
    auto r_tv = right.move_typed_value();
    if (l_tv.type()->actual()->is_prim()) {
        if (op != Op::Assign) {
            // primitive binary op
            assert(r_tv.type()->actual()->is_prim());
            auto l_type = l_tv.type()->actual()->as_prim();
            auto r_type = r_tv.type()->actual()->as_prim();
            auto l_rval = l_tv.move_value().move_rvalue();
            auto r_rval = r_tv.move_value().move_rvalue();
            r_tv = l_type->binary_op(
                op, std::move(l_rval), r_type, std::move(r_rval));
        }
    } else if (l_tv.type()->actual()->is_class()) {
        // class op
        auto cls = l_tv.type()->actual()->as_class();
        if (op != Op::Assign || cls->has_op(Op::Assign)) {
            auto cls = l_tv.type()->actual()->as_class();
            auto fset = cls->op(op);
            assert(fset);
            auto obj = l_tv.move_value();
            ExprResList args;
            args.push_back(std::move(right));
            auto funcall = _eval.funcall_helper(_scope);
            return funcall->funcall(node, fset, obj.self(), std::move(args));
        }
    }

    // handle assignment
    if (ops::is_assign(op)) {
        if (lval.empty() || r_tv.value().empty())
            return {std::move(l_tv)};
        return assign(
            node, TypedValue{l_tv.type(), Value{lval}}, std::move(r_tv));
    }
    return {std::move(r_tv)};
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
        auto cast = _eval.cast_helper(_scope);
        arg = cast->cast(node, error.cast_bi_type_id, std::move(arg));
        if (!arg)
            return std::move(arg);
    } break;
    case TypeError::Ok:
        break;
    }
    return apply_unary_op(node, op, lval, arg_node, std::move(arg));
}

ExprRes EvalExprVisitor::apply_unary_op(
    Ref<ast::Expr> node,
    Op op,
    LValue lval,
    Ref<ast::Expr> arg_node,
    ExprRes&& arg,
    Ref<ast::TypeName> type_name) {

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
            assert(type_name);
            auto type = _eval.resolver()->resolve_type_name(type_name, _scope);
            if (!type)
                return {ExprError::UnresolvableType};
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
                auto int_type = _program->builtins().int_type();
                auto rval = int_type->construct();
                rval.set_is_consteval(true);
                args.push_back({int_type, Value{std::move(rval)}});
            }
            arg = bind(node, fset, std::move(arg));
            auto funcall = _eval.funcall_helper(_scope);
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
            auto rval = cls->construct();
            auto funcall = _eval.funcall_helper(_scope);
            funcall->funcall(
                node, cls->constructors(), rval.self(), std::move(args));
            return {type, Value{std::move(rval)}};
        }
    }

    auto tv = type->actual()->type_op(node->op());
    if (!tv) {
        diag().error(node, "invalid type operator");
        return {ExprError::InvalidTypeOperator};
    }
    return tv;
}

ExprRes EvalExprVisitor::type_op(Ref<ast::TypeOpExpr> node, ExprRes res) {
    auto type = res.type();
    auto val = res.move_value();

    // obj.Base.<type_op>?
    if (node->has_base()) {
        if (!type->is_class()) {
            diag().error(node->base(), "not an object");
            return {ExprError::NotObject};
        }
        auto cls = class_base(node, type->as_class(), node->base());
        if (!cls) {
            auto error = node->base()->is_super() ? ExprError::NoSuper
                                                  : ExprError::BaseNotFound;
            return {error};
        }
        val = Value{val.as(cls)};
    }

    // class type op may require an evaluator
    if (type->is_class()) {
        // custom lengthof?
        auto cls = type->as_class();
        if (node->op() == TypeOp::LengthOf && cls->has_fun("alengthof")) {
            if (!val.has_rvalue())
                return {builtins().unsigned_type(), Value{RValue{}}};
            auto funcall = _eval.funcall_helper(_scope);
            ExprResList args;
            return funcall->funcall(
                node, cls->fun("alengthof"), val.self(), std::move(args));
        }
    }

    auto tv = type->actual()->type_op(node->op(), val);
    if (!tv)
        return {ExprError::InvalidTypeOperator};
    return tv;
}

ExprRes EvalExprVisitor::ident_self(Ref<ast::Ident> node) {
    auto self = _scope->self();
    return {_scope->eff_self_cls(), Value{self}};
}

ExprRes EvalExprVisitor::ident_super(Ref<ast::Ident> node) {
    auto self_cls = _scope->self_cls();
    auto sup = class_super(node, self_cls);
    if (!sup)
        return {ExprError::NoSuper};
    auto self = _scope->self();
    return {sup, Value{self.as(sup)}};
}

ExprRes EvalExprVisitor::ident_var(Ref<ast::Ident> node, Ref<Var> var) {
    if (!_eval.resolver()->resolve(var, _scope))
        return {ExprError::UnresolvableVar};
    return {var->type(), Value{var->lvalue()}};
}

ExprRes EvalExprVisitor::ident_prop(Ref<ast::Ident> node, Ref<Prop> prop) {
    return {prop->type(), Value{_scope->self().prop(prop)}};
}

ExprRes EvalExprVisitor::ident_fset(Ref<ast::Ident> node, Ref<FunSet> fset) {
    return {builtins().fun_type(), Value{_scope->self().bound_fset(fset)}};
}

ExprRes EvalExprVisitor::callable_op(Ref<ast::FunCall> node) {
    assert(node->is_op_call());
    auto lval = _scope->self();
    auto cls = _scope->eff_self_cls();
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

    auto cls = obj.type()->actual()->as_class();
    auto fset = cls->op(Op::ArrayAccess);
    if (!fset)
        return {ExprError::NoOperator};
    bool is_tmp = obj.value().is_tmp();
    ExprResList args;
    args.push_back(std::move(idx));

    // call operator
    auto funcall = _eval.funcall_helper(_scope);
    ExprRes res =
        funcall->funcall(node, fset, obj.move_value().self(), std::move(args));
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

    auto int_idx = idx.value().copy_rvalue().get<Integer>();
    auto array_type = obj.type()->non_alias()->deref()->non_alias()->as_array();
    assert(array_type);
    auto item_type = array_type->item_type();
    auto array_val = obj.move_value();

    // check bounds
    if (int_idx < 0 || int_idx + 1 > array_type->array_size()) {
        diag().error(node->index(), "array index is out of range");
        return {ExprError::ArrayIndexOutOfRange};
    }
    return {item_type, array_val.array_access(int_idx)};
}

ExprRes EvalExprVisitor::member_access_op(Ref<ast::MemberAccess> node, ExprRes&& obj) {
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
    if (!_eval.resolver()->resolve(var))
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
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(node);
    if (!check_is_assignable(node, to.value()))
        return {ExprError::NotAssignable};
    auto cast = _eval.cast_helper(_scope);
    auto res = cast->cast(node, to.type()->deref(), std::move(tv));
    if (!res)
        return res;
    auto lval = to.move_value().lvalue();
    return {to.type(), lval.assign(res.move_value().move_rvalue())};
}

ExprResList EvalExprVisitor::eval_args(Ref<ast::ArgList> args) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(args);

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

Diag& EvalExprVisitor::diag() { return _program->diag(); }

Builtins& EvalExprVisitor::builtins() { return _program->builtins(); }

std::string_view EvalExprVisitor::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

std::string_view EvalExprVisitor::text(str_id_t str_id) {
    return _program->text_pool().get(str_id);
}

} // namespace ulam::sema
