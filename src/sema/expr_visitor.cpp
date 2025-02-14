#include <cassert>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/bound.hpp>

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
    return {};
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
        [&](Ref<Var> var) -> ExprRes { return {var->type(), LValue{var}}; },
        [&](Ref<Prop> prop) -> ExprRes {
            return {prop->type(), LValue{BoundProp{_scope->self(), prop}}};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            return {
                builtins().type(FunId),
                LValue{BoundFunSet{_scope->self(), fset}}};
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
        val = prim_type->cast_to(prim_cast_type, std::move(val));
        return {cast_type, std::move(val)};

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
        return {type, RValue{number.value<Integer>()}};
    } else {
        // Unsigned(n)
        auto tpl = builtins().prim_type_tpl(UnsignedId);
        auto type = tpl->type(diag(), node, number.bitsize());
        assert(type);
        return {type, RValue{number.value<Unsigned>()}};
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
    if (!lval->is<BoundFunSet>()) {
        diag().emit(Diag::Error, loc_id, 1, "is not a function");
        return {};
    }
    auto& bound_fset = lval->get<BoundFunSet>();
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
        return funcall(*(match_res.begin()), obj_view, std::move(arg_list));
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
    if (!obj_res.ok()) {
        diag().emit(Diag::Error, node->obj()->loc_id(), 1, "object not found");
        return {ExprError::SymbolNotFound};
    }

    // is an object?
    if (!obj_res.type()->is_class()) {
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
        [&](Ref<Var> var) -> ExprRes { return {var->type(), LValue{var}}; },
        [&](Ref<Prop> prop) -> ExprRes {
            return {prop->type(), LValue{make_bound_prop(prop)}};
        },
        [&](Ref<FunSet> fset) -> ExprRes {
            auto obj_cls = get_obj_cls();
            if (fset->is_virtual() && obj_cls != cls) {
                auto sym = obj_cls->get(name.str_id());
                if (sym->is<FunSet>())
                    fset = sym->get<FunSet>();
            }
            return {builtins().type(FunId), LValue{make_bound_fset(fset)}};
        },
        [&](auto other) -> ExprRes { assert(false); });
}

ExprRes ExprVisitor::visit(Ref<ast::ArrayAccess> node) {
    debug() << __FUNCTION__ << " ArrayAccess\n";
    return {};
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

ExprRes ExprVisitor::prim_binary_op(
    Ref<ast::BinaryOp> node, PrimTypedValue&& left, PrimTypedValue&& right) {
    debug() << __FUNCTION__ << " " << ops::str(node->op()) << "\n";
    Op op = ops::non_assign(node->op());
    PrimTypeErrorPair type_errors{};
    if (op != Op::None) {
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
                return prim_cast(std::move(tv), error.suggested_type);
            case PrimTypeError::Ok:
                return std::move(tv);
            default:
                assert(false);
            }
        };
        left = recast(type_errors.first, std::move(left), node->lhs());
        right = recast(type_errors.second, std::move(right), node->rhs());

        // apply op
        right = prim_binary_op_impl(op, std::move(left), std::move(right));
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
ExprVisitor::assign(Ref<ast::BinaryOp> node, LValue* lval, TypedValue&& tv) {
    debug() << __FUNCTION__ << "\n";
    return lval->accept(
        [&](Ref<Var> var) -> ExprRes { assert(false && "assign to var"); },
        [&](BoundProp& bound_prop) -> ExprRes {
            assert(bound_prop.mem()->type() == tv.type()); // TMP
            auto rval = tv.value().move_rvalue();
            if (!rval.empty())
                bound_prop.store(std::move(rval));
            return {bound_prop.mem()->type(), LValue{bound_prop}};
        },
        [&](auto&& other) -> ExprRes { assert(false); });
}

PrimTypedValue
ExprVisitor::prim_cast(PrimTypedValue&& tv, BuiltinTypeId type_id) {
    debug() << __FUNCTION__ << "\n";
    assert(tv.type()->is_expl_castable_to(type_id));
    return tv.type()->cast_to(type_id, tv.move_value());
}

PrimTypedValue ExprVisitor::prim_binary_op_impl(
    Op op, PrimTypedValue&& left, PrimTypedValue&& right) {
    debug() << __FUNCTION__ << "\n";
    assert(!ops::is_assign(op));
    auto prim_tv = left.type()->binary_op(
        op, left.move_value(), right.type(), right.move_value());
    return {prim_tv.type(), prim_tv.move_value()};
}

ExprRes
ExprVisitor::funcall(Ref<Fun> fun, ObjectView obj_view, TypedValueList&& args) {
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
