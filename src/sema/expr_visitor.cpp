#include "libulam/semantic/type/builtin_type_id.hpp"
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type/ops.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>

#define DEBUG_EXPR_VISITOR // TEST
#ifdef DEBUG_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::ExprVisitor] "
#endif
#include "src/debug.hpp"

#define DBG_LINE(node) debug() << _program->sm().line_at(node->loc_id())

namespace ulam::sema {

ExprRes ExprVisitor::visit(Ref<ast::TypeOpExpr> node) {
    debug() << __FUNCTION__ << " TypeOpExpr\n";
    DBG_LINE(node);
    if (node->has_type_name()) {
        Resolver resolver{_program};
        auto type = resolver.resolve_type_name(node->type_name(), _scope, true);
        if (!type)
            return {ExprError::UnresolvableType};
        auto tv = type->actual()->type_op(node->op());
        if (!tv)
            return {ExprError::InvalidTypeOperator};
        return tv;
    }
    assert(node->has_expr());
    auto expr_res = node->expr()->accept(*this);
    if (!expr_res)
        return expr_res;
    assert(expr_res.type());
    auto val = expr_res.move_value();
    auto tv = expr_res.type()->actual()->type_op(node->op(), val);
    if (!tv)
        return {ExprError::InvalidTypeOperator};
    return tv;
}

ExprRes ExprVisitor::visit(Ref<ast::Ident> node) {
    debug() << __FUNCTION__ << " Ident `" << str(node->name().str_id())
            << "`\n";
    DBG_LINE(node);

    // self
    if (node->is_self()) {
        auto self = _scope->self();
        return {_scope->eff_self_cls(), Value{self}};
    }

    // super
    if (node->is_super()) {
        auto self_cls = _scope->self_cls();
        auto sup = class_super(node, self_cls);
        if (!sup)
            return {ExprError::NoSuper};
        auto self = _scope->self();
        return {sup, Value{self.as(sup)}};
    }

    auto name_id = node->name().str_id();
    auto sym =
        node->is_local() ? _scope->get_local(name_id) : _scope->get(name_id);
    if (!sym) {
        diag().error(node, "symbol not found");
        return {ExprError::SymbolNotFound};
    }

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
            Resolver resolver{_program};
            if (!resolver.resolve(var, _scope))
                return {ExprError::UnresolvableVar};
            return {var->type(), Value{var->lvalue()}};
        },
        [&](Ref<Prop> prop) -> ExprRes {
            return {prop->type(), Value{_scope->self().prop(prop)}};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            return {
                builtins().type(FunId), Value{_scope->self().bound_fset(fset)}};
        },
        [&](auto value) -> ExprRes { assert(false); });
}

ExprRes ExprVisitor::visit(Ref<ast::ParenExpr> node) {
    debug() << __FUNCTION__ << " ParenExpr\n";
    DBG_LINE(node);
    return node->inner()->accept(*this);
}

ExprRes ExprVisitor::visit(Ref<ast::BinaryOp> node) {
    debug() << __FUNCTION__ << " BinaryOp\n";
    DBG_LINE(node);
    assert(node->has_lhs() && node->has_rhs());

    // TODO: special case for short-circuiting
    auto left = node->lhs()->accept(*this);
    auto right = node->rhs()->accept(*this);
    if (!left || !right)
        return {ExprError::Error};

    debug() << "left is " << (left.value().is_consteval() ? "" : "not ")
            << "consteval\n";
    debug() << "right is " << (right.value().is_consteval() ? "" : "not ")
            << "consteval\n";

    Op op = node->op();
    LValue lval;
    if (ops::is_assign(op)) {
        if (!check_is_assignable(node, left.value()))
            return {ExprError::NotAssignable};
        lval = left.value().lvalue();
    }

    auto type_errors =
        binary_op_type_check(op, left.type(), right.typed_value());
    auto recast = [&](Ref<ast::Expr> expr, TypeError error,
                      TypedValue&& tv) -> TypedValue {
        switch (error.status) {
        case TypeError::Incompatible:
            diag().error(expr, "incompatible type");
            return {};
        case TypeError::ExplCastRequired: {
            auto rval = tv.move_value().move_rvalue();
            auto message =
                std::string{"suggest casting "} + tv.type()->name() + " to ";
            message +=
                (error.cast_bi_type_id != NoBuiltinTypeId)
                    ? std::string{builtin_type_str(error.cast_bi_type_id)}
                    : error.cast_type->name();
            diag().error(expr, message);
            return {};
        }
        case TypeError::ImplCastRequired: {
            if (error.cast_bi_type_id != NoBuiltinTypeId)
                return do_cast(expr, error.cast_bi_type_id, std::move(tv));
            assert(error.cast_type);
            return {
                error.cast_type, do_cast(expr, error.cast_type, std::move(tv))};
        }
        case TypeError::Ok:
            return std::move(tv);
        default:
            assert(false);
        };
    };
    auto l_tv = recast(node->lhs(), type_errors.first, left.move_typed_value());
    auto r_tv =
        recast(node->rhs(), type_errors.second, right.move_typed_value());
    if (!l_tv || !r_tv)
        return {ExprError::InvalidOperandType};

    if (op != Op::Assign) {
        if (l_tv.type()->actual()->is_prim()) {
            assert(r_tv.type()->actual()->is_prim());
            auto l_type = l_tv.type()->actual()->as_prim();
            auto r_type = r_tv.type()->actual()->as_prim();
            auto l_rval = l_tv.move_value().move_rvalue();
            auto r_rval = r_tv.move_value().move_rvalue();
            r_tv = l_type->binary_op(
                op, std::move(l_rval), r_type, std::move(r_rval));
        } else {
            // TODO: class operators
            assert(false);
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

ExprRes ExprVisitor::visit(Ref<ast::UnaryOp> node) {
    debug() << __FUNCTION__ << " UnaryOp\n";
    DBG_LINE(node);
    auto res = node->arg()->accept(*this);
    if (!res)
        return res;

    Op op = node->op();
    auto tv = res.move_typed_value();

    LValue lval;
    RValue orig_rval;
    if (ops::is_inc_dec(op)) {
        // store lvalue
        if (!check_is_assignable(node, tv.value()))
            return {ExprError::NotAssignable};
        lval = tv.value().lvalue();

        // store original rvalue
        if (ops::is_unary_post_op(op))
            orig_rval = tv.value().copy_rvalue();
    }

    auto error = unary_op_type_check(node->op(), tv.type());
    switch (error.status) {
    case TypeError::Incompatible:
        diag().error(node, "incompatible");
        return {ExprError::InvalidOperandType};
    case TypeError::ExplCastRequired:
        if (ops::is_inc_dec(op)) {
            // ??
            diag().error(node, "incompatible");
            return {ExprError::InvalidOperandType};
        }
        return {ExprError::CastRequired};
    case TypeError::ImplCastRequired:
        assert(!ops::is_inc_dec(op));
        tv = do_cast(node, error.cast_bi_type_id, std::move(tv));
        break;
    case TypeError::Ok:
        break;
    }

    auto arg_type = tv.type()->deref();
    if (arg_type->is_prim()) {
        tv = arg_type->as_prim()->unary_op(op, tv.move_value().move_rvalue());
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
            Resolver resolver{_program};
            auto type = resolver.resolve_type_name(node->type_name(), _scope);
            if (!type)
                return {ExprError::UnresolvableType};
            if (!check_is_object(node, type))
                return {ExprError::NotObject};
            assert(op == Op::Is);
            assert(!tv.value().empty());
            auto dyn_type = tv.value().dyn_obj_type();
            bool is =
                dyn_type->is_same(type) || dyn_type->is_impl_castable_to(type);
            auto boolean = builtins().boolean();
            return {boolean, Value{boolean->construct(is)}};

        } else if (arg_type->is_class()) {
            Resolver resolver{_program};
            auto cls = arg_type->as_class();
            auto fset = cls->resolved_op(op, resolver);
            assert(fset);
            return funcall(node, fset, tv.value().self(), {});
        }
    }
    assert(false);
}

ExprRes ExprVisitor::visit(Ref<ast::Cast> node) {
    debug() << __FUNCTION__ << " Cast\n";
    DBG_LINE(node);
    // eval expr
    auto res = node->expr()->accept(*this);
    if (!res)
        return res;

    // resolve target type
    Resolver resolver{_program};
    auto cast_type =
        resolver.resolve_full_type_name(node->full_type_name(), _scope);
    if (!cast_type)
        return {ExprError::InvalidCast};

    // (Void) expr
    if (cast_type->is(VoidId))
        return {builtins().void_type(), Value{RValue{}}};

    auto cast_res = maybe_cast(node, cast_type, res.move_typed_value(), true);
    if (cast_res.second == CastError)
        return {ExprError::InvalidCast};
    return {cast_type, std::move(cast_res.first)};
}

ExprRes ExprVisitor::visit(Ref<ast::BoolLit> node) {
    debug() << __FUNCTION__ << " BoolLit\n";
    DBG_LINE(node);
    // Bool(1)
    auto type = builtins().boolean();
    auto rval = type->construct(node->value());
    rval.set_is_consteval(true);
    return {type, Value{std::move(rval)}};
}

ExprRes ExprVisitor::visit(Ref<ast::NumLit> node) {
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

ExprRes ExprVisitor::visit(Ref<ast::StrLit> node) {
    debug() << __FUNCTION__ << " StrLit\n";
    DBG_LINE(node);
    auto type = builtins().type(StringId);
    return {type, Value{RValue{node->value(), true}}};
}

ExprRes ExprVisitor::visit(Ref<ast::FunCall> node) {
    debug() << __FUNCTION__ << " FunCall\n";
    DBG_LINE(node);
    auto callable = node->callable()->accept(*this);
    if (!callable)
        return callable;

    // get fun set
    auto val = callable.move_value();
    assert(val.is_lvalue());
    auto lval = val.lvalue();
    if (!lval.is<BoundFunSet>()) {
        diag().error(node->callable(), "is not a function");
        return {ExprError::NotFunction};
    }

    // eval args
    auto [args, success] = eval_args(node->args());
    if (!success)
        return {ExprError::ArgsEvalError};

    auto fset = lval.get<BoundFunSet>().fset();
    return funcall(node->callable(), fset, lval.self(), std::move(args));
}

ExprRes ExprVisitor::visit(Ref<ast::MemberAccess> node) {
    debug() << __FUNCTION__ << " MemberAccess\n";
    DBG_LINE(node);
    assert(node->has_obj());

    // eval object expr
    auto obj_res = node->obj()->accept(*this);
    if (!obj_res)
        return {ExprError::Error};

    // is an object?
    if (!check_is_class(node, obj_res.type(), true))
        return {ExprError::NotClass};

    auto cls = obj_res.type()->actual()->as_class();
    auto obj_val = obj_res.move_value();

    // foo.Base.bar?
    if (node->has_base()) {
        cls = class_base(node, cls, node->base());
        if (!cls) {
            auto error = node->base()->is_super() ? ExprError::NoSuper
                                                  : ExprError::BaseNotFound;
            return {error};
        }
        obj_val = Value{obj_val.as(cls)};
    }

    // get symbol
    assert(cls->is_ready());
    auto name = node->ident()->name();
    auto sym = cls->get(name.str_id());
    if (!sym) {
        diag().error(node->ident(), "member not found");
        return {ExprError::MemberNotFound};
    }

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
            Resolver resolver{_program};
            if (!resolver.resolve(var))
                return {ExprError::UnresolvableVar};
            return {var->type(), Value{var->lvalue()}};
        },
        [&](Ref<Prop> prop) -> ExprRes {
            return {prop->type(), obj_val.prop(prop)};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            return {builtins().type(FunId), obj_val.bound_fset(fset)};
        },
        [&](auto other) -> ExprRes { assert(false); });
}

ExprRes ExprVisitor::visit(Ref<ast::ClassConstAccess> node) {
    debug() << __FUNCTION__ << "ClassConstAccess";
    DBG_LINE(node);
    Resolver resolver{_program};
    auto type = resolver.resolve_class_name(node->type_name(), _scope, false);
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
    // TODO: move resolution to class
    auto var = sym->get<Var>();
    auto scope = cls->scope();
    auto scope_view = scope->view(var->scope_version());
    if (!resolver.resolve(var, ref(scope_view)))
        return {ExprError::UnresolvableClassConst};
    return {var->type(), Value{var->rvalue()}};
}

ExprRes ExprVisitor::visit(Ref<ast::ArrayAccess> node) {
    debug() << __FUNCTION__ << " ArrayAccess\n";
    DBG_LINE(node);
    assert(node->has_array());
    assert(node->has_index());

    // eval array expr
    auto array_res = node->array()->accept(*this);
    if (!array_res)
        return {ExprError::Error};

    // index
    auto index_res = node->index()->accept(*this);
    if (!index_res)
        return {ExprError::UnknownArrayIndex};
    // cast to default Int
    auto int_type = builtins().int_type();
    auto idx_cast_res =
        maybe_cast(node->index(), int_type, index_res.move_typed_value());
    if (idx_cast_res.second == CastError)
        return {ExprError::InvalidCast};
    auto index_val = std::move(idx_cast_res.first);

    // class?
    if (array_res.type()->actual()->is_class()) {
        auto cls = array_res.type()->actual()->as_class();
        auto fset = cls->op(Op::ArrayAccess);
        if (!fset)
            return {ExprError::NoOperator};
        bool is_tmp = array_res.value().is_tmp();
        TypedValueList args;
        args.emplace_back(int_type, std::move(index_val));
        ExprRes res =
            funcall(node, fset, array_res.move_value().self(), std::move(args));
        if (is_tmp && res.value().is_lvalue()) {
            LValue lval = res.move_value().lvalue();
            lval.set_is_xvalue(true);
            return {res.type(), Value{std::move(lval)}};
        }
        return res;
    }

    auto index = index_val.move_rvalue().get<Integer>();

    // string?
    if (array_res.type()->actual()->is(StringId)) {
        auto type = builtins().string_type();
        auto len = type->len(array_res.value());
        if (index < 0 || index + 1 > len) {
            diag().error(node->index(), "char index is out of range");
            return {ExprError::CharIndexOutOfRange};
        }
        auto chr = type->chr(array_res.value(), index);
        bool is_consteval = array_res.value().is_consteval();
        return {
            builtins().unsigned_type(8),
            Value{RValue{(Unsigned)chr, is_consteval}}};
    }

    // array?
    if (!array_res.type()->actual()->is_array()) {
        diag().error(node->array(), "not an array");
        return {ExprError::NotArray};
    }

    // array
    auto array_type =
        array_res.type()->non_alias()->deref()->non_alias()->as_array();
    assert(array_type);
    auto item_type = array_type->item_type();
    auto array_val = array_res.move_value();

    // check bounds
    if (index < 0 || index + 1 > array_type->array_size()) {
        diag().error(node->index(), "array index is out of range");
        return {ExprError::ArrayIndexOutOfRange};
    }
    return {item_type, array_val.array_access(index)};
}

bool ExprVisitor::check_is_assignable(Ref<ast::Expr> node, const Value& value) {
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

bool ExprVisitor::check_is_object(
    Ref<ast::Expr> node, Ref<const Type> type, bool deref) {
    type = deref ? type->actual() : type->canon();
    if (!type->is_object()) {
        diag().error(node, "not a class or Atom");
        return false;
    }
    return true;
}

bool ExprVisitor::check_is_class(
    Ref<ast::Expr> node, Ref<const Type> type, bool deref) {
    type = deref ? type->actual() : type->canon();
    if (!type->is_class()) {
        diag().error(node, "not a class");
        return false;
    }
    return true;
}

Ref<Class> ExprVisitor::class_super(Ref<ast::Expr> node, Ref<Class> cls) {
    if (!cls->has_super()) {
        diag().error(node, "class does not have a superclass");
        return {};
    }
    return cls->super();
}

Ref<Class> ExprVisitor::class_base(
    Ref<ast::Expr> node, Ref<Class> cls, Ref<ast::TypeIdent> ident) {
    if (ident->is_super())
        return class_super(node, cls);
    assert(!ident->is_self());
    auto base = cls->base(ident->name_id());
    if (!base)
        diag().error(node, "class does not have a base with this name");
    return base;
}

ExprRes ExprVisitor::cast(
    Ref<ast::Expr> node, Ref<Type> type, ExprRes&& res, bool expl) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(node);
    auto cast_res = maybe_cast(node, type, res.move_typed_value(), expl);
    if (cast_res.second == CastError)
        return {ExprError::InvalidCast};
    return {type, std::move(cast_res.first)};
}

bitsize_t
ExprVisitor::bitsize_for(Ref<ast::Expr> expr, BuiltinTypeId bi_type_id) {
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
    auto cast_res = maybe_cast(expr, uns_type, res.move_typed_value(), true);
    if (cast_res.second == CastError)
        return NoBitsize;
    auto rval = cast_res.first.move_rvalue();
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

array_size_t ExprVisitor::array_size(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(expr);
    ExprRes res = expr->accept(*this);
    if (!res)
        return UnknownArraySize;

    // cast to default Int
    auto int_type = builtins().int_type();
    auto cast_res = maybe_cast(expr, int_type, res.move_typed_value());
    if (cast_res.second == CastError)
        return UnknownArraySize;

    auto rval = cast_res.first.move_rvalue();
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

std::pair<TypedValueList, bool> ExprVisitor::eval_args(Ref<ast::ArgList> args) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(args);
    std::pair<TypedValueList, bool> res;
    res.second = true;
    for (unsigned n = 0; n < args->child_num(); ++n) {
        // TODO: default arguments
        ExprRes arg_res = args->get(n)->accept(*this);
        if (!arg_res) {
            res.second = false;
            break;
        }
        res.first.push_back(arg_res.move_typed_value());
    }
    return res;
}

std::pair<TypedValueList, bool>
ExprVisitor::eval_tpl_args(Ref<ast::ArgList> args, Ref<ClassTpl> tpl) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(args);

    // tmp param eval scope
    auto param_scope_view = tpl->param_scope()->view(0);
    auto scope = make<BasicScope>(ref(param_scope_view));
    std::list<Ref<Var>> cls_params;

    std::pair<TypedValueList, bool> res;
    res.second = false;
    unsigned n = 0;
    for (auto param : tpl->params()) {
        // param type
        Resolver resolver{_program};
        auto type = resolver.resolve_type_name(param->type_node(), ref(scope));
        if (!type)
            return res;

        // value
        Value val{RValue{}};
        if (n < args->child_num()) {
            // argument provided
            auto arg = args->get(n);
            auto arg_res = arg->accept(*this);
            if (arg_res)
                arg_res = cast(arg, type, std::move(arg_res), false);
            if (!arg_res)
                return res;
            val = arg_res.move_value();

        } else if (param->node()->has_init()) {
            // default argument
            ExprVisitor ev{_program, ref(scope)};
            bool ok = false;
            std::tie(val, ok) = ev.eval_init(param->node(), type);
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

std::pair<Value, bool>
ExprVisitor::eval_init(Ref<ast::VarDecl> node, Ref<Type> type) {
    std::pair<Value, bool> res{RValue{}, false};
    if (node->has_init_value()) {
        // single expr
        ExprRes init_res = node->init_value()->accept(*this);
        if (init_res)
            init_res =
                cast(node->init_value(), type, std::move(init_res), false);
        if (init_res)
            res = {init_res.move_value(), true};

    } else if (node->has_init_list()) {
        // init list
        auto [val, ok] = eval_init_list(type, node->init_list());
        if (ok)
            res = {std::move(val), ok};
    }
    return res;
}

std::pair<Value, bool>
ExprVisitor::eval_init_list(Ref<Type> type, Ref<ast::InitList> list) {
    if (!type->is_array()) {
        // TODO: class constructors
        diag().error(list, "type is not an array");
        return {Value{RValue{}}, false};
    }

    auto fill = [&](auto& self, LValue cur,
                    Ref<ast::InitList> list) -> std::pair<bool, bool> {
        auto array_type = cur.type()->as_array();
        auto array_size = array_type->array_size();
        auto item_type = array_type->item_type();
        assert(array_type->array_size() >= list->child_num());
        assert(list->child_num() > 0);

        bool all_consteval = true;
        if (list->is_flat()) {
            unsigned last_expr_idx = list->child_num() - 1;
            RValue last_rval;
            for (unsigned n = 0; n < array_size; ++n) {
                bool is_last = (n + 1 == array_size);
                RValue rval;
                if (n <= last_expr_idx) {
                    auto expr = list->expr(n);
                    ExprRes res = expr->accept(*this);
                    if (res)
                        res = cast(expr, item_type, std::move(res), false);
                    if (!res)
                        return {false, false};
                    rval = res.move_value().move_rvalue();
                    if (n == last_expr_idx && !is_last)
                        last_rval = rval.copy();
                } else {
                    rval = is_last ? std::move(last_rval) : last_rval.copy();
                }
                all_consteval = all_consteval && rval.is_consteval();
                cur.array_access(n).assign(std::move(rval));
            }
        } else {
            bool all_consteval = true;
            for (unsigned n = 0; n < list->child_num(); ++n) {
                auto [ok, is_consteval] =
                    self(self, cur.array_access(n), list->sublist(n));
                if (!ok)
                    return {false, false};
                all_consteval = all_consteval && is_consteval;
            }
        }
        return {true, all_consteval};
    };

    Value val{type->actual()->construct()};
    auto [ok, is_consteval] = fill(fill, LValue{val.data_view()}, list);
    if (ok && is_consteval)
        val.rvalue().set_is_consteval(is_consteval);
    return {std::move(val), ok};
}

ExprRes
ExprVisitor::assign(Ref<ast::OpExpr> node, TypedValue&& to, TypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    DBG_LINE(node);
    if (!check_is_assignable(node, to.value()))
        return {ExprError::NotAssignable};
    auto cast_res = maybe_cast(node, to.type()->deref(), std::move(tv));
    if (cast_res.second == CastError)
        return {ExprError::InvalidCast};
    auto lval = to.move_value().lvalue();
    return {to.type(), lval.assign(cast_res.first.move_rvalue())};
}

ExprVisitor::CastRes ExprVisitor::maybe_cast(
    Ref<ast::Expr> node, Ref<Type> to, TypedValue&& tv, bool expl) {
    auto from = tv.type();
    auto val = tv.move_value();

    if (to->is_ref()) {
        // taking or casting a reference
        if (val.is_rvalue()) {
            diag().error(node, "cannot take a reference of rvalue");
            return {Value{LValue{}}, CastError};
        }
        if (val.lvalue().is_xvalue()) {
            diag().error(node, "cannot take a reference to xvalue");
            return {Value{LValue{}}, CastError};
        }
        if (from->deref()->is_class())
            from = val.dyn_obj_type()->ref_type();
        from = from->ref_type();

    } else if (from->is_ref()) {
        // copy value from reference
        assert(val.is_lvalue());
        val = val.deref();
        from = from->deref();
        if (from->is_class())
            from = val.dyn_obj_type();
    }

    if (to->is_same(from))
        return {std::move(val), NoCast};

    // implicit cast to base class
    if (!expl && to->is_ref() && to->actual()->is_class() && from->actual()->is_class()) {
        auto to_cls = to->actual()->as_class();
        auto from_cls = from->actual()->as_class();
        if (to_cls->is_base_of(from_cls))
            return {std::move(val), NoCast};
    }

    if (!expl && from->is_impl_castable_to(to, val)) {
        val = do_cast(node, to, {from, std::move(val)});
        return {std::move(val), CastOk};
    }

    if (from->is_expl_castable_to(to)) {
        if (expl) {
            val = do_cast(node, to, {from, std::move(val)});
            return {std::move(val), CastOk};
        }
        diag().error(node, "suggest explicit cast");
    }
    diag().error(node, "invalid cast");
    return {Value{RValue{}}, CastError};
}

Value ExprVisitor::do_cast(
    Ref<ast::Expr> node, Ref<const Type> to, TypedValue&& tv) {
    auto from = tv.type();
    auto val = tv.move_value();

    if (to->is_ref()) {
        // taking or casting a reference
        assert(!val.is_tmp());
        from = from->ref_type();

    } else if (from->is_ref()) {
        // copy value from reference
        assert(val.is_lvalue());
        from = from->deref();
        val = val.deref();
    }

    assert(!to->is_same(from));
    assert(from->is_expl_castable_to(to));

    if (from->is_prim()) {
        assert(to->is_prim());
        return from->as_prim()->cast_to(to, std::move(val));

    } else if (from->is_class()) {
        auto cls = from->as_class();
        auto convs = cls->convs(to, true);
        if (convs.size() == 0) {
            assert(to->is_object());
            assert(cls->is_castable_to_object_type(to, true));
            return cls->cast_to_object_type(to, std::move(val));

        } else {
            ExprRes res = funcall(node, *convs.begin(), val.self(), {});
            if (!res) {
                diag().error(node, "conversion failed");
                return Value{}; // ??
            }
            if (!res.type()->is_same(to)) {
                assert(res.type()->is_expl_castable_to(to));
                return do_cast(node, to, res.move_typed_value());
            }
            return res.move_value();
        }
    } else if (from->is(AtomId)) {
        return from->cast_to(to, std::move(val));

    } else if (from->is_ref()) {
        assert(to->is_ref());
        return from->cast_to(to, std::move(val));
    }

    assert(false);
}

TypedValue ExprVisitor::do_cast(
    Ref<ast::Expr> node, BuiltinTypeId bi_type_id, TypedValue&& tv) {
    auto type = tv.type()->actual();

    if (type->is_class()) {
        // class conversion
        auto cls = type->as_class();
        auto convs = cls->convs(bi_type_id, true);
        assert(convs.size() == 1);
        auto self = tv.move_value().self();
        ExprRes res = funcall(node, *convs.begin(), self, {});
        if (!res) {
            diag().error(node, "conversion failed");
            return {}; // ??
        }
        assert(res.type()->is_prim());
        if (!res.type()->is(bi_type_id))
            return do_cast(node, bi_type_id, res.move_typed_value());
        return {res.type()->as_prim(), res.move_value()};
    }

    // primitive to builtin
    auto val = tv.move_value().deref();
    if (type->is_prim())
        return type->as_prim()->cast_to(bi_type_id, std::move(val));

    assert(false);
}

ExprRes ExprVisitor::funcall(
    Ref<ast::Expr> node, Ref<FunSet> fset, LValue self, TypedValueList&& args) {
    auto dyn_cls = self.dyn_cls();
    auto match_res = fset->find_match(dyn_cls, args);
    if (match_res.empty()) {
        diag().error(node, "no matching functions found");
        return {ExprError::NoMatchingFunction};
    } else if (match_res.size() > 1) {
        diag().error(node, "ambiguous function call");
        return {ExprError::AmbiguousFunctionCall};
    }

    TypedValueList call_args{};
    auto fun = *match_res.begin();
    for (auto& param : fun->params()) {
        assert(!args.empty());

        auto arg = std::move(args.front());
        args.pop_front();

        auto param_type = param->type();

        // binding rvalue or xvalue lvalue to const ref
        if (param_type->is_ref() && arg.value().is_tmp()) {
            assert(param->is_const());
            param_type = param_type->deref(); // cast to non-ref type if casting
        }

        auto cast_res = maybe_cast(node, param_type, std::move(arg), false);
        assert(cast_res.second != CastError);
        call_args.emplace_back(param_type, std::move(cast_res.first));
    }
    return funcall(node, fun, self, std::move(call_args));
}

ExprRes ExprVisitor::funcall(
    Ref<ast::Expr> node, Ref<Fun> fun, LValue self, TypedValueList&& args) {
    debug() << __FUNCTION__ << " " << str(fun->name_id()) << "\n";
    return {fun->ret_type(), Value{}};
}

Diag& ExprVisitor::diag() { return _program->diag(); }

Builtins& ExprVisitor::builtins() { return _program->builtins(); }

std::string_view ExprVisitor::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema
