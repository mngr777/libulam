#pragma once
#include "libulam/ast/nodes/access.hpp"
#include <cstdint>
#include <filesystem>
#include <libulam/ast/nodes.hpp>
#include <libulam/preproc.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/str_pool.hpp>
#include <libulam/token.hpp>
#include <string_view>
#include <utility>

namespace ulam {

class Context;
class Preproc;

class Parser {
public:
    explicit Parser(Context& ctx, UniqStrPool& str_pool):
        _ctx{ctx}, _str_pool{str_pool}, _pp{ctx} {}

    Ptr<ast::ModuleDef> parse_module_file(const std::filesystem::path& path);
    Ptr<ast::ModuleDef>
    parse_module_str(const std::string& text, const std::string& name);

    Ptr<ast::Block> parse_stmts(std::string text);

private:
    using expr_flags_t = std::uint8_t;
    static constexpr expr_flags_t ExprNoFlags = 0;
    static constexpr expr_flags_t ExprStopAtComma = 1;

    void consume();
    void consume_if(tok::Type type);

    bool match(tok::Type type);
    bool expect(tok::Type type);
    void unexpected();
    void diag(std::string text);
    void diag(const Token& token, std::string text);
    void diag(loc_id_t loc_id, std::size_t size, std::string text);
    template <typename... Ts> void panic(Ts... stop);

    Ptr<ast::ModuleDef> parse_module(const std::string_view name);
    void parse_module_var_or_type_def(Ref<ast::ModuleDef> node);
    Ptr<ast::TypeDef> parse_module_type_def(bool is_marked_local);
    Ptr<ast::ClassDef> parse_class_def();
    Ptr<ast::ClassDef> parse_class_def_head();
    Ptr<ast::TypeNameList> parse_class_ancestor_list();
    void parse_class_def_body(Ref<ast::ClassDef> node);
    Ptr<ast::TypeDef> parse_type_def();
    bool parse_class_var_or_fun_def(Ref<ast::ClassDefBody> node);
    std::pair<ast::Str, tok::Type> parse_op_fun_name();
    Ptr<ast::VarDefList> parse_var_def_list(bool is_const);
    Ptr<ast::VarDefList> parse_var_def_list_rest(
        Ptr<ast::TypeName>&& base_type,
        bool is_const,
        ast::Str first_name,
        bool first_is_ref);
    Ptr<ast::VarDef> parse_var_def();
    Ptr<ast::VarDef> parse_var_def_rest(ast::Str name, bool is_ref);
    Ptr<ast::FunDef>
    parse_fun_def_rest(Ptr<ast::FunRetType>&& ret_type, ast::Str name);
    Ptr<ast::FunDef> parse_op_fun_def_rest(
        Ptr<ast::FunRetType>&& ret_type, ast::Str name, tok::Type op_tok_type);
    Ptr<ast::ParamList> parse_param_list(bool allow_ellipsis = false);
    Ptr<ast::Param> parse_param(bool requires_value);

    std::tuple<bool, Ptr<ast::Expr>, Ptr<ast::InitList>>
    parse_init_value_or_list(bool is_required, Ref<ast::ExprList> array_dims);
    Ptr<ast::InitList> parse_init_list();
    bool validate_init_list(Ref<ast::InitList> list);

    Ptr<ast::Block> parse_block();
    void parse_as_block(Ref<ast::Block> node, bool implicit_braces = false);
    Ptr<ast::Stmt> parse_stmt();
    Ptr<ast::Stmt> parse_if_or_as_if();
    Ptr<ast::For> parse_for();
    Ptr<ast::While> parse_while();
    Ptr<ast::Return> parse_return();
    Ptr<ast::Break> parse_break();
    Ptr<ast::Continue> parse_continue();

    Ptr<ast::Expr> parse_expr(expr_flags_t flags = ExprNoFlags);
    Ptr<ast::Expr>
    parse_expr_climb(ops::Prec min_prec, expr_flags_t flags = ExprNoFlags);
    Ptr<ast::Expr> parse_expr_climb_rest(
        Ptr<ast::Expr>&& lhs,
        ops::Prec min_prec,
        expr_flags_t flags = ExprNoFlags);
    Ptr<ast::Expr> parse_expr_lhs(expr_flags_t flags = ExprNoFlags);
    Ptr<ast::Expr> parse_paren_expr_or_cast(expr_flags_t flags = ExprNoFlags);
    Ptr<ast::Expr> parse_class_const_access_or_type_op();
    Ptr<ast::TypeOpExpr>
    parse_type_op_rest(Ptr<ast::TypeName>&& type, Ptr<ast::Expr>&& expr);
    Ptr<ast::TypeExpr> parse_type_expr();
    Ptr<ast::ExprList> parse_array_dims(bool allow_empty = false);
    Ptr<ast::FullTypeName>
    parse_full_type_name(bool maybe_type_op_or_const = false);
    Ptr<ast::TypeName> parse_type_name(bool maybe_type_op = false);
    Ptr<ast::TypeSpec> parse_type_spec();
    Ptr<ast::FunCall> parse_funcall(Ptr<ast::Expr>&& callable);
    Ptr<ast::ArgList> parse_arg_list();
    Ptr<ast::ArrayAccess> parse_array_access(Ptr<ast::Expr>&& array);
    Ptr<ast::Expr> parse_member_access_or_type_op(Ptr<ast::Expr>&& obj);
    Ptr<ast::MemberAccess>
    parse_member_access_rest(Ptr<ast::Expr>&& obj, loc_id_t op_loc_id);
    Ptr<ast::ClassConstAccess>
    parse_class_const_access_rest(Ptr<ast::TypeName> type_name);
    Ptr<ast::TypeOpExpr> parse_expr_type_op(Ptr<ast::Expr>&& obj);
    Ptr<ast::TypeIdent> parse_type_ident(bool allow_self = false);
    Ptr<ast::Ident> parse_ident(bool allow_self = false);
    bool parse_is_ref();
    Ptr<ast::BoolLit> parse_bool_lit();
    Ptr<ast::NumLit> parse_num_lit();
    Ptr<ast::StrLit> parse_str_lit();

    template <typename N, typename... Args> Ptr<N> tree(Args&&... args);
    template <typename N, typename... Args>
    Ptr<N> tree_loc(loc_id_t loc_id, Args&&... args);

    std::string_view tok_str();
    ast::Str tok_ast_str();
    str_id_t tok_str_id();

    Context& _ctx;
    UniqStrPool& _str_pool;
    Preproc _pp;

    Token _tok;
};

} // namespace ulam
