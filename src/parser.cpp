#include "src/parser.hpp"
#include "libulam/ast/nodes/expr.hpp"
#include "libulam/lang/op.hpp"
#include "libulam/token.hpp"
#include "src/lexer.hpp"
#include <cassert>
#include <stdexcept> // TEST
#include <string>

#ifdef DEBUG_PARSER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Parser] "
#endif
#include "src/debug.hpp"

namespace ulam {

ast::Ptr<ast::Node> Parser::parse() {
    return expr(); // TEST
}

const Token& Parser::next() const { return _lex.peek(); }

void Parser::consume() { _lex.next(); }

void Parser::expect(tok::Type type) {
    // TODO
    if (next().type != type)
        throw std::invalid_argument(
            std::string("Unexpected token: ") + next().type_name() +
            " expecting " + tok::type_name(type));
    consume();
}

const std::string_view Parser::name_str() const {
    assert(next().is(tok::Name));
    return _ctx.name_str(next().str_id);
}

const std::string_view Parser::value_str() const {
    assert(next().is(tok::Number) || next().is(tok::String));
    return _ctx.value_str(next().str_id);
}

ast::Ptr<ast::Expr> Parser::expr() {
    debug() << "expr\n";
    Op pre = tok::unary_pre_op(next().type);
    if (pre != Op::none) {
        consume();
        return ast::make<ast::UnaryOp>(pre, expr_climb(op::prec(pre)));
    }
    return expr_climb(0);
}

ast::Ptr<ast::Expr> Parser::expr_climb(op::Prec min_prec) {
    debug() << "expr_climb " << (int) min_prec << "\n";
    auto lhs = atom();
    while (true) {
        Op op = next().bin_op();
        if (op::prec(op) < min_prec)
            break;
        consume();
        auto rhs = expr_climb(op::right_prec(op));
        lhs = ast::make<ast::BinaryOp>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

ast::Ptr<ast::Expr> Parser::atom() {
    debug() << "atom " << next().type_name() << "\n";
    switch (next().type) {
    case tok::Number:
        return number();
    case tok::String:
        return string();
    case tok::ParenOpen:
        return paren_expr();
    default:
        diag("unexpected token");
    }
    return nullptr;
}

ast::Ptr<ast::Expr> Parser::paren_expr() {
    debug() << "paren_expr\n";
    assert(next().is(tok::ParenOpen));
    consume();
    auto node = ast::make<ast::ParenExpr>(expr());
    expect(tok::ParenClose);
    return node;
}

ast::Ptr<ast::Number> Parser::number() {
    debug() << "number: " << value_str() << "\n";
    assert(next().is(tok::Number));
    auto node = ast::make<ast::Number>();
    consume();
    return node;
}

ast::Ptr<ast::String> Parser::string() {
    debug() << "string: " << value_str() << "\n";
    assert(next().is(tok::String));
    auto node = ast::make<ast::String>();
    consume();
    return node;
}

} // namespace ulam
