#include "libulam/semantic/expr_res.hpp"
#include "libulam/semantic/value/array.hpp"
#include "src/semantic/detail/integer.hpp"
#include <cassert>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
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

// TODO: use canon types for casting

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

    if (left.type()->is_prim()) {
        // binary op on primitive types
        if (!right.type()->is_prim()) {
            diag().emit(
                Diag::Error, node->rhs()->loc_id(), 1,
                "non-primitive type cannot be used as right operand type");
            return {ExprError::InvalidOperandType};
        }
        return prim_binary_op(
            node, {left.type()->as_prim(), left.move_value()},
            {right.type()->as_prim(), right.move_value()});

    } else {
        // TODO: operator funs for classes, error(?) for arrays, ...
        assert(false);
    }
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::UnaryPreOp> node) {
    debug() << __FUNCTION__ << " UnaryPreOp\n";
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::UnaryPostOp> node) {
    debug() << __FUNCTION__ << " UnaryPostOp\n";
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::Cast> node) {
    debug() << __FUNCTION__ << " Cast\n";
    // eval expr
    auto res = node->expr()->accept(*this);
    if (!res.ok())
        return {res.error()};

    // resolve target type
    Resolver resolver{_program};
    auto cast_type = resolver.resolve_type_name(node->type_name(), _scope);
    if (!cast_type)
        return {ExprError::InvalidCast};

    auto type = res.type();
    auto val = res.move_value();

    if (type->is_prim()) {
        // prim type cast
        if (!cast_type->is_prim()) {
            diag().emit(
                Diag::Error, node->loc_id(), 1,
                "cannot cast to non-primitive type");
            return {ExprError::InvalidCast};
        }
        auto prim_type = type->as_prim();
        auto prim_cast_type = cast_type->as_prim();
        if (!prim_type->is_expl_castable_to(prim_cast_type)) {
            diag().emit(Diag::Error, node->loc_id(), 1, "not castable to type");
            return {ExprError::InvalidCast};
        }
        return {
            cast_type,
            Value{prim_type->cast_to(prim_cast_type, val.move_rvalue())}};

    } else {
        // class type cast
        assert(false);
    }
    return {};
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

    auto make_bound_prop = [&](Ref<Prop> prop) {
        return obj_val.accept(
            [&](LValue& lval) {
                return lval.accept(
                    [&](Ref<Var> var) {
                        return BoundProp{var->obj_view(), prop};
                    },
                    [&](ArrayAccess& array_access) {
                        auto obj_view = array_access.item_object_view();
                        return BoundProp{obj_view, prop};
                    },
                    [&](ObjectView obj_view) {
                        return BoundProp{obj_view, prop};
                    },
                    [&](BoundProp& bound_prop) {
                        return bound_prop.mem_obj_bound_prop(prop);
                    },
                    [&](auto other) -> BoundProp { assert(false); });
            },
            [&](RValue& rval) {
                assert(rval.is<SPtr<Object>>());
                return BoundProp{rval.get<SPtr<Object>>(), prop};
            },
            [&](auto other) -> BoundProp { assert(false); });
    };

    auto make_bound_fset = [&](Ref<FunSet> fset) {
        return obj_val.accept(
            [&](LValue& lval) {
                return lval.accept(
                    [&](Ref<Var> var) {
                        return BoundFunSet{var->obj_view(), fset};
                    },
                    [&](ArrayAccess& array_access) {
                        auto obj_view = array_access.item_object_view();
                        return BoundFunSet{obj_view, fset};
                    },
                    [&](ObjectView obj_view) {
                        return BoundFunSet{obj_view, fset};
                    },
                    [&](BoundProp& bound_prop) {
                        return bound_prop.mem_obj_bound_fset(fset);
                    },
                    [&](auto other) -> BoundFunSet { assert(false); });
            },
            [&](RValue& rval) {
                assert(rval.is<SPtr<Object>>());
                return BoundFunSet{rval.get<SPtr<Object>>(), fset};
            },
            [&](auto other) -> BoundFunSet { assert(false); });
    };

    auto get_obj_cls = [&]() {
        return obj_val.accept(
            [&](LValue& lval) {
                return lval.accept(
                    [&](Ref<Var> var) {
                        assert(var->type()->is_class());
                        return var->type()->as_class();
                    },
                    [&](ObjectView obj_view) { return obj_view.cls(); },
                    [&](BoundProp& bound_prop) {
                        auto type = bound_prop.mem()->type();
                        assert(type->is_class());
                        return type->as_class();
                    },
                    [&](auto other) -> Ref<Class> { assert(false); });
            },
            [&](RValue& rval) {
                assert(rval.is<SPtr<Object>>());
                return rval.get<SPtr<Object>>()->cls();
            },
            [&](auto other) -> Ref<Class> { assert(false); });
    };

    return sym->accept(
        [&](Ref<Var> var) -> ExprRes {
            return {var->type(), Value{LValue{var}}};
        },
        [&](Ref<Prop> prop) -> ExprRes {
            return {prop->type(), Value{LValue{make_bound_prop(prop)}}};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            auto obj_cls = get_obj_cls();
            if (fset->is_virtual() && obj_cls != cls) {
                auto sym = obj_cls->get(name.str_id());
                if (sym->is<FunSet>())
                    fset = sym->get<FunSet>();
            }
            return {
                builtins().type(FunId), Value{LValue{make_bound_fset(fset)}}};
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

    if (array_res.type()->canon()->is_class())
        assert(false); // not implemented

    // is an array?
    if (!array_res.type()->canon()->is_array()) {
        diag().emit(Diag::Error, node->array()->loc_id(), 1, "not an array");
        return {ExprError::NotArray};
    }

    // array
    auto array_type = array_res.type()->as_array();
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

ExprRes ExprVisitor::prim_binary_op(
    Ref<ast::BinaryOp> node, PrimTypedValue&& left, PrimTypedValue&& right) {
    debug() << __FUNCTION__ << " " << ops::str(node->op()) << "\n";
    Op op = ops::non_assign(node->op());
    PrimTypeErrorPair type_errors{};
    if (op != Op::None) {
        // get rvalues
        PrimTypedValue left_tv = {
            left.type(), Value{RValue{left.value().move_rvalue()}}};
        PrimTypedValue right_tv = {
            right.type(), Value{RValue{right.value().move_rvalue()}}};

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
                    std::string{"suggest casting to"} +
                    std::string{builtin_type_str(error.suggested_type)};
                diag().emit(Diag::Error, node->loc_id(), 1, message);
            } // fallthru
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

        // apply op
        right =
            prim_binary_op_impl(op, std::move(left_tv), std::move(right_tv));
    }

    // handle assignment
    if (ops::is_assign(node->op())) {
        if (!left.value().is_lvalue()) {
            diag().emit(
                Diag::Error, node->loc_id(), 1, "cannot assign to rvalue");
            return {ExprError::NotLvalue};
        }
        return assign(
            node, left.value().lvalue(), {right.type(), right.move_value()});
    }
    return {right.type(), right.move_value()};
}

ExprRes
ExprVisitor::assign(Ref<ast::BinaryOp> node, LValue& lval, TypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    return lval.accept(
        [&](Ref<Var> var) -> ExprRes {
            var->set_value(Value{tv.move_value().move_rvalue()});
            return {var->type(), Value{LValue{var}}};
        },
        [&](ArrayAccess& array_access) -> ExprRes {
            auto [rval, _] =
                maybe_cast(node, array_access.type(), std::move(tv));
            auto res_rval = rval.copy();
            if (!rval.empty())
                array_access.store(std::move(rval));
            return {array_access.type(), Value{std::move(res_rval)}};
        },
        [&](BoundProp& bound_prop) -> ExprRes {
            auto [rval, _] =
                maybe_cast(node, bound_prop.mem()->type(), std::move(tv));
            if (!rval.empty())
                bound_prop.store(std::move(rval));
            return {bound_prop.mem()->type(), Value{LValue{bound_prop}}};
        },
        [&](auto&& other) -> ExprRes { assert(false); });
}

std::pair<RValue, bool>
ExprVisitor::maybe_cast(Ref<ast::Expr> node, Ref<Type> type, TypedValue&& tv) {
    if (type == tv.type())
        return {tv.value().move_rvalue(), false};
    if (tv.type()->is_impl_castable_to(type))
        return {do_cast(type, std::move(tv)), true};
    if (tv.type()->is_expl_castable_to(type)) {
        diag().emit(Diag::Error, node->loc_id(), 1, "suggest explicit cast");
        return {do_cast(type, std::move(tv)), true};
    }
    return {RValue{}, false};
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
    return tv.type()->cast_to(type_id, tv.move_value());
}

RValue ExprVisitor::prim_cast(Ref<PrimType> type, PrimTypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    assert(tv.type()->is_expl_castable_to(type));
    return tv.type()->cast_to(type, tv.value().move_rvalue());
}

PrimTypedValue ExprVisitor::prim_binary_op_impl(
    Op op, PrimTypedValue&& left, PrimTypedValue&& right) {
    debug() << __FUNCTION__ << "\n";
    assert(!ops::is_assign(op));
    auto prim_tv = left.type()->binary_op(
        op, left.move_value(), right.type(), right.move_value());
    return {prim_tv.type(), prim_tv.move_value()};
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
