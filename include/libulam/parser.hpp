#pragma once
#include <libulam/str_pool.hpp>
#include <libulam/ast.hpp>
#include <libulam/lang/ops.hpp>
#include <libulam/preproc.hpp>
#include <libulam/token.hpp>
#include <string>

namespace ulam {

class Context;

class Parser {
public:
    explicit Parser(Context& ctx);

    ast::Ptr<ast::Module> parse_file(const std::filesystem::path& path);
    ast::Ptr<ast::Module> parse_string(const std::string& text);

private:
    template <typename... Ts> void consume(Ts... types);
    bool eof();

    bool match(tok::Type type);
    bool expect(tok::Type type);
    void unexpected();
    void diag(std::string text);
    void diag(const Token& token, std::string text);
    template <typename... Ts> void panic(Ts... stop);

    ast::Ptr<ast::Module> parse_module();
    ast::Ptr<ast::ClassDef> parse_class_def();
    ast::Ptr<ast::ClassDef> parse_class_def_head();
    void parse_class_def_body(ast::Ref<ast::ClassDef> node);
    ast::Ptr<ast::TypeDef> parse_type_def();
    ast::Ptr<ast::VarDefList> parse_var_def_list_rest(
        ast::Ptr<ast::Expr>&& base_type, std::string&& first_name);
    ast::Ptr<ast::FunDef>
    parse_fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name_);
    ast::Ptr<ast::ParamList> parse_param_list();
    ast::Ptr<ast::Param> parse_param();

    ast::Ptr<ast::Block> parse_block();
    ast::Ptr<ast::Stmt> parse_stmt();
    ast::Ptr<ast::If> parse_if();
    ast::Ptr<ast::For> parse_for();
    ast::Ptr<ast::While> parse_while();

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

    // TODO: return string_view
    std::string tok_str();
    str_id_t tok_str_id();

    Context& _ctx;
    Preproc _pp;

    ast::Ptr<ast::Context> _ast_ctx; // TODO: move to ast root

    Token _tok;
};

} // namespace ulam
