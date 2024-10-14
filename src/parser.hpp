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

    ast::Ptr<ast::Module> module();
    ast::Ptr<ast::ClassDef> class_def();
    ast::Ptr<ast::TypeDef> type_def();
    ast::Ptr<ast::VarDef> var_def();
    ast::Ptr<ast::VarDef>
    var_def_rest(ast::Ptr<ast::Expr>&& type, std::string&& name_);
    ast::Ptr<ast::FunDef>
    fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name_);
    ast::Ptr<ast::ParamList> param_list();
    ast::Ptr<ast::Param> param();
    ast::Ptr<ast::Block> block();

    ast::Ptr<ast::Expr> expr();
    ast::Ptr<ast::Expr> expr_climb(ops::Prec min_prec);
    ast::Ptr<ast::Expr> cast_expr();
    ast::Ptr<ast::Expr> paren_expr_or_cast();
    ast::Ptr<ast::Name> name();
    ast::Ptr<ast::Number> number();
    ast::Ptr<ast::String> string();

    ast::Ptr<ast::Expr>
    binop_tree(Op op, ast::Ptr<ast::Expr>&& lhs, ast::Ptr<ast::Expr>&& rhs);

    std::string name_str() const;
    std::string value_str() const;

    Context& _ctx;
    Lexer _lex;
};

} // namespace ulam
