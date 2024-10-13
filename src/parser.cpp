#include "src/parser.hpp"
#include "libulam/ast/nodes/expr.hpp"
#include "libulam/ast/nodes/module.hpp"
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

void Parser::consume() {
    debug() << next().type_name() << " -> \n";
    _lex.next();
    debug() << "-> " << next().type_name() << "\n";
}

void Parser::expect(tok::Type type) {
    // TODO
    if (next().type != type)
        throw std::invalid_argument(
            std::string("Unexpected token: ") + next().type_name() +
            " expecting " + tok::type_name(type));
    consume();
}

bool Parser::eof() { return next().is(tok::None); }

ast::Ptr<ast::Module> Parser::module() {
    auto node = tree<ast::Module>();
    switch (next().type) {
    case tok::Element:
    case tok::Quark:
    case tok::Transient:
        node->add(class_def());
        break;

    default:
        diag("unexpected token");
    }
    return node;
}

ast::Ptr<ast::ClassDef> Parser::class_def() {
    assert(
        next().is(tok::Element) || next().is(tok::Quark) ||
        next().is(tok::Transient));
    debug() << "class_def " << next().type_name() << "\n";
    auto node = tree<ast::ClassDef>(next().class_kind());
    expect(tok::BraceOpen);
    while (!next().is(tok::BraceClose)) {
        switch (next().type) {
        case tok::Typedef:
            node->add(type_def());
            break;
        case tok::Name: {
            auto type = expr();
            auto id = name();
            if (next().is(tok::ParenOpen)) {
                node->add(fun_def_rest(std::move(type), std::move(id)));
            } else {
                node->add(var_def_rest(std::move(type), std::move(id)));
            }
        } break;
        default:
            diag("unexpected token");
        }
    }
    expect(tok::BraceClose);
    return node;
}

ast::Ptr<ast::TypeDef> Parser::type_def() {
    assert(next().type == tok::Typedef);
    debug() << "type_def\n";
    return nullptr; // TODO
}

ast::Ptr<ast::VarDef>
Parser::var_def_rest(ast::Ptr<ast::Expr>&& type, ast::Ptr<ast::Name>&& id) {
    debug() << "var_def_rest\n";
    return nullptr;
}
ast::Ptr<ast::FunDef>
Parser::fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, ast::Ptr<ast::Name>&& id) {
    debug() << "fun_def_rest\n";
    assert(next().type == tok::ParenOpen);
    return nullptr;
}

ast::Ptr<ast::Expr> Parser::expr() {
    debug() << "expr\n";
    Op pre = tok::unary_pre_op(next().type);
    if (pre != Op::None) {
        consume();
        return tree<ast::UnaryOp>(pre, expr_climb(ops::prec(pre)));
    }
    return expr_climb(0);
}

ast::Ptr<ast::Expr> Parser::expr_climb(ops::Prec min_prec) {
    debug() << "expr_climb " << (int)min_prec << "\n";
    auto lhs = cast_expr();
    while (true) {
        Op op = next().bin_op();
        if (ops::prec(op) < min_prec)
            break;
        debug() << "op: " << ops::str(op) << "\n";
        consume();
        auto rhs = expr_climb(ops::right_prec(op));
        lhs = tree<ast::BinaryOp>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

ast::Ptr<ast::Expr> Parser::cast_expr() {
    debug() << "cast_expr " << next().type_name() << "\n";
    switch (next().type) {
    case tok::Name:
        return name();
    case tok::Number:
        return number();
    case tok::String:
        return string();
    case tok::ParenOpen:
        return paren_expr_or_cast();
    default:
        diag("unexpected token");
    }
    return nullptr;
}

ast::Ptr<ast::Expr> Parser::paren_expr_or_cast() {
    debug() << "paren_expr\n";
    assert(next().is(tok::ParenOpen));
    consume();
    auto inner = expr();
    expect(tok::ParenClose);
    return tree<ast::ParenExpr>(std::move(inner));
}

ast::Ptr<ast::Name> Parser::name() {
    debug() << "name\n";
    assert(next().is(tok::Name));
    auto node = tree<ast::Name>();
    consume();
    return node;
}

ast::Ptr<ast::Number> Parser::number() {
    debug() << "number: " << value_str() << "\n";
    assert(next().is(tok::Number));
    auto node = tree<ast::Number>();
    consume();
    return node;
}

ast::Ptr<ast::String> Parser::string() {
    debug() << "string: " << value_str() << "\n";
    assert(next().is(tok::String));
    auto node = tree<ast::String>();
    consume();
    return node;
}

ast::Ptr<ast::Expr> Parser::binop_tree(
    Op op, ast::Ptr<ast::Expr>&& lhs, ast::Ptr<ast::Expr>&& rhs) {
    switch (op) {
    case Op::Funcall: {
        // TODO
        return nullptr;
    }
    case Op::ArrayAccess: {
        // TODO
        return nullptr;
    }
    default:
        return ast::make<ast::BinaryOp>(op, std::move(lhs), std::move(rhs));
    }
}

const std::string_view Parser::name_str() const {
    assert(next().is(tok::Name));
    return _ctx.name_str(next().str_id);
}

const std::string_view Parser::value_str() const {
    assert(next().is(tok::Number) || next().is(tok::String));
    return _ctx.value_str(next().str_id);
}

} // namespace ulam
