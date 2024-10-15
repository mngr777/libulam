#pragma once
#include "context.hpp"
#include "lexer.hpp"
#include "libulam/ast.hpp"
#include "libulam/ast/nodes/module.hpp"
#include "libulam/ast/nodes/params.hpp"
#include "libulam/lang/ops.hpp"
#include "libulam/token.hpp"
#include <string>

namespace ulam {

class SourceStream;

class Parser {
public:
    Parser(Context& ctx, SourceStream& ss): _ctx{ctx}, _lex{_ctx, ss} {}

    ast::Ptr<ast::Node> parse();

private:
    const Token& next() const;
    void consume();
    void expect(tok::Type type);
    bool eof();

    void diag(std::string message) {}

    template <typename N, typename... Args> ast::Ptr<N> tree(Args&&... args) {
        return ast::make<N>(std::forward<Args>(args)...);
    }

    ast::Ptr<ast::Module> parse_module();
    ast::Ptr<ast::ClassDef> parse_class_def();
    ast::Ptr<ast::TypeDef> parse_type_def();
    ast::Ptr<ast::VarDef> parse_var_def();
    ast::Ptr<ast::VarDef>
    parse_var_def_rest(ast::Ptr<ast::Expr>&& type, std::string&& name_);
    ast::Ptr<ast::FunDef>
    parse_fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name_);
    ast::Ptr<ast::ParamList> parse_param_list();
    ast::Ptr<ast::Param> parse_param();
    ast::Ptr<ast::Block> parse_block();

    ast::Ptr<ast::Expr> parse_expr();
    ast::Ptr<ast::Expr> parse_expr_climb(ops::Prec min_prec);
    ast::Ptr<ast::Expr> parse_cast_expr();
    ast::Ptr<ast::Expr> parse_paren_expr_or_cast();
    ast::Ptr<ast::FunCall> parse_funcall(ast::Ptr<ast::Expr>&& obj);
    ast::Ptr<ast::ArrayAccess> parse_array_access(ast::Ptr<ast::Expr>&& obj);
    ast::Ptr<ast::Name> parse_name();
    ast::Ptr<ast::Number> parse_number();
    ast::Ptr<ast::String> parse_string();

    ast::Ptr<ast::Expr>
    binop_tree(Op op, ast::Ptr<ast::Expr>&& lhs, ast::Ptr<ast::Expr>&& rhs);

    std::string name_str() const;
    std::string value_str() const;

    Context& _ctx;
    Lexer _lex;
};

} // namespace ulam
