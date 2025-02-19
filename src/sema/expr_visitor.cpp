#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/bound.hpp>
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
        return type->type_op(node->op());
    }
    assert(node->has_expr());
    auto expr_res = node->expr()->accept(*this);
    return expr_res.type()->type_op(node->op());
}

ExprRes ExprVisitor::visit(Ref<ast::Ident> node) {
    debug() << __FUNCTION__ << " Ident `" << str(node->name().str_id())
            << "`\n";
    auto name = node->name();
    auto sym = _scope->get(name.str_id());
    if (!sym) {
        diag().emit(
            Diag::Error, node->loc_id(), str(name.str_id()).size(),
            "symbol not found");
        return {ExprError::SymbolNotFound};
    }

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
            return {var->type(), Value{LValue{var}}};
        },
        [&](Ref<Prop> prop) -> ExprRes {
            return {
                prop->type(), Value{LValue{BoundProp{_scope->self(), prop}}}};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            return {
                builtins().type(FunId),
                Value{LValue{BoundFunSet{_scope->self(), fset}}}};
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

    auto left = node->lhs()->accept(*this);
    auto right = node->rhs()->accept(*this);
    if (!left || !right)
        return {ExprError::Error};

    if (left.type()->canon()->is_prim()) {
        // binary op on primitive types
        if (!right.type()->canon()->is_prim()) {
            diag().emit(
                Diag::Error, node->rhs()->loc_id(), 1,
                "non-primitive type cannot be used as right operand type");
            return {ExprError::InvalidOperandType};
        }
        return prim_binary_op(
            node, {left.type()->canon()->as_prim(), left.move_value()},
            {right.type()->canon()->as_prim(), right.move_value()});

    } else if (left.type()->canon()->is_array()) {
        return array_binary_op(
            node, left.move_typed_value(), right.move_typed_value());

    } else {
        // TODO: operator funs for classes, error(?) for arrays, ...
        assert(false);
    }
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::UnaryOp> node) {
    debug() << __FUNCTION__ << " UnaryOp\n";
    auto res = node->arg()->accept(*this);
    if (!res.ok())
        return res;

    if (res.type()->canon()->is_prim()) {
        return prim_unary_op(
            node, {res.type()->canon()->as_prim(), res.move_value()});

    } else {
        assert(false); // not implemented
    }
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

    auto type = res.type();
    auto cast_res = maybe_cast(node, type, res.move_typed_value());
    if (cast_res.second == CastError)
        return {ExprError::InvalidCast};
    return {cast_type, Value{std::move(cast_res.first)}};
}

ExprRes ExprVisitor::visit(Ref<ast::BoolLit> node) {
    debug() << __FUNCTION__ << " BoolLit\n";
    // Bool(1)
    auto type = builtins().prim_type_tpl(BoolId)->type(diag(), node, 1);
    assert(type);
    return {type, Value{RValue{(Unsigned)node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::NumLit> node) {
    debug() << __FUNCTION__ << " NumLit\n";
    const auto& number = node->value();
    if (number.is_signed()) {
        // Int(n)
        auto tpl = builtins().prim_type_tpl(IntId);
        auto type = tpl->type(diag(), node, number.bitsize());
        assert(type);
        return {type, Value{RValue{number.value<Integer>()}}};
    } else {
        // Unsigned(n)
        auto tpl = builtins().prim_type_tpl(UnsignedId);
        auto type = tpl->type(diag(), node, number.bitsize());
        assert(type);
        return {type, Value{RValue{number.value<Unsigned>()}}};
    }
}

ExprRes ExprVisitor::visit(Ref<ast::StrLit> node) {
    debug() << __FUNCTION__ << " StrLit\n";
    auto type = builtins().prim_type(StringId);
    return {type, Value{RValue{node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::FunCall> node) {
    debug() << __FUNCTION__ << " FunCall\n";
    auto loc_id = node->callable()->loc_id();
    auto callable = node->callable()->accept(*this);
    if (!callable)
        return {};

    // get fun set
    auto val = callable.move_value();
    assert(val.is_lvalue());
    auto lval = val.lvalue();
    if (!lval.is<BoundFunSet>()) {
        diag().emit(Diag::Error, loc_id, 1, "is not a function");
        return {};
    }
    auto& bound_fset = lval.get<BoundFunSet>();
    auto obj_view = bound_fset.obj_view();
    auto fset = bound_fset.mem();

    // eval args
    auto [arg_list, success] = eval_args(node->args());
    if (!success)
        return {};

    // find match
    auto match_res = fset->find_match(arg_list);
    if (match_res.size() == 1) {
        // success, one match found
        return funcall(
            node, *(match_res.begin()), obj_view, std::move(arg_list));
    } else if (match_res.size() == 0) {
        diag().emit(Diag::Error, loc_id, 1, "no matching function found");
    } else {
        diag().emit(Diag::Error, loc_id, 1, "ambiguous funcall");
    }

    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::MemberAccess> node) {
    debug() << __FUNCTION__ << " MemberAccess\n";
    assert(node->has_obj());

    // eval object expr
    auto obj_res = node->obj()->accept(*this);
    if (!obj_res.ok())
        return {ExprError::Error};

    // is an object?
    if (!obj_res.type()->canon()->is_class()) {
        diag().emit(Diag::Error, node->obj()->loc_id(), 1, "not an object");
        return {ExprError::NotObject};
    }

    auto cls = obj_res.type()->as_class();
    auto obj_val = obj_res.move_value();

    // get symbol
    auto name = node->ident()->name();
    auto sym = cls->get(name.str_id());
    if (!sym) {
        diag().emit(
            Diag::Error, node->ident()->loc_id(), 1, "member not found");
        return {ExprError::MemberNotFound};
    }

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
            return {var->type(), Value{LValue{var}}};
        },
        [&](Ref<Prop> prop) -> ExprRes {
            return {prop->type(), obj_val.bound_prop(prop)};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            auto obj_cls = obj_val.obj_cls();
            if (fset->is_virtual() && obj_cls != cls) {
                auto sym = obj_cls->get(name.str_id());
                if (sym->is<FunSet>())
                    fset = sym->get<FunSet>();
            }
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

    if (array_res.type()->non_alias()->is_class())
        assert(false); // not implemented

    // is an array?
    if (!array_res.type()->non_alias()->is_array()) {
        diag().emit(Diag::Error, node->array()->loc_id(), 1, "not an array");
        return {ExprError::NotArray};
    }

    // array
    auto array_type = array_res.type()->non_alias()->as_array();
    auto item_type = array_type->item_type();
    auto array_val = array_res.move_value();

    // index
    auto index = array_index(node->index());
    if (index == UnknownArrayIdx)
        return {ExprError::UnknownArrayIndex};
    if (index + 1 > array_type->array_size()) {
        diag().emit(
            Diag::Error, node->index()->loc_id(), 1,
            "array index is out of range");
        return {ExprError::ArrayIndexOutOfRange};
    }
    return {item_type, array_val.array_access(item_type, index)};
}

ExprRes ExprVisitor::cast(
    loc_id_t loc_id,
    std::size_t len,
    ExprRes&& res,
    Ref<Type> type,
    bool is_expl) {
    debug() << __FUNCTION__ << "\n";
    if (res.type()->is_prim()) {
        if (!type->is_prim()) {
            diag().emit(
                Diag::Error, loc_id, len, "cannot cast to non-primitive type");
            return {ExprError::InvalidCast};
        }
        if (!res.type()->as_prim()->is_castable_to(
                type->builtin_type_id(), is_expl)) {
            diag().emit(
                Diag::Error, loc_id, len,
                std::string{"cannot cast to "} +
                    std::string{builtin_type_str(type->builtin_type_id())});
            return {ExprError::InvalidCast};
        }
        return {ExprError::NotImplemented};
    } else {
        return {ExprError::NotImplemented};
    }
}

array_idx_t ExprVisitor::array_index(Ref<ast::Expr> expr) {
    debug() << __FUNCTION__ << "\n";
    ExprRes res = expr->accept(*this);
    if (!res.ok())
        return UnknownArrayIdx;

    // TODO: clean up

    auto type = res.type();
    auto rval = res.move_value().move_rvalue();
    switch (type->builtin_type_id()) {
    case IntId: {
        auto int_val = rval.get<Integer>();
        if (int_val < 0) {
            diag().emit(Diag::Error, expr->loc_id(), 1, "array index is < 0");
            return UnknownArrayIdx;
        }
        return (array_idx_t)int_val;
    }
    case UnsignedId: {
        auto uns_val = rval.get<Unsigned>();
        return (array_idx_t)uns_val;
    }
    case UnaryId: {
        auto uns_val = detail::count_ones(rval.get<Unsigned>());
        return (array_idx_t)uns_val;
    }
    default:
        diag().emit(
            Diag::Error, expr->loc_id(), 1, "array index is non-numeric");
        return UnknownArrayIdx;
    }
}

ExprRes
ExprVisitor::prim_unary_op(Ref<ast::UnaryOp> node, PrimTypedValue&& arg) {
    debug() << __FUNCTION__ << "\n";
    Op op = node->op();
    LValue lval;
    RValue orig_rval;
    if (ops::is_inc_dec(op)) {
        // store lvalue
        if (!arg.value().is_lvalue()) {
            diag().emit(Diag::Error, node->loc_id(), 1, "cannot modify rvalue");
            return {ExprError::NotLvalue};
        }
        lval = arg.value().lvalue();

        // store original rvalue
        if (ops::is_unary_post_op(op))
            orig_rval = arg.value().copy_rvalue();
    }

    auto error = prim_unary_op_type_check(op, arg.type());
    switch (error.status) {
    case PrimTypeError::Incompatible:
        diag().emit(Diag::Error, node->loc_id(), 1, "incompatible type");
        return {ExprError::InvalidOperandType};
    case PrimTypeError::ExplCastRequired:
        if (ops::is_inc_dec(op)) {
            // cannot cast for in-place operations
            diag().emit(Diag::Error, node->loc_id(), 1, "non-numeric type");
            return {ExprError::InvalidOperandType};
        } else {
            auto message = std::string{"suggest casting to "} +
                           std::string{builtin_type_str(error.suggested_type)};
            diag().emit(Diag::Error, node->loc_id(), 1, message);
        }
        [[fallthrough]];
    case PrimTypeError::ImplCastRequired:
        assert(!ops::is_inc_dec(op));
        arg = prim_cast(error.suggested_type, std::move(arg));
        break;
    case PrimTypeError::Ok:
        break;
    }

    if (ops::is_inc_dec(op)) {
        assign(node, Value{lval}, {arg.type(), arg.move_value()});
        if (ops::is_unary_pre_op(op))
            return {arg.type(), Value{lval}};
        return {arg.type(), Value{std::move(orig_rval)}};
    }
    return {arg.type(), arg.move_value()};
}

ExprRes ExprVisitor::prim_binary_op(
    Ref<ast::BinaryOp> node, PrimTypedValue&& left, PrimTypedValue&& right) {
    debug() << __FUNCTION__ << " " << ops::str(node->op()) << "\n";
    Op op = ops::non_assign(node->op());
    PrimTypeErrorPair type_errors{};
    if (op != Op::None) {
        // get rvalues
        PrimTypedValue left_tv = {
            left.type(), Value{left.value().move_rvalue()}};
        PrimTypedValue right_tv = {
            right.type(), Value{right.value().move_rvalue()}};

        // check operand types
        type_errors = prim_binary_op_type_check(op, left.type(), right.type());

        // cast if required and possible
        auto recast = [&](PrimTypeError error, PrimTypedValue&& tv,
                          Ref<const ast::Expr> node) -> PrimTypedValue {
            switch (error.status) {
            case PrimTypeError::Incompatible:
                diag().emit(
                    Diag::Error, node->loc_id(), 1, "incompatible type");
                return {};
            case PrimTypeError::ExplCastRequired: {
                auto message =
                    std::string{"suggest casting to "} +
                    std::string{builtin_type_str(error.suggested_type)};
                diag().emit(Diag::Error, node->loc_id(), 1, message);
            }
                [[fallthrough]];
            case PrimTypeError::ImplCastRequired:
                return prim_cast(error.suggested_type, std::move(tv));
            case PrimTypeError::Ok:
                return std::move(tv);
            default:
                assert(false);
            }
        };
        left_tv = recast(type_errors.first, std::move(left_tv), node->lhs());
        right_tv = recast(type_errors.second, std::move(right_tv), node->rhs());
        if (!left_tv || !right_tv)
            return {ExprError::InvalidOperandType};

        // apply op
        right = left_tv.type()->binary_op(
            op, left_tv.move_value().move_rvalue(), right_tv.type(),
            right_tv.move_value().move_rvalue());
    }

    // handle assignment
    if (ops::is_assign(node->op())) {
        if (!left.value().is_lvalue()) {
            diag().emit(
                Diag::Error, node->loc_id(), 1, "cannot assign to rvalue");
            return {ExprError::NotLvalue};
        }
        return assign(
            node, left.move_value(), {right.type(), right.move_value()});
    }
    return {right.type(), right.move_value()};
}

ExprRes ExprVisitor::array_binary_op(
    Ref<ast::BinaryOp> node, TypedValue&& left, TypedValue&& right) {
    debug() << __FUNCTION__ << "\n";
    // assignment only
    if (node->op() != Op::Assign) {
        diag().emit(
            Diag::Error, node->loc_id(), 1, "unsupported array operation");
        return {ExprError::InvalidOperandType};
    }
    if (!right.type()->canon()->is_array()) {
        diag().emit(
            Diag::Error, node->loc_id(), 1, "cannot assign to an array");
        return {ExprError::InvalidOperandType};
    }
    if (right.type()->canon() != left.type()->canon()) {
        diag().emit(Diag::Error, node->loc_id(), 1, "array types do not match");
        return {ExprError::TypeMismatch};
    }
    if (!left.value().is_lvalue()) {
        diag().emit(Diag::Error, node->loc_id(), 1, "cannot assign");
        return {ExprError::NotLvalue};
    }
    return assign(node, left.move_value(), std::move(right));
}

ExprRes
ExprVisitor::assign(Ref<ast::OpExpr> node, Value&& val, TypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    if (!val.is_lvalue()) {
        diag().emit(Diag::Error, node->loc_id(), 1, "cannot assign to rvalue");
        return {ExprError::NotLvalue};
    }
    auto lval = val.lvalue();
    auto [rval, _] = maybe_cast(node, lval.type(), std::move(tv));
    return {lval.type(), lval.assign(std::move(rval))};
}

ExprVisitor::CastRes
ExprVisitor::maybe_cast(Ref<ast::Expr> node, Ref<Type> type, TypedValue&& tv) {
    if (type->canon() == tv.type()->canon())
        return {tv.value().move_rvalue(), NoCast};
    if (tv.type()->is_impl_castable_to(type))
        return {do_cast(type, std::move(tv)), CastOk};
    if (tv.type()->is_expl_castable_to(type)) {
        diag().emit(Diag::Error, node->loc_id(), 1, "suggest explicit cast");
        return {do_cast(type, std::move(tv)), CastOk};
    }
    return {RValue{}, CastError};
}

RValue ExprVisitor::do_cast(Ref<Type> type, TypedValue&& tv) {
    if (type->is_prim()) {
        assert(tv.type()->is_prim());
        return prim_cast(
            type->as_prim(), {tv.type()->as_prim(), tv.move_value()});
    } else {
        assert(false);
    }
}

PrimTypedValue
ExprVisitor::prim_cast(BuiltinTypeId type_id, PrimTypedValue&& tv) {
    debug() << __FUNCTION__ << " (type ID)\n";
    assert(tv.type()->is_expl_castable_to(type_id));
    return tv.type()->cast_to(type_id, tv.value().move_rvalue());
}

RValue ExprVisitor::prim_cast(Ref<PrimType> type, PrimTypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    assert(tv.type()->is_expl_castable_to(type));
    return tv.type()->cast_to(type, tv.value().move_rvalue());
}

ExprRes ExprVisitor::funcall(
    Ref<ast::FunCall> node,
    Ref<Fun> fun,
    ObjectView obj_view,
    TypedValueList&& args) {
    debug() << __FUNCTION__ << " " << str(fun->name_id()) << "\n";
    return {fun->ret_type(), Value{}};
}

std::pair<TypedValueList, bool> ExprVisitor::eval_args(Ref<ast::ArgList> args) {
    debug() << __FUNCTION__ << "\n";
    std::pair<TypedValueList, bool> res;
    res.second = true;
    for (unsigned n = 0; n < args->child_num(); ++n) {
        ExprRes arg_res = args->get(n)->accept(*this);
        res.first.push_back(arg_res.move_typed_value());
        if (!arg_res)
            res.second = false;
    }
    return res;
}

Diag& ExprVisitor::diag() { return _program->diag(); }

Builtins& ExprVisitor::builtins() { return _program->builtins(); }

std::string_view ExprVisitor::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema
