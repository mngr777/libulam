#include "./expr_visitor.hpp"
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

bool ends_with_cast(const std::string& data) {
    static const std::string_view Cast{"cast"};
    return data.size() > Cast.size() &&
           std::string_view{data}.substr(data.size() - Cast.size()) == Cast;
}

std::string add_cast(std::string&& data) {
    if (!ends_with_cast(data))
        return data + " cast";
    return std::move(data);
}

std::string
add_array_access(const std::string& data, const std::string& idx_data) {
    static const std::string_view Self_{"self "};
    assert(!data.empty());

    if (*data.rbegin() == '.') {
        // ULAM quirk:
        // `a.b[c] -> `a b c [] .`, but
        // `/* self. */ b[c]` -> `self b . c []`

        // Go 3 spaces back or (or to the beginning),
        // then compare substring of len 5 to "self "
        auto pos = std::string::npos;
        for (int i = 0; i < 3; ++i) {
            pos = data.rfind(' ', pos);
            // at least 2 spaces must be present, since we have member access op
            assert(i == 2 || pos != std::string::npos);
            assert(pos != 0); // data string can't start with ' '
            if (pos != std::string::npos)
                --pos;
        }
        pos = (pos == std::string::npos) ? 0 : pos + 1;
        if (std::string_view{data}.substr(pos, Self_.size()) != Self_)
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
    std::string data{};
    if (left.has_data() && right.has_data()) {
        auto l_data = left.data<std::string>();
        auto r_data = right.data<std::string>();

        switch (ulam::ops::kind(op)) {
        case ulam::ops::Kind::Equality:
            // cast right arg to exact type for comparison
            if (right.type() != left.type())
                r_data = add_cast(std::move(r_data));
            break;
        case ulam::ops::Kind::Numeric: {
            if (op == ulam::Op::Assign) {
                if (right.type() != left.type() &&
                    !right.value().is_consteval()) {
                    r_data = add_cast(std::move(r_data));
                }

            } else if (ulam::ops::is_assign(op)) {
                if (right.type() != left.type())
                    r_data = add_cast(std::move(r_data));

            } else {
                // cast to 32 or 64 common bit width
                auto l_size = left.type()->bitsize();
                auto r_size = right.type()->bitsize();
                auto size = std::max(l_size, r_size);
                assert(size < 64);
                size = (size > 32) ? 64 : 32;
                if (l_size != size)
                    l_data = add_cast(std::move(l_data));
                if (r_size != size)
                    r_data = add_cast(std::move(r_data));
            }
            break;
        }
        default: {
        } // do nothing
        }

        data = l_data + " " + r_data + " " + ulam::ops::str(op);
    }
    auto res = ulam::sema::EvalExprVisitor::apply_binary_op(
        node, op, lval, l_node, std::move(left), r_node, std::move(right));
    if (res && !data.empty())
        res.set_data(data);
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::type_op(
    ulam::Ref<ulam::ast::TypeOpExpr> node, EvalExprVisitor::ExprRes res) {
    res = ulam::sema::EvalExprVisitor::type_op(node, std::move(res));
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
    if (res)
        res.set_data(std::string{"self"});
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
    if (res) {
        auto data = std::string{"self {args}"} +
                    std::string{str(fset->name_id())} + " .";
        res.set_data(data);
    }
    return res;
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_class(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    // TODO
    return ulam::sema::EvalExprVisitor::array_access_class(
        node, std::move(obj), std::move(idx));
}

EvalExprVisitor::ExprRes EvalExprVisitor::array_access_string(
    ulam::Ref<ulam::ast::ArrayAccess> node,
    EvalExprVisitor::ExprRes&& obj,
    EvalExprVisitor::ExprRes&& idx) {
    std::string data;
    if (obj.has_data() && idx.has_data())
        data =
            add_array_access(obj.data<std::string>(), idx.data<std::string>());
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
        // data = obj.data<std::string>() + " " + idx.data<std::string>() + "
        // []";
        data =
            add_array_access(obj.data<std::string>(), idx.data<std::string>());
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
    auto data = obj.data<std::string>("");
    auto res = ulam::sema::EvalExprVisitor::member_access_prop(
        node, std::move(obj), prop);
    if (!res)
        return res;
    if (!data.empty()) {
        data += std::string{" "} + std::string{str(prop->name_id())} + " .";
        res.set_data(data);
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
    if (!data.empty()) {
        assert(fset->has_name_id());
        data +=
            std::string{" {args}"} + std::string{str(fset->name_id())} + " .";
        res.set_data(data);
    }
    return res;
}
