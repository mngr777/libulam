#pragma once
#include "libulam/ast/nodes/var_decl.hpp"
#include <cstdint>
#include <filesystem>
#include <libulam/ast/nodes.hpp>
#include <libulam/detail/variant.hpp>
#include <libulam/preproc.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/str_pool.hpp>
#include <libulam/token.hpp>
#include <stack>
#include <string_view>
#include <utility>

namespace ulam {

class Context;
class Preproc;

class Parser {
public:
    using Path = std::filesystem::path;

    Parser(Context& ctx, UniqStrPool& str_pool, UniqStrPool& text_pool):
        _ctx{ctx}, _str_pool{str_pool}, _text_pool{text_pool}, _pp{ctx} {}

    Ptr<ast::ModuleDef> parse_module_file(const Path& path);
    Ptr<ast::ModuleDef>
    parse_module_str(const std::string& text, const Path& path);

    Ptr<ast::Block> parse_stmts(std::string text);

    void add_str_src(const std::string& text, const Path& path);

private:
    using var_flags_t = std::uint8_t;
    static constexpr var_flags_t NoVarFlags = 0;
    static constexpr var_flags_t VarIsConst = 1;
    static constexpr var_flags_t VarAllowConstructorInit = 2;

    using expr_flags_t = std::uint8_t;
    static constexpr expr_flags_t NoExprFlags = 0;
    static constexpr expr_flags_t ExprStopAtComma = 1;

    using type_flags_t = std::uint8_t;
    static constexpr type_flags_t NoTypeFlags = 0;
    static constexpr type_flags_t TypeAllowSelf = 1;
    static constexpr type_flags_t TypeAllowSuper = 1 << 1;
    static constexpr type_flags_t TypeAllowLocal = 1 << 2;

    using ident_flags_t = std::uint8_t;
    static constexpr ident_flags_t NoIdentFlags = 0;
    static constexpr ident_flags_t IdentAllowSelfOrSuper = 1;
    static constexpr ident_flags_t IdentAllowLocal = 1 << 1;

    void consume();
    void putback(Token token);
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
    Ptr<ast::VarDefList> parse_var_def_list(var_flags_t flags = NoVarFlags);
    Ptr<ast::VarDefList> parse_var_def_list_rest(
        Ptr<ast::TypeName>&& base_type,
        ast::Str first_name,
        bool first_is_ref,
        var_flags_t flags = NoVarFlags);
    Ptr<ast::VarDef> parse_var_def(var_flags_t flags = NoVarFlags);
    Ptr<ast::VarDef> parse_var_def_rest(
        ast::Str name, bool is_ref, var_flags_t flags = NoVarFlags);
    Ptr<ast::FunDef>
    parse_fun_def_rest(Ptr<ast::FunRetType>&& ret_type, ast::Str name);
    Ptr<ast::FunDef> parse_op_fun_def_rest(
        Ptr<ast::FunRetType>&& ret_type, ast::Str name, tok::Type op_tok_type);
    Ptr<ast::FunDef> parse_constructor_def_rest(ast::Str name);
    Ptr<ast::ParamList> parse_param_list(bool allow_ellipsis = false);
    Ptr<ast::Param> parse_param(bool requires_value);
    std::pair<Ptr<ast::InitValue>, bool> parse_init(
        bool is_required,
        Ref<ast::ExprList> array_dims,
        var_flags_t flags = NoVarFlags);
    detail::Variant<Ptr<ast::InitList>, Ptr<ast::InitMap>>
    parse_init_list_or_map();
    Ptr<ast::InitList> parse_init_list();
    Ptr<ast::InitMap> parse_init_map();

    Ptr<ast::Block> parse_block();
    void parse_as_block(Ref<ast::Block> node, bool implicit_braces = false);
    Ptr<ast::Stmt> parse_stmt();
    Ptr<ast::Stmt> parse_stmt_local();
    Ptr<ast::Stmt> parse_stmt_type();
    Ptr<ast::ExprStmt> parse_expr_stmt();
    Ptr<ast::Stmt> parse_if_or_as_if();
    Ptr<ast::For> parse_for();
    Ptr<ast::While> parse_while();
    Ptr<ast::Which> parse_which();
    Ptr<ast::WhichCase> parse_which_case();
    Ptr<ast::Return> parse_return();
    Ptr<ast::Break> parse_break();
    Ptr<ast::Continue> parse_continue();

    Ptr<ast::Expr> parse_expr(expr_flags_t flags = NoExprFlags);
    Ptr<ast::Expr>
    parse_expr_climb(ops::Prec min_prec, expr_flags_t flags = NoExprFlags);
    Ptr<ast::Expr> parse_expr_climb_rest(
        Ptr<ast::Expr>&& lhs,
        ops::Prec min_prec,
        expr_flags_t flags = NoExprFlags);
    Ptr<ast::Expr> parse_expr_lhs(expr_flags_t flags = NoExprFlags);
    Ptr<ast::Expr> parse_expr_lhs_local(expr_flags_t flags = NoExprFlags);
    Ptr<ast::Expr> parse_paren_expr_or_cast(expr_flags_t flags = NoExprFlags);
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
    Ptr<ast::FunCall> parse_op_call();
    Ptr<ast::ArgList> parse_arg_list();
    Ptr<ast::ArrayAccess> parse_array_access(Ptr<ast::Expr>&& array);
    Ptr<ast::Expr> parse_member_access_type_op_or_op_call(Ptr<ast::Expr>&& obj);
    Ptr<ast::Expr> parse_member_access_or_op_call_rest(
        Ptr<ast::Expr>&& obj, loc_id_t op_loc_id);
    Ptr<ast::Ternary> parse_ternary(Ptr<ast::Expr>&& cond);
    Ptr<ast::ClassConstAccess>
    parse_class_const_access_rest(Ptr<ast::TypeName> type_name);
    Ptr<ast::TypeOpExpr> parse_expr_type_op(Ptr<ast::Expr>&& obj);
    Ptr<ast::TypeIdent> parse_type_ident(type_flags_t = NoTypeFlags);
    Ptr<ast::Ident> parse_ident(ident_flags_t flags = NoIdentFlags);
    bool parse_is_ref();
    Ptr<ast::BoolLit> parse_bool_lit();
    Ptr<ast::NumLit> parse_num_lit();
    Ptr<ast::NumLit> parse_char_lit();
    Ptr<ast::StrLit> parse_str_lit();

    template <typename N, typename... Args> Ptr<N> tree(Args&&... args);
    template <typename N, typename... Args>
    Ptr<N> tree_loc(loc_id_t loc_id, Args&&... args);

    std::string_view tok_str();
    ast::Str tok_ast_str();
    str_id_t tok_str_id();

    Context& _ctx;
    UniqStrPool& _str_pool;
    UniqStrPool& _text_pool;
    Preproc _pp;
    Token _tok;
    std::stack<Token> _back;
};

} // namespace ulam
