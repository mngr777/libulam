#include "libulam/sema/resolver.hpp"
#include "libulam/semantic/expr_res.hpp"
#include <cassert>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>

#ifdef DEBUG_PARSER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::sema::ExprVisitor] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

// TODO: use canon types for casting

ExprRes ExprVisitor::visit(Ref<ast::TypeOpExpr> node) { return {}; }

ExprRes ExprVisitor::visit(Ref<ast::Ident> node) {
    auto name = node->name();
    auto sym = _scope->get(name.str_id());
    if (!sym) {
        diag().emit(
            Diag::Error, node->loc_id(), str(name.str_id()).size(),
            "symbol not found");
        return {ExprError::SymbolNotFound};
    }
    if (sym->is<Var>()) {
        auto var = sym->get<Var>();
        return {var->type(), LValue{var}};
    } else {
        assert(sym->is<FunSet>());
        auto fset = sym->get<FunSet>();
        return {builtins().type(FunId), LValue{fset}};
    }
    assert(false);
}

ExprRes ExprVisitor::visit(Ref<ast::ParenExpr> node) {
    return node->inner()->accept(*this);
}

ExprRes ExprVisitor::visit(Ref<ast::BinaryOp> node) {
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

        // check types
        auto left_prim_type = left.type()->as_prim();
        auto right_prim_type = right.type()->as_prim();
        auto errors = prim_binary_op_type_check(
            node->op(), left_prim_type, right_prim_type);

        // both ok?
        if (errors.first.ok() && errors.second.ok()) {
            // perform bin op
            return prim_binary_op(
                node->op(), {left_prim_type, left.move_value()},
                {right_prim_type, right.move_value()});
        }

        // cast to suggested types if possible
        auto recast = [&](PrimTypeError error, PrimTypedValue&& tv,
                          Ref<const ast::Expr> node) -> PrimTypedValue {
            // ok?
            if (error.ok()) {
                return std::move(tv);
            }
            // incompatible type?
            if (error.incompatible()) {
                diag().emit(
                    Diag::Error, node->loc_id(), 1, "incompatible type");
                return {};
            }
            if (error.requires_expl_cast()) {
                diag().emit(
                    Diag::Error, node->loc_id(), 1,
                    std::string{"suggest casting to"} +
                        std::string{builtin_type_str(error.suggested_type)});
                // continue to cast
            }
            return prim_cast(std::move(tv), error.suggested_type);
        };
        auto left_tv = recast(
            errors.first, {left_prim_type, left.move_value()}, node->lhs());
        auto right_tv = recast(
            errors.second, {right_prim_type, right.move_value()}, node->rhs());

        // did casting help?
        if (left_tv.type() && right_tv.type()) {
            // perform bin op
            return prim_binary_op(
                node->op(), std::move(left_tv), std::move(right_tv));
        }
        return {}; // give up

    } else {
        // TODO: operator funs for classes, error(?) for arrays, ...
        assert(false);
    }
    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::UnaryPreOp> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::UnaryPostOp> node) { return {}; }

ExprRes ExprVisitor::visit(Ref<ast::Cast> node) {
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
    // Bool(1)
    auto type = builtins().prim_type_tpl(BoolId)->type(diag(), node, 1);
    assert(type);
    return {type, Value{RValue{(Unsigned)node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::NumLit> node) {
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
    // String
    auto type = builtins().prim_type(StringId);
    assert(type);
    return {type, Value{RValue{node->value()}}};
}

ExprRes ExprVisitor::visit(Ref<ast::FunCall> node) {
    auto loc_id = node->callable()->loc_id();
    auto callable = node->callable()->accept(*this);
    if (!callable)
        return {};

    // find fun set
    auto rval = callable.value().rvalue();
    if (!rval->is<Ref<FunSet>>()) {
        diag().emit(Diag::Error, loc_id, 1, "is not a function name");
        return {};
    }
    auto fset = rval->get<Ref<FunSet>>();

    // eval args
    auto [arg_list, success] = eval_args(node->args());
    if (!success)
        return {};

    // find match
    auto match_res = fset->find_match(arg_list);
    if (match_res.size() == 1) {
        // success, one match found
        return funcall(*(match_res.begin()), std::move(arg_list));
    } else if (match_res.size() == 0) {
        diag().emit(Diag::Error, loc_id, 1, "no matching function found");
    } else {
        diag().emit(Diag::Error, loc_id, 1, "ambiguous funcall");
    }

    return {};
}

ExprRes ExprVisitor::visit(Ref<ast::MemberAccess> node) { return {}; }
ExprRes ExprVisitor::visit(Ref<ast::ArrayAccess> node) { return {}; }

ExprRes ExprVisitor::cast(
    loc_id_t loc_id,
    std::size_t len,
    ExprRes&& res,
    Ref<Type> type,
    bool is_expl) {
    if (res.type()->is_prim()) {
        if (!type->is_prim()) {
            diag().emit(
                Diag::Error, loc_id, len, "cannot cast to non-primitive type");
            return {ExprError::InvalidCast};
        }
        if (!res.type()->as_prim()->is_castable_to(type->builtin_type_id(), is_expl)) {
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

PrimTypedValue
ExprVisitor::prim_cast(PrimTypedValue&& tv, BuiltinTypeId type_id) {
    assert(tv.type()->is_expl_castable_to(type_id));
    return tv.type()->cast_to(type_id, tv.move_value());
}

ExprRes ExprVisitor::prim_binary_op(
    Op op, PrimTypedValue&& left, PrimTypedValue&& right) {
    auto prim_tv = left.type()->binary_op(
        op, left.move_value(), right.type(), right.move_value());
    return {prim_tv.type(), prim_tv.move_value()};
}

std::pair<TypedValueList, bool> ExprVisitor::eval_args(Ref<ast::ArgList> args) {
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

ExprRes ExprVisitor::funcall(Ref<Fun> fun, TypedValueList&& args) {
    return {fun->ret_type(), Value{}};
}

Diag& ExprVisitor::diag() { return _program->diag(); }
Builtins& ExprVisitor::builtins() { return _program->builtins(); }

std::string_view ExprVisitor::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema
