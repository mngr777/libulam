#pragma once
#include "libulam/ast/nodes/stmts.hpp"
#include <filesystem>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/preproc.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/str_pool.hpp>
#include <libulam/token.hpp>
#include <string_view>

namespace ulam {

class Context;
class Preproc;

class Parser {
public:
    explicit Parser(Context& ctx):
        _ctx{ctx}, _pp{ctx}, _ast{ast::make<ast::Root>()} {}

    void parse_file(const std::filesystem::path& path);
    void parse_string(const std::string& text);

    ast::Ptr<ast::Root> move_ast();

private:
    void parse();

    void consume();
    void consume_if(tok::Type type);
    bool eof();

    bool match(tok::Type type);
    bool expect(tok::Type type);
    void unexpected();
    void diag(std::string text);
    void diag(const Token& token, std::string text);
    void diag(loc_id_t loc_id, std::size_t size, std::string text);
    template <typename... Ts> void panic(Ts... stop);

    ast::Ptr<ast::ModuleDef> parse_module();
    void parse_module_var_or_type_def(ast::Ref<ast::ModuleDef> node);
    ast::Ptr<ast::TypeDef> parse_module_type_def(bool is_marked_local);
    ast::Ptr<ast::ClassDef> parse_class_def();
    ast::Ptr<ast::ClassDef> parse_class_def_head();
    void parse_class_def_body(ast::Ref<ast::ClassDef> node);
    ast::Ptr<ast::TypeDef> parse_type_def();
    ast::Ptr<ast::VarDefList> parse_var_def_list(bool is_const);
    ast::Ptr<ast::VarDefList> parse_var_def_list_rest(
        ast::Ptr<ast::TypeName>&& base_type,
        ast::Str first_name,
        bool is_const);
    ast::Ptr<ast::FunDef>
    parse_fun_def_rest(ast::Ptr<ast::TypeName>&& ret_type, ast::Str name);
    ast::Ptr<ast::ParamList> parse_param_list();
    ast::Ptr<ast::Param> parse_param(bool requires_value);

    ast::Ptr<ast::Block> parse_block();
    void parse_as_block(ast::Ref<ast::Block> node);
    ast::Ptr<ast::Stmt> parse_stmt();
    ast::Ptr<ast::If> parse_if();
    ast::Ptr<ast::For> parse_for();
    ast::Ptr<ast::While> parse_while();
    ast::Ptr<ast::Return> parse_return();

    ast::Ptr<ast::Expr> parse_expr();
    ast::Ptr<ast::Expr> parse_expr_climb(ops::Prec min_prec);
    ast::Ptr<ast::Expr> parse_expr_lhs();
    ast::Ptr<ast::Expr> parse_paren_expr_or_cast();
    ast::Ptr<ast::TypeOpExpr> parse_type_op();
    ast::Ptr<ast::TypeOpExpr>
    parse_type_op_rest(ast::Ptr<ast::TypeName>&& type);
    ast::Ptr<ast::TypeName> parse_type_name();
    ast::Ptr<ast::TypeSpec> parse_type_spec();
    ast::Ptr<ast::FunCall> parse_funcall(ast::Ptr<ast::Expr>&& obj);
    ast::Ptr<ast::ArgList> parse_arg_list();
    ast::Ptr<ast::ArrayAccess> parse_array_access(ast::Ptr<ast::Expr>&& array);
    ast::Ptr<ast::MemberAccess> parse_member_access(ast::Ptr<ast::Expr>&& obj);
    ast::Ptr<ast::TypeIdent> parse_type_ident();
    ast::Ptr<ast::Ident> parse_ident();
    ast::Ptr<ast::BoolLit> parse_bool_lit();
    ast::Ptr<ast::NumLit> parse_num_lit();
    ast::Ptr<ast::StrLit> parse_str_lit();

    template <typename N, typename... Args> ast::Ptr<N> tree(Args&&... args);

    std::string_view tok_str();
    ast::Str tok_ast_str();
    str_id_t tok_str_id();

    Context& _ctx;
    Preproc _pp;

    ast::Ptr<ast::Root> _ast;

    Token _tok;
};

} // namespace ulam
