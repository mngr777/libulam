#pragma once
#include "context.hpp"
#include "lexer.hpp"
#include "libulam/ast.hpp"
#include "libulam/lang/op.hpp"
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

    const std::string_view name_str() const;
    const std::string_view value_str() const;

    void diag(std::string message) {}

    template <typename N, typename... Args> ast::Ptr<N> tree(Args... args) {
        return ast::make<N>(std::forward<Args>(args)...);
    }

    ast::Ptr<ast::Expr> expr();
    ast::Ptr<ast::Expr> expr_climb(op::Prec min_prec);
    ast::Ptr<ast::Expr> cast_expr();
    ast::Ptr<ast::Expr> name();
    ast::Ptr<ast::Expr> paren_expr_or_cast();
    ast::Ptr<ast::Number> number();
    ast::Ptr<ast::String> string();

    Context& _ctx;
    Lexer _lex;
};

} // namespace ulam
