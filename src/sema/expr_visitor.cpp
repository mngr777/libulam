#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
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

namespace ulam::sema {

ExprRes ExprVisitor::visit(Ref<ast::TypeOpExpr> node) {
    debug() << __FUNCTION__ << " TypeOpExpr\n";
    if (node->has_type_name()) {
        Resolver resolver{_program};
        auto type = resolver.resolve_type_name(node->type_name(), _scope);
        if (!type)
            return {ExprError::UnresolvableType};
        auto tv = type->actual()->type_op(node->op());
        if (!tv)
            return {ExprError::InvalidTypeOperator};
        return tv;
    }
    assert(node->has_expr());
    auto expr_res = node->expr()->accept(*this);
    auto tv = expr_res.type()->actual()->type_op(node->op());
    if (!tv)
        return {ExprError::InvalidTypeOperator};
    return tv;
}

ExprRes ExprVisitor::visit(Ref<ast::Ident> node) {
    debug() << __FUNCTION__ << " Ident `" << str(node->name().str_id())
            << "`\n";
    auto name = node->name();
    auto sym = _scope->get(name.str_id());
    if (!sym) {
        diag().error(node, "symbol not found");
        return {ExprError::SymbolNotFound};
    }

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
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
    return node->inner()->accept(*this);
}

ExprRes ExprVisitor::visit(Ref<ast::BinaryOp> node) {
    debug() << __FUNCTION__ << " BinaryOp\n";
    assert(node->has_lhs() && node->has_rhs());

    // TODO: special case for short-circuiting
    auto left = node->lhs()->accept(*this);
    auto right = node->rhs()->accept(*this);
    if (!left || !right)
        return {ExprError::Error};

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
            RValue rval = do_cast(expr, error.cast_type, std::move(tv));
            return {error.cast_type, Value{std::move(rval)}};
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
        return assign(node, Value{lval}, std::move(r_tv));
    }
    return {std::move(r_tv)};
}

ExprRes ExprVisitor::visit(Ref<ast::UnaryOp> node) {
    debug() << __FUNCTION__ << " UnaryOp\n";
    auto res = node->arg()->accept(*this);
    if (!res.ok())
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
                assign(node, Value{lval}, std::move(tv));
            if (ops::is_unary_pre_op(op))
                return {tv.type(), Value{lval}};
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
            auto cls = arg_type->as_class();
            auto fset = cls->op(op);
            assert(fset);
            return funcall(node, fset, tv.value().self(), {});
        }
    }
    assert(false);
}

ExprRes ExprVisitor::visit(Ref<ast::Cast> node) {
    debug() << __FUNCTION__ << " Cast\n";
    // eval expr
    auto res = node->expr()->accept(*this);
    if (!res.ok())
        return res;

    // resolve target type
    Resolver resolver{_program};
    auto cast_type = resolver.resolve_type_name(node->type_name(), _scope);
    if (!cast_type)
        return {ExprError::InvalidCast};

    auto cast_res = maybe_cast(node, cast_type, res.move_typed_value(), true);
    if (cast_res.second == CastError)
        return {ExprError::InvalidCast};
    return {cast_type, Value{std::move(cast_res.first)}};
}

ExprRes ExprVisitor::visit(Ref<ast::BoolLit> node) {
    debug() << __FUNCTION__ << " BoolLit\n";
    // Bool(1)
    auto type = builtins().boolean();
    auto rval = type->construct(node->value());
    rval.set_is_consteval(true);
    return {type, Value{std::move(rval)}};
}

ExprRes ExprVisitor::visit(Ref<ast::NumLit> node) {
    debug() << __FUNCTION__ << " NumLit\n";
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
    auto type = builtins().type(StringId);
    return {type, Value{RValue{node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::FunCall> node) {
    debug() << __FUNCTION__ << " FunCall\n";
    auto callable = node->callable()->accept(*this);
    if (!callable)
        return {};

    // get fun set
    auto val = callable.move_value();
    assert(val.is_lvalue());
    auto lval = val.lvalue();
    if (!lval.is<BoundFunSet>()) {
        diag().error(node->callable(), "is not a function");
        return {};
    }

    // eval args
    auto [args, success] = eval_args(node->args());
    if (!success)
        return {};

    auto fset = lval.get<BoundFunSet>().fset();
    return funcall(node->callable(), fset, lval.self(), std::move(args));
}

ExprRes ExprVisitor::visit(Ref<ast::MemberAccess> node) {
    debug() << __FUNCTION__ << " MemberAccess\n";
    assert(node->has_obj());

    // eval object expr
    auto obj_res = node->obj()->accept(*this);
    if (!obj_res.ok())
        return {ExprError::Error};

    // is an object?
    if (!check_is_class(node, obj_res.type(), true))
        return {ExprError::NotClass};

    auto cls = obj_res.type()->actual()->as_class();
    auto obj_val = obj_res.move_value();

    // get symbol
    auto name = node->ident()->name();
    auto sym = cls->get(name.str_id());
    if (!sym) {
        diag().error(node->ident(), "member not found");
        return {ExprError::MemberNotFound};
    }

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
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

ExprRes ExprVisitor::visit(Ref<ast::ArrayAccess> node) {
    debug() << __FUNCTION__ << " ArrayAccess\n";
    assert(node->has_array());
    assert(node->has_index());

    // eval array expr
    auto array_res = node->array()->accept(*this);
    if (!array_res.ok())
        return {ExprError::Error};

    // index
    auto index = array_index(node->index());
    if (index == UnknownArrayIdx)
        return {ExprError::UnknownArrayIndex};

    // is class?
    if (array_res.type()->actual()->is_class()) {
        auto cls = array_res.type()->actual()->as_class();
        auto fset = cls->op(Op::ArrayAccess);
        if (!fset)
            return {ExprError::NoOperator};
        bool is_tmp = array_res.value().is_rvalue() ||
                      array_res.value().lvalue().is_xvalue();
        TypedValueList args;
        args.emplace_back(builtins().int_type(), Value{RValue{(Integer)index}});
        ExprRes res =
            funcall(node, fset, array_res.move_value().self(), std::move(args));
        if (is_tmp && res.value().is_lvalue()) {
            LValue lval = res.move_value().lvalue();
            lval.set_is_xvalue(true);
            return {res.type(), Value{std::move(lval)}};
        }
        return res;
    }

    // is an array?
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
    if (index + 1 > array_type->array_size()) {
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
    if (!type->is_object()) {
        diag().error(node, "not a class");
        return false;
    }
    return true;
}

ExprRes ExprVisitor::cast(
    Ref<ast::Expr> node, Ref<Type> type, ExprRes&& res, bool expl) {
    debug() << __FUNCTION__ << "\n";
    auto cast_res = maybe_cast(node, type, res.move_typed_value(), expl);
    if (cast_res.second == CastError)
        return {ExprError::InvalidCast};
    return {type, Value{std::move(cast_res.first)}};
}

array_idx_t ExprVisitor::array_index(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n";
    ExprRes res = expr->accept(*this);
    if (!res.ok())
        return UnknownArrayIdx;

    // Cast to default Int
    auto int_type = builtins().prim_type(IntId, IntType::DefaultSize);
    auto cast_res = maybe_cast(expr, int_type, res.move_typed_value());
    if (cast_res.second == CastError)
        return UnknownArrayIdx;

    auto int_val = cast_res.first.get<Integer>();
    if (int_val < 0) {
        diag().error(expr, "array index is < 0");
        return UnknownArrayIdx;
    }
    return (array_idx_t)int_val;
}

ExprRes
ExprVisitor::assign(Ref<ast::OpExpr> node, Value&& val, TypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    if (!check_is_assignable(node, val))
        return {ExprError::NotAssignable};
    auto lval = val.lvalue();
    auto [rval, _] = maybe_cast(node, lval.type(), std::move(tv));
    return {lval.type(), lval.assign(std::move(rval))};
}

ExprVisitor::CastRes ExprVisitor::maybe_cast(
    Ref<ast::Expr> node, Ref<Type> type, TypedValue&& tv, bool expl) {
    auto to = type->actual();
    auto from = tv.type()->actual();

    if (to->is_same(from))
        return {tv.value().move_rvalue(), NoCast};

    if (from->is_castable_to(to, expl))
        return {do_cast(node, to, std::move(tv)), CastOk};

    if (!expl && from->is_expl_castable_to(to)) {
        diag().error(node, "suggest explicit cast");
    }
    return {RValue{}, CastError};
}

RValue ExprVisitor::do_cast(
    Ref<ast::Expr> node, Ref<const Type> type, TypedValue&& tv) {
    auto to = type->actual();
    auto from = tv.type()->actual();

    assert(from->is_expl_castable_to(to));
    if (from->is_prim()) {
        assert(to->is_prim());
        return from->as_prim()->cast_to(to, tv.move_value().move_rvalue());

    } else if (from->is_class()) {
        auto cls = from->as_class();
        auto convs = cls->convs(to, true);
        assert(convs.size() == 1);
        auto obj_val = tv.move_value();
        ExprRes res = funcall(node, *convs.begin(), obj_val.self(), {});
        if (!res.ok()) {
            diag().error(node, "conversion failed");
            return RValue{};
        }
        if (res.type()->is_same(to)) {
            assert(res.type()->is_expl_castable_to(to));
            return do_cast(node, type, res.move_typed_value());
        }
        return res.move_value().move_rvalue();
    }
    assert(false);
}

TypedValue ExprVisitor::do_cast(
    Ref<ast::Expr> node, BuiltinTypeId builtin_type_id, TypedValue&& tv) {
    auto actual = tv.type()->actual();
    assert(actual->is_expl_castable_to(builtin_type_id));
    if (actual->is_prim()) {
        return actual->as_prim()->cast_to(
            builtin_type_id, tv.move_value().move_rvalue());

    } else if (actual->is_class()) {
        auto cls = actual->as_class();
        auto convs = cls->convs(builtin_type_id, true);
        assert(convs.size() == 1);
        auto obj_val = tv.move_value();
        ExprRes res = funcall(node, *convs.begin(), obj_val.self(), {});
        if (!res.ok()) {
            diag().error(node, "conversion failed");
            return {}; // ??
        }
        assert(res.type()->is_prim());
        if (!res.type()->as_prim()->is(builtin_type_id))
            return do_cast(node, builtin_type_id, res.move_typed_value());
        return {res.type()->as_prim(), res.move_value()};
    }
    assert(false);
}

ExprRes ExprVisitor::funcall(
    Ref<ast::Expr> node, Ref<FunSet> fset, LValue self, TypedValueList&& args) {
    auto match_res = fset->find_match(self.dyn_cls(), args);
    if (match_res.size() == 1) {
        return funcall(node, *(match_res.begin()), self, std::move(args));
    } else if (match_res.size() == 0) {
        diag().error(node, "no matching function found");
    } else {
        diag().error(node, "ambiguous funcall");
    }
    return {};
}

ExprRes ExprVisitor::funcall(
    Ref<ast::Expr> node, Ref<Fun> fun, LValue self, TypedValueList&& args) {
    debug() << __FUNCTION__ << " " << str(fun->name_id()) << "\n";
    return {fun->ret_type(), Value{}};
}

std::pair<TypedValueList, bool> ExprVisitor::eval_args(Ref<ast::ArgList> args) {
    debug() << __FUNCTION__ << "\n";
    std::pair<TypedValueList, bool> res;
    res.second = true;
    for (unsigned n = 0; n < args->child_num(); ++n) {
        ExprRes arg_res = args->get(n)->accept(*this);
        if (!arg_res) {
            res.second = false;
            break;
        }
        res.first.push_back(arg_res.move_typed_value());
    }
    return res;
}

Diag& ExprVisitor::diag() { return _program->diag(); }

Builtins& ExprVisitor::builtins() { return _program->builtins(); }

std::string_view ExprVisitor::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema
