#include "./visitor.hpp"
#include "../out.hpp"
#include "./cast.hpp"
#include "./expr_res.hpp"
#include "./expr_visitor.hpp"
#include "./flags.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include "src/semantic/detail/leximited.hpp"
#include <libulam/sema/eval/except.hpp>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalVisitor] "
#endif
#include "src/debug.hpp"

namespace {

std::string which_tmp_type_name(const std::string& idx) {
    return "_SWITCHTYPEDEF" + idx;
}

std::string which_tmp_var_name(const std::string& idx) {
    return "Uh_switchcond" + idx;
}

} // namespace

ulam::sema::ExprRes EvalVisitor::eval(ulam::Ref<ulam::ast::Block> block) {
    // codegen
    // NOTE: ulam::sema::evl::NoExec set on first function call, see `funcall`
    Base::eval(block);

    // exec
    auto no_codegen_raii = flags_raii(flags() | evl::NoCodegen);
    return Base::eval(block);
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Block> node) {
    bool has_pref = false;
    std::size_t size{0};
    if (codegen_enabled()) {
        has_pref = !_next_prefix.empty();
        block_open();
        size = _data.size();
    }
    Base::visit(node);
    if (codegen_enabled()) {
        bool nospace = !has_pref && (_data.size() == size);
        block_close(nospace);
    }
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::If> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    block_open();

    // cond
    auto cond_res = eval_expr(node->cond());
    append(exp::data(cond_res));
    append("cond");

    // if-branch
    node->if_branch()->accept(*this);
    append("if");

    // else-branch
    if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
        append("else");
    }

    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::IfAs> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto stringifier = make_stringifier();
    block_open();

    // ident and type
    auto res = eval_as_cond_ident(node);
    auto type = resolve_as_cond_type(node);
    assert(!type->is_ref());
    append(exp::data(res));
    append(out::type_str(stringifier, type, false));
    append("as");
    append("cond");

    // make tmp var
    {
        ulam::Ptr<ulam::ast::VarDef> def{};
        auto scope_raii{_scope_stack.raii(ulam::scp::NoFlags)};
        if (!res.value().empty()) {
            auto dyn_type = res.value().dyn_obj_type();
            if (!dyn_type->is_same(type) &&
                !dyn_type->is_impl_castable_to(type))
                res = {type, ulam::Value{ulam::LValue{}}};
        }
        auto [var_def, var] =
            define_as_cond_var(node, std::move(res), type, scope());

        // add tmp variable def
        if (node->ident()->is_self()) {
            set_next_prefix(
                " " + out::type_str(stringifier, type->ref_type()) + " self; ");
        } else {
            assert(var);
            set_next_prefix(
                " " + out::var_def_str(str_pool(), stringifier, var) + "; ");
        }

        // if-branch, always in {} because of tmp var def
        bool is_block = node->if_branch()->is_block();
        if (!is_block)
            block_open();
        node->if_branch()->accept(*this);
        if (!is_block)
            block_close();

        append("if");
    }

    // else-branch
    if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
        append("else");
    }

    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::While> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto tmp_idx = std::to_string(next_tmp_idx());
    auto scope_raii = _scope_stack.raii(ulam::scp::Break | ulam::scp::Continue);
    block_open();

    // cond, TODO: cast to Bool
    auto cond_res = eval_expr(node->cond());
    if (cond_res.has_data()) {
        append(cond_res.data<std::string>());
        append("cond");
    }

    // body
    node->body()->accept(*this);

    append("_" + tmp_idx + ": while");
    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Which> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto label_idx = std::to_string(next_tmp_idx());
    auto tmp_idx = ulam::detail::leximited((ulam::Unsigned)next_tmp_idx());
    auto ctx_raii = _ctx_stack.raii<std::string>(tmp_idx);

    auto scope_raii{_scope_stack.raii(ulam::scp::Break)};
    block_open();

    ulam::Ptr<ulam::Var> var = make_which_tmp_var(node);
    assert(var);

    for (unsigned n = 0; n < node->case_num(); ++n) {
        auto case_ = node->case_(n);
        if (case_->expr())
            which_match(node->expr(), case_->expr(), ulam::ref(var));
        if (case_->has_branch()) {
            case_->branch()->accept(*this);
        }
        if (!case_->is_default()) {
            append("if");
        } else {
            append("else else _" + label_idx + ":");
        }
    }
    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::For> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto scope_raii = _scope_stack.raii(ulam::scp::Break | ulam::scp::Continue);
    block_open();

    // init
    if (node->has_init())
        node->init()->accept(*this);

    // cond, TODO: cast to Bool
    if (node->has_cond()) {
        auto cond_res = eval_expr(node->cond());
        if (cond_res.has_data())
            append(cond_res.data<std::string>());
    }
    append("cond");

    // body
    node->body()->accept(*this);

    append("_" + std::to_string(next_tmp_idx()) + ":");
    if (node->has_upd()) {
        auto upd_res = eval_expr(node->upd());
        if (upd_res.has_data())
            append(upd_res.data<std::string>());
    }
    append("while");
    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Return> node) {
    auto res = ret_res(node);
    if (codegen_enabled()) {
        if (res.has_data()) {
            append(res.data<std::string>());
            append("return");
        }
    } else {
        throw ulam::sema::EvalExceptReturn(node, std::move(res));
    }
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Break> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }
    append("break");
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Continue> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }
    append("goto");
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::ExprStmt> node) {
    if (!node->has_expr())
        return;
    auto res = eval_expr(node->expr());
    if (codegen_enabled())
        append(exp::data(res));
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::EmptyStmt> node) {
    if (codegen_enabled())
        append(";");
}

bool EvalVisitor::codegen_enabled() const {
    bool is_enabled = in_main() && !has_flag(evl::NoCodegen);
    assert(!is_enabled || has_flag(ulam::sema::evl::NoExec));
    return is_enabled;
}

ulam::sema::ExprRes EvalVisitor::funcall(
    ulam::Ref<ulam::Fun> fun,
    ulam::LValue self,
    ulam::sema::ExprResList&& args) {
    if (_stack.size() == 0 && !has_flag(evl::NoCodegen)) {
        auto no_exec_raii = flags_raii(flags() | ulam::sema::evl::NoExec);
        return Base::funcall(fun, self, std::move(args));
    } else {
        return Base::funcall(fun, self, std::move(args));
    }
}

ulam::Ptr<ulam::sema::EvalExprVisitor> EvalVisitor::_expr_visitor(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalExprVisitor>(*this, program(), scope, flags);
}

ulam::Ptr<ulam::sema::EvalInit> EvalVisitor::_init_helper(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalInit>(*this, program(), scope, flags);
}

ulam::Ptr<ulam::sema::EvalCast> EvalVisitor::_cast_helper(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalCast>(*this, program(), scope, flags);
}

ulam::Ptr<ulam::sema::EvalFuncall> EvalVisitor::_funcall_helper(
    ulam::Ref<ulam::Scope> scope, ulam::sema::eval_flags_t flags) {
    return ulam::make<EvalFuncall>(*this, program(), scope, flags);
}

ulam::Ref<ulam::AliasType>
EvalVisitor::type_def(ulam::Ref<ulam::ast::TypeDef> node) {
    auto alias_type = Base::type_def(node);
    if (alias_type && codegen_enabled()) {
        auto stringifier = make_stringifier();
        append(out::type_def_str(stringifier, alias_type) + "; ");
    }
    return alias_type;
}

void EvalVisitor::var_init_expr(
    ulam::Ref<ulam::Var> var, ulam::sema::ExprRes&& init, bool in_expr) {
    std::string data;
    if (!in_expr && codegen_enabled())
        data = exp::data(init);
    Base::var_init_expr(var, std::move(init), in_expr);
    if (!in_expr && codegen_enabled()) {
        append("=");
        append(data + "; ");
    }
}

void EvalVisitor::var_init_default(ulam::Ref<ulam::Var> var, bool in_expr) {
    Base::var_init_default(var, in_expr);
    if (!in_expr && codegen_enabled())
        append("; ", true);
}

void EvalVisitor::var_init(ulam::Ref<ulam::Var> var, bool in_expr) {
    if (!in_expr && codegen_enabled()) {
        auto stringifier = make_stringifier();
        append(out::var_def_str(str_pool(), stringifier, var));
    }
}

ulam::Ptr<ulam::Var>
EvalVisitor::make_which_tmp_var(ulam::Ref<ulam::ast::Which> node) {
    if (!codegen_enabled())
        return Base::make_which_tmp_var(node);

    const auto& tmp_idx = _ctx_stack.top<std::string>();
    auto res = Base::eval_which_expr(node);
    assert(res);

    auto stringifier = make_stringifier();
    auto type = res.type()->canon();
    auto type_str = out::type_str(stringifier, type);
    auto type_dim_str = out::type_dim_str(type);

    // tmp typedef
    append("typedef");
    append(type_str);
    append(which_tmp_type_name(tmp_idx) + type_dim_str + "; ");
    // tmp var def
    append(type_str);
    append(which_tmp_var_name(tmp_idx) + type_dim_str);
    append("=");
    append(exp::data(res) + "; ");

    return ulam::make<ulam::Var>(res.move_typed_value());
}

ulam::sema::ExprRes EvalVisitor::eval_which_match(
    ulam::Ref<ulam::ast::Expr> expr,
    ulam::Ref<ulam::ast::Expr> case_expr,
    ulam::sema::ExprRes&& expr_res,
    ulam::sema::ExprRes&& case_res) {
    if (codegen_enabled()) {
        const auto& idx = _ctx_stack.top<std::string>();
        exp::set_data(expr_res, which_tmp_var_name(idx));
    }
    auto res = Base::eval_which_match(
        expr, case_expr, std::move(expr_res), std::move(case_res));
    if (codegen_enabled()) {
        append(exp::data(res));
        append("cond");
    }
    return res;
}

ulam::sema::ExprRes EvalVisitor::_eval_expr(
    ulam::Ref<ulam::ast::Expr> expr, ulam::sema::eval_flags_t flags_) {
    auto res = Base::_eval_expr(expr, flags_);
    assert(res);
    if (codegen_enabled()) {
        debug() << "expr: "
                << (res.has_data() ? res.data<std::string>()
                                   : std::string{"no data"})
                << "\n";
    }
    return res;
}

ulam::sema::ExprRes EvalVisitor::_to_boolean(
    ulam::Ref<ulam::ast::Expr> expr,
    ulam::sema::ExprRes&& res,
    ulam::sema::eval_flags_t flags_) {
    return Base::_to_boolean(expr, std::move(res), flags_ | evl::NoCodegen);
}

void EvalVisitor::block_open(bool nospace) {
    auto prefix = move_next_prefix();
    append("{", nospace);
    if (!prefix.empty())
        append(std::move(prefix), true);
}

void EvalVisitor::block_close(bool nospace) { append("}", nospace); }

void EvalVisitor::append(std::string data, bool nospace) {
    if (!nospace && !_data.empty())
        _data += " ";
    if (!_next_prefix.empty())
        _data += move_next_prefix();
    _data += std::move(data);
}

void EvalVisitor::set_next_prefix(std::string prefix) {
    assert(!prefix.empty());
    assert(_next_prefix.empty());
    _next_prefix = prefix;
}

std::string EvalVisitor::move_next_prefix() {
    std::string prefix;
    std::swap(prefix, _next_prefix);
    return prefix;
}

Stringifier EvalVisitor::make_stringifier() {
    Stringifier stringifier{program()};
    return stringifier;
}
