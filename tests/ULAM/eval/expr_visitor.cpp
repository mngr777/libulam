#include "./expr_visitor.hpp"
#include "./expr_flags.hpp"
#include "libulam/semantic/type/builtin_type_id.hpp"
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/value.hpp>

#ifdef DEBUG_EVAL_EXPR_VISITOR
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalExprVisitor] "
#endif
#include "src/debug.hpp"

#define DBG_LINE(node) debug() << _program->sm().line_at(node->loc_id())

namespace {

using flags_t = ulam::sema::ExprRes::flags_t;

std::string add_cast(std::string&& data, flags_t flags) {
    if ((flags & exp::ExplCast) || (flags & exp::ImplCast) == 0)
        return data + " cast";
    return std::move(data);
}

std::string add_array_access(
    const std::string& data, const std::string& idx_data, flags_t flags) {
    assert(!data.empty());

    // TODO: use a flag
    if (*data.rbegin() == '.' && (flags & exp::ExtMemberAccess)) {
        // ULAM quirk:
        // `a.b[c] -> `a b c [] .`, but
        // `/* self. */ b[c]` -> `self b . c []`
        return data.substr(0, data.size() - 1) + idx_data + " [] .";
    }
    return data + " " + idx_data + " []";
}

} // namespace

EvalExprVisitor::ExprRes
EvalExprVisitor::visit(ulam::Ref<ulam::ast::BoolLit> node) {
    auto res = ulam::sema::EvalExprVisitor::visit(node);
    if (res)
        res.set_data(std::string{node->value() ? "true" : "false"});
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::visit(ulam::Ref<ulam::ast::NumLit> node) {
    auto res = ulam::sema::EvalExprVisitor::visit(node);
    if (res)
        res.set_data(node->value().str());
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::visit(ulam::Ref<ulam::ast::StrLit> node) {
    auto res = ulam::sema::EvalExprVisitor::visit(node);
    if (res)
        res.set_data(std::string{text(node->value().id)});
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::type_op(
    ulam::Ref<ulam::ast::TypeOpExpr> node, ulam::Ref<ulam::Type> type) {
    auto res = ulam::sema::EvalExprVisitor::type_op(node, type);
    const auto& val = res.value();
    if (val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            res.set_data(_stringifier.stringify(res.type(), rval));
        });
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::apply_binary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ulam::LValue lval,
    ulam::Ref<ulam::ast::Expr> l_node,
    EvalExprVisitor::ExprRes&& left,
    ulam::Ref<ulam::ast::Expr> r_node,
    EvalExprVisitor::ExprRes&& right) {

    std::string data;
    if (left.has_data() && right.has_data()) {
        auto l_type = left.type()->actual();
        auto r_type = right.type()->actual();
        auto l_data = left.data<std::string>();
        auto r_data = right.data<std::string>();
        auto l_flags = left.flags();
        auto r_flags = right.flags();

        switch (ulam::ops::kind(op)) {
        case ulam::ops::Kind::Assign:
            if (r_type != l_type)
                r_data = add_cast(std::move(r_data), r_flags);
            break;
        case ulam::ops::Kind::Equality:
        case ulam::ops::Kind::Comparison:
            // cast right arg to exact type for comparison
            if (r_type != l_type) {
                if (l_type->is_prim() &&
                    ulam::has_bitsize(l_type->bi_type_id())) {
                    assert(r_type->is(l_type->bi_type_id()));
                    if (l_type->bitsize() < r_type->bitsize()) {
                        l_data = add_cast(std::move(l_data), l_flags);
                    } else {
                        r_data = add_cast(std::move(r_data), r_flags);
                    }
                } else {
                    r_data = add_cast(std::move(r_data), r_flags);
                }
            }
            break;
        case ulam::ops::Kind::Numeric: {
            if (ulam::ops::is_assign(op)) {
                if (r_type != l_type)
                    r_data = add_cast(std::move(r_data), r_flags);

            } else if (!l_type->is_class()) {
                // cast to 32 or 64 common bit width
                auto l_size = l_type->bitsize();
                auto r_size = r_type->bitsize();
                auto size = std::max(l_size, r_size);
                assert(size < 64);
                size = (size > 32) ? 64 : 32;
                if (l_size != size)
                    l_data = add_cast(std::move(l_data), l_flags);
                if (r_size != size)
                    r_data = add_cast(std::move(r_data), r_flags);
            }
            break;
        }
        default: {
        } // do nothing
        }

        data = l_data + " " + r_data + " " + ulam::ops::str(op);
        if (op == ulam::Op::Sum || op == ulam::Op::Diff)
            data += "b";
    }
    auto res = ulam::sema::EvalExprVisitor::apply_binary_op(
        node, op, lval, l_node, std::move(left), r_node, std::move(right));
    if (res && !data.empty()) {
        const auto& val = res.value();
        if (/* !ulam::ops::is_assign(op) && */ val.is_consteval()) {
            val.with_rvalue([&](const ulam::RValue& rval) {
                res.set_data(_stringifier.stringify(res.type(), rval));
            });
        } else {
            res.set_data(data);
        }
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::apply_unary_op(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Op op,
    ulam::LValue lval,
    ulam::Ref<ulam::ast::Expr> arg_node,
    EvalExprVisitor::ExprRes&& arg,
    ulam::Ref<ulam::ast::TypeName> type_name) {

    std::string data;
    if (arg.has_data()) {
        data = arg.data<std::string>();
        std::string op_str{ulam::ops::str(op)};
        if (op == ulam::Op::UnaryMinus || op == ulam::Op::UnaryPlus) {
            data = op_str + data;
        } else {
            data = data + " " + op_str;
        }
    }
    auto res = ulam::sema::EvalExprVisitor::apply_unary_op(
        node, op, lval, arg_node, std::move(arg), type_name);
    if (res && !data.empty())
        res.set_data(data);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::type_op(
    ulam::Ref<ulam::ast::TypeOpExpr> node, EvalExprVisitor::ExprRes res) {
    res = ulam::sema::EvalExprVisitor::type_op(node, std::move(res));
    if (!res)
        return res;

    const auto& val = res.value();
    if (val.is_consteval()) {
        val.with_rvalue([&](const ulam::RValue& rval) {
            res.set_data(_stringifier.stringify(res.type(), rval));
        });
    }
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::ident_self(ulam::Ref<ulam::ast::Ident> node) {
    auto res = ulam::sema::EvalExprVisitor::ident_self(node);
    if (res) {
        res.set_data(std::string{"self"});
        res.set_flag(exp::Self);
    }
    return res;
}

EvalExprVisitor::ExprRes
EvalExprVisitor::ident_super(ulam::Ref<ulam::ast::Ident> node) {
    auto res = ulam::sema::EvalExprVisitor::ident_super(node);
    if (res)
        res.set_data(std::string{"super"});
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::ident_var(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Var> var) {
    auto res = ulam::sema::EvalExprVisitor::ident_var(node, var);
    if (res)
        res.set_data(std::string{str(var->name_id())});
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::ident_prop(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::Prop> prop) {
    auto res = ulam::sema::EvalExprVisitor::ident_prop(node, prop);
    if (res) {
        auto data =
            std::string{"self "} + std::string{str(prop->name_id())} + " .";
        res.set_data(data);
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::ident_fset(
    ulam::Ref<ulam::ast::Ident> node, ulam::Ref<ulam::FunSet> fset) {
    auto res = ulam::sema::EvalExprVisitor::ident_fset(node, fset);
    if (res)
        res.set_data(std::string{"self {args}{fun} ."});
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_class(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    auto data = obj.data<std::string>("");
    bool ext_member_access = obj.has_flag(exp::ExtMemberAccess);
    if (!data.empty() && ext_member_access) {
        // ULAM quirk:
        // `a.b[c] -> `a b c [] .`, but
        // `/* self. */ b[c]` -> `self b . c []`
        // remove " ." at the end to append after `aref` call
        auto size = data.size();
        assert(size > 2 && data[size - 1] == '.' && data[size - 2] == ' ');
        obj.set_data(data.substr(0, data.size() - 2));
    }

    auto res = ulam::sema::EvalExprVisitor::array_access_class(
        node, std::move(obj), std::move(idx));
    if (!res)
        return res;

    data = res.data<std::string>();
    if (!data.empty() && ext_member_access)
        res.set_data(data + " .");
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_string(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    std::string data;
    if (obj.has_data() && idx.has_data())
        data = add_array_access(
            obj.data<std::string>(), idx.data<std::string>(), obj.flags());
    auto res = ulam::sema::EvalExprVisitor::array_access_string(
        node, std::move(obj), std::move(idx));
    if (!data.empty())
        res.set_data(std::move(data));
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_array(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    std::string data;
    if (obj.has_data() && idx.has_data())
        data = add_array_access(
            obj.data<std::string>(), idx.data<std::string>(), obj.flags());
    auto res = ulam::sema::EvalExprVisitor::array_access_array(
        node, std::move(obj), std::move(idx));
    if (!data.empty())
        res.set_data(std::move(data));
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_op(
    ulam::Ref<ulam::ast::MemberAccess> node, EvalExprVisitor::ExprRes&& obj) {
    if (!obj)
        return std::move(obj);
    auto data = obj.data<std::string>("");
    auto res =
        ulam::sema::EvalExprVisitor::member_access_op(node, std::move(obj));
    if (!res)
        return res;
    if (!data.empty()) {
        data += std::string{" operator"} + ulam::ops::str(node->op()) + " .";
        res.set_data(data);
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_var(
    ulam::Ref<ulam::ast::MemberAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    ulam::Ref<ulam::Var> var) {
    if (!obj)
        return std::move(obj);
    auto data = obj.data<std::string>("");
    auto res = ulam::sema::EvalExprVisitor::member_access_var(
        node, std::move(obj), var);
    if (!res)
        return res;
    if (!data.empty()) {
        data += std::string{" "} + std::string{str(var->name_id())} + " .";
        res.set_data(data);
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_prop(
    ulam::Ref<ulam::ast::MemberAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    ulam::Ref<ulam::Prop> prop) {
    if (!obj)
        return std::move(obj);
    bool is_self = obj.has_flag(exp::Self);
    auto data = obj.data<std::string>("");
    auto res = ulam::sema::EvalExprVisitor::member_access_prop(
        node, std::move(obj), prop);
    if (!res)
        return res;
    if (!data.empty()) {
        data += std::string{" "} + std::string{str(prop->name_id())} + " .";
        res.set_data(data);
        if (!is_self)
            res.set_flag(exp::ExtMemberAccess);
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::member_access_fset(
    ulam::Ref<ulam::ast::MemberAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    ulam::Ref<ulam::FunSet> fset) {
    if (!obj)
        return std::move(obj);
    auto data = obj.data<std::string>("");
    auto res = ulam::sema::EvalExprVisitor::member_access_fset(
        node, std::move(obj), fset);
    if (!res)
        return res;
    if (!data.empty())
        res.set_data(callable_data(data, fset));
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::bind(
    ulam::Ref<ulam::ast::Expr> node,
    ulam::Ref<ulam::FunSet> fset,
    ulam::sema::ExprRes&& obj) {
    assert(obj);
    auto data = obj.data<std::string>("");
    auto res = ulam::sema::EvalExprVisitor::bind(node, fset, std::move(obj));
    if (!res)
        return res;
    if (!data.empty())
        res.set_data(callable_data(data, fset));
    return res;
}

std::string EvalExprVisitor::callable_data(
    const std::string& data, ulam::Ref<ulam::FunSet> fset) {
    return data + " {args}{fun} .";
}
