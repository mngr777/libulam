#include "src/parser.hpp"
#include "libulam/ast/nodes/expr.hpp"
#include "libulam/ast/nodes/module.hpp"
#include "libulam/ast/nodes/params.hpp"
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

ast::Ptr<ast::Node> Parser::parse() { return parse_module(); }

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

bool Parser::eof() { return next().is(tok::Eof); }

ast::Ptr<ast::Module> Parser::parse_module() {
    auto node = tree<ast::Module>();
    switch (next().type) {
    case tok::Element:
    case tok::Quark:
    case tok::Transient:
        node->add(parse_class_def());
        break;
    default:
        diag("unexpected token");
    }
    return node;
}

ast::Ptr<ast::ClassDef> Parser::parse_class_def() {
    assert(
        next().is(tok::Element) || next().is(tok::Quark) ||
        next().is(tok::Transient));
    debug() << "class_def " << next().type_name() << "\n";
    auto kind = next().class_kind();
    consume();
    auto node = tree<ast::ClassDef>(kind, name_str());
    consume();
    // TODO: template params, ancestors
    expect(tok::BraceL);
    while (!next().is(tok::BraceR)) {
        switch (next().type) {
        case tok::Typedef:
            node->body()->add(parse_type_def());
            break;
        case tok::Name: {
            auto type = parse_expr();
            auto name_ = name_str();
            consume();
            if (next().is(tok::ParenL)) {
                node->body()->add(parse_fun_def_rest(std::move(type), std::move(name_)));
            } else {
                node->body()->add(parse_var_def_rest(std::move(type), std::move(name_)));
            }
        } break;
        default:
            diag("unexpected token");
        }
    }
    expect(tok::BraceR);
    return node;
}

ast::Ptr<ast::TypeDef> Parser::parse_type_def() {
    assert(next().type == tok::Typedef);
    debug() << "type_def\n";
    consume();
    auto type = parse_expr();
    auto alias = name_str();
    consume();
    expect(tok::Semicolon);
    return tree<ast::TypeDef>(std::move(alias), std::move(type));
}

ast::Ptr<ast::VarDef>
Parser::parse_var_def_rest(ast::Ptr<ast::Expr>&& type, std::string&& name_) {
    debug() << "var_def_rest\n";
    ast::Ptr<ast::Expr> value = nullptr;
    if (next().is(tok::Assign)) {
        consume();
        value = parse_expr();
    }
    expect(tok::Semicolon);
    return tree<ast::VarDef>(std::move(name_), std::move(type), std::move(value));
}

ast::Ptr<ast::FunDef> Parser::parse_fun_def_rest(
    ast::Ptr<ast::Expr>&& ret_type, std::string&& name_) {
    debug() << "fun_def_rest\n";
    assert(next().type == tok::ParenL);
    auto params = parse_param_list();
    auto block = parse_block();
    return tree<ast::FunDef>(
        std::move(name_), std::move(ret_type), std::move(params), std::move(block));
}

ast::Ptr<ast::Block> Parser::parse_block() {
    auto node = tree<ast::Block>();
    expect(tok::BraceL);
    expect(tok::BraceR);
    return node;
}

ast::Ptr<ast::ParamList> Parser::parse_param_list() {
    debug() << "param_list\n";
    assert(next().is(tok::ParenL));
    consume();
    auto node = tree<ast::ParamList>();
    while (!next().is(tok::ParenR)) {
        auto type = parse_expr();
        auto name_ = name_str();
        consume();
        ast::Ptr<ast::Expr> default_value = nullptr;
        if (next().is(tok::Assign)) {
            consume();
            default_value = parse_expr();
        }
        node->add(
            tree<ast::Param>(std::move(name_), std::move(type), std::move(default_value)));
        if (next().is(tok::Comma))
            consume();
    }
    expect(tok::ParenR);
    return node;
}

ast::Ptr<ast::Expr> Parser::parse_expr() {
    debug() << "expr\n";
    Op pre = tok::unary_pre_op(next().type);
    if (pre != Op::None) {
        consume();
        return tree<ast::UnaryOp>(pre, parse_expr_climb(ops::prec(pre)));
    }
    return parse_expr_climb(0);
}

ast::Ptr<ast::Expr> Parser::parse_expr_climb(ops::Prec min_prec) {
    debug() << "expr_climb " << (int)min_prec << "\n";
    auto lhs = parse_cast_expr();
    while (true) {
        Op op = next().bin_op();
        if (ops::prec(op) < min_prec)
            break;
        debug() << "op: " << ops::str(op) << "\n";
        switch (op) {
        case Op::FunCall:
            lhs = parse_funcall(std::move(lhs));
            break;
        case Op::ArrayAccess:
            lhs = parse_array_access(std::move(lhs));
            break;
        default:
            consume();
            lhs = tree<ast::BinaryOp>(op, std::move(lhs), parse_expr_climb(ops::right_prec(op)));
        }
    }
    return lhs;
}

ast::Ptr<ast::Expr> Parser::parse_cast_expr() {
    debug() << "cast_expr " << next().type_name() << "\n";
    switch (next().type) {
    case tok::Name:
        return parse_name();
    case tok::Number:
        return parse_number();
    case tok::String:
        return parse_string();
    case tok::ParenL:
        return parse_paren_expr_or_cast();
    default:
        diag("unexpected token");
    }
    return nullptr;
}

ast::Ptr<ast::Expr> Parser::parse_paren_expr_or_cast() {
    debug() << "paren_expr\n";
    assert(next().is(tok::ParenL));
    consume();
    auto inner = parse_expr();
    expect(tok::ParenR);
    return tree<ast::ParenExpr>(std::move(inner));
}

ast::Ptr<ast::FunCall> Parser::parse_funcall(ast::Ptr<ast::Expr>&& obj) {
    debug() << "funcall\n";
    // TODO
    return nullptr;
}

ast::Ptr<ast::ArrayAccess> Parser::parse_array_access(ast::Ptr<ast::Expr>&& array) {
    debug() << "array_access\n";
    // TODO
    return nullptr;
}

ast::Ptr<ast::Name> Parser::parse_name() {
    debug() << "name\n";
    assert(next().is(tok::Name));
    auto node = tree<ast::Name>();
    consume();
    return node;
}

ast::Ptr<ast::Number> Parser::parse_number() {
    debug() << "number: " << value_str() << "\n";
    assert(next().is(tok::Number));
    auto node = tree<ast::Number>();
    consume();
    return node;
}

ast::Ptr<ast::String> Parser::parse_string() {
    debug() << "string: " << value_str() << "\n";
    assert(next().is(tok::String));
    auto node = tree<ast::String>();
    consume();
    return node;
}

ast::Ptr<ast::Expr> Parser::binop_tree(
    Op op, ast::Ptr<ast::Expr>&& lhs, ast::Ptr<ast::Expr>&& rhs) {
    switch (op) {
    case Op::FunCall: {
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

std::string Parser::name_str() const {
    assert(next().is(tok::Name));
    return std::string(_ctx.name_str(next().str_id));
}

std::string Parser::value_str() const {
    assert(next().is(tok::Number) || next().is(tok::String));
    return std::string(_ctx.value_str(next().str_id));
}

} // namespace ulam
