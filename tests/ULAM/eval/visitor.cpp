#include "./visitor.hpp"
#include "../out.hpp"
#include "./cast.hpp"
#include "./expr_res.hpp"
#include "./expr_visitor.hpp"
#include "./flags.hpp"
#include "./funcall.hpp"
#include "./init.hpp"
#include "libulam/ast/nodes/stmts.hpp"
#include "libulam/semantic/scope.hpp"
#include "src/semantic/detail/leximited.hpp"
#include <libulam/sema/eval/except.hpp>
#include <list>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/EvalVisitor] "
#endif
#include "src/debug.hpp"

namespace {

using ctx_type_t = EvalContextStack::type_t;

class ForContext {
public:
    static constexpr ctx_type_t Type = 1;
};

class WhileContext {
public:
    static constexpr ctx_type_t Type = 2;
};

class WhichContext {
public:
    static constexpr ctx_type_t Type = 3;

    WhichContext(const std::string& idx):
        _tmp_type_name{"_SWITCHTYPEDEF" + idx},
        _tmp_var_name{"Uh_switchcond" + idx} {}

    const std::string& tmp_type_name() const { return _tmp_type_name; }
    const std::string& tmp_var_name() const { return _tmp_var_name; }

    unsigned cond_num() const { return _conds.size(); }

    std::string cond_str() {
        std::string str;
        for (const auto& cond : _conds)
            str += (str.empty() ? "" : " ") + cond;
        for (unsigned i = 1; i < _conds.size(); ++i)
            str += " ||";
        _conds.clear();
        return str;
    }

    void add_cond(std::string&& cond) { _conds.push_back(std::move(cond)); }

    bool case_has_breaks() const { return _case_has_breaks; }

    void set_case_has_breaks(bool has_breaks) {
        _case_has_breaks = has_breaks;
        _has_breaks = _has_breaks || has_breaks;
    }

    bool has_breaks() const { return _has_breaks; }

private:
    std::string _tmp_type_name;
    std::string _tmp_var_name;
    std::list<std::string> _conds;
    bool _case_has_breaks{false};
    bool _has_breaks{false};
};

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
    {
        auto scope_raii = _scope_stack.raii<ulam::BasicScope>(scope());
        auto cond = node->cond();
        auto [res, cond_ctx] = eval_cond(cond, scope());
        if (cond->is_as_cond()) {
            add_as_cond(cond->as_cond(), cond_ctx.type());
        } else {
            append(exp::data(res));
            append("cond");
        }

        // if-branch
        maybe_wrap_stmt(node->if_branch(), cond->is_as_cond());
        append("if");
    }

    // else-branch
    if (node->has_else_branch()) {
        node->else_branch()->accept(*this);
        append("else");
    }

    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::For> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto ctx_raii = _ctx_stack.raii<ForContext>({});
    auto scope_raii = _scope_stack.raii<ulam::BasicScope>(
        scope(), ulam::scp::BreakAndContinue);
    block_open();

    // init
    if (!node->init()->is_empty())
        node->init()->accept(*this);

    // cond
    {
        auto iter_scope_raii = _scope_stack.raii<ulam::BasicScope>(scope());
        EvalCondContext cond_ctx;
        if (node->has_cond()) {
            auto cond = node->cond();
            auto [res, cond_ctx] = eval_cond(node->cond(), scope());
            if (cond->is_as_cond()) {
                add_as_cond(cond->as_cond(), cond_ctx.type());
            } else {
                append(exp::data(res));
                append("cond");
            }
        }


        // body
        if (node->has_body()) {
            bool is_as_cond = node->has_cond() && node->cond()->is_as_cond();
            maybe_wrap_stmt(node->body(), is_as_cond);
        }
    }

    append("_" + std::to_string(next_tmp_idx()) + ":");
    if (node->has_upd()) {
        auto upd_res = eval_expr(node->upd());
        if (upd_res.has_data())
            append(upd_res.data<std::string>());
    }
    append("while");
    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::While> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }

    auto tmp_idx = std::to_string(next_tmp_idx());
    auto ctx_raii = _ctx_stack.raii<WhileContext>({});
    auto scope_raii = _scope_stack.raii<ulam::BasicScope>(
        scope(), ulam::scp::BreakAndContinue);
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
    auto ctx_raii = _ctx_stack.raii<WhichContext>(tmp_idx);
    auto& ctx = _ctx_stack.top<WhichContext>();

    auto scope_raii =
        _scope_stack.raii<ulam::BasicScope>(scope(), ulam::scp::Break);
    block_open();

    ulam::Ptr<ulam::Var> var = make_which_tmp_var(node);
    assert(var);

    bool fallthru = false;
    unsigned non_default_num = 0;
    for (unsigned n = 0; n < node->case_num(); ++n) {
        auto case_ = node->case_(n);
        bool is_default = case_->is_default();
        bool has_branch = case_->has_branch();
        ctx.set_case_has_breaks(false);
        if (!is_default)
            ++non_default_num;

        if (case_->expr())
            which_match(node->expr(), case_->expr(), ulam::ref(var));

        bool has_cond = fallthru;
        if (!is_default && has_branch) {
            append(ctx.cond_str());
            append("cond");
            has_cond = true;

        } else if (is_default && n == 0) {
            // default case is the only one, t41038
            append("true");
            append("cond");
            has_cond = true;
        }

        if (has_branch) {
            case_->branch()->accept(*this);
            if (has_cond) {
                if (!is_default || !fallthru)
                    append("if");
            } else {
                append("else");
            }
        }
        fallthru = !has_branch;
    }

    if (non_default_num > 0)
        append("else");
    if (ctx.has_breaks())
        append("_" + label_idx + ":");
    block_close();
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Return> node) {
    if (!codegen_enabled())
        return Base::visit(node);

    auto res = ret_res(node);
    if (res.has_data()) {
        append(res.data<std::string>());
        append("return");
    }
}

void EvalVisitor::visit(ulam::Ref<ulam::ast::Break> node) {
    if (!codegen_enabled()) {
        Base::visit(node);
        return;
    }
    if (_ctx_stack.top_type_is(WhichContext::Type))
        _ctx_stack.top<WhichContext>().set_case_has_breaks(true);
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

ulam::Ptr<ulam::sema::Resolver>
EvalVisitor::_resolver(bool in_expr, ulam::sema::eval_flags_t flags) {
    return ulam::make<ulam::sema::Resolver>(*this, program(), in_expr, flags);
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

ulam::Ptr<ulam::Var> EvalVisitor::make_var(
    ulam::Ref<ulam::ast::TypeName> type_name,
    ulam::Ref<ulam::ast::VarDef> node,
    bool is_const) {
    // auto flags_ = flags();
    // if (is_const)
    //     flags_ |= evl::NoConstFold;
    // auto no_fold = flags_raii(flags_);
    return Base::make_var(type_name, node, is_const);
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

    auto& ctx = _ctx_stack.top<WhichContext>();
    auto res = Base::eval_which_expr(node);
    assert(res);

    auto stringifier = make_stringifier();
    auto type = res.type()->canon();
    auto type_str = out::type_str(stringifier, type);
    auto type_dim_str = out::type_dim_str(type);

    // tmp typedef
    append("typedef");
    append(type_str);
    append(ctx.tmp_type_name() + type_dim_str + "; ");

    // tmp var def
    append(type_str);
    append(ctx.tmp_var_name() + type_dim_str);
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
        const auto& ctx = _ctx_stack.top<WhichContext>();
        exp::set_data(expr_res, ctx.tmp_var_name());
    }
    auto res = Base::eval_which_match(
        expr, case_expr, std::move(expr_res), std::move(case_res));
    if (codegen_enabled()) {
        auto& ctx = _ctx_stack.top<WhichContext>();
        ctx.add_cond(exp::data(res));
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

void EvalVisitor::add_as_cond(
    ulam::Ref<ulam::ast::UnaryOp> as_cond, ulam::Ref<ulam::Type> type) {
    assert(as_cond->op() == ulam::Op::As);
    auto stringifier = make_stringifier();
    std::string name{str(as_cond->ident()->name_id())};
    append(name);
    append(out::type_str(stringifier, type, false));
    append("as");
    append("cond");
    set_next_prefix(
        " " + out::type_str(stringifier, type->ref_type()) + " " + name + "; ");
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

void EvalVisitor::maybe_wrap_stmt(ulam::Ref<ulam::ast::Stmt> stmt, bool wrap) {
    if (wrap && !stmt->is_block())
        block_open();

    stmt->accept(*this);

    if (wrap && !stmt->is_block())
        block_close();
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
