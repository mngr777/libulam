#include "src/parser/number.hpp"
#include "src/parser/string.hpp"
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/parser.hpp>
#include <libulam/token.hpp>
#include <stdexcept> // TMP
#include <string>

#ifdef DEBUG_PARSER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Parser] "
#endif
#include "src/debug.hpp"

namespace ulam {

ast::Ptr<ast::Module> Parser::parse_file(const std::filesystem::path& path) {
    _pp.main_file(path);
    _pp >> _tok;
    return parse_module();
}

ast::Ptr<ast::Module> Parser::parse_string(const std::string& text) {
    _pp.main_str(text);
    _pp >> _tok;
    return parse_module();
}

void Parser::consume() { _pp >> _tok; }

void Parser::expect(tok::Type type) {
    if (_tok.type != type)
        diag(
            std::string("Unexpected token '") + tok_str() + "', expecting " +
            tok::type_name(type));
    consume();
}

bool Parser::eof() { return _tok.is(tok::Eof); }

void Parser::diag(std::string message) {
    throw std::invalid_argument(message); // TMP
}

ast::Ptr<ast::Module> Parser::parse_module() {
    auto node = tree<ast::Module>();
    while (!_tok.is(tok::Eof)) {
        switch (_tok.type) {
        case tok::Element:
        case tok::Quark:
        case tok::Transient:
            node->add(parse_class_def());
            break;
        default:
            diag("unexpected token in module");
        }
    }
    return node;
}

ast::Ptr<ast::ClassDef> Parser::parse_class_def() {
    assert(_tok.in(tok::Element, tok::Quark, tok::Transient));
    // element/quark/transient TODO: union
    auto kind = _tok.class_kind();
    consume();
    // name
    if (!_tok.is(tok::TypeIdent))
        diag("Class name is not a type ident");
    auto name = tok_str();
    consume();
    // params
    ast::Ptr<ast::ParamList> params{};
    if (_tok.is(tok::ParenL))
        params = parse_param_list();
    auto node = tree<ast::ClassDef>(kind, std::move(name), std::move(params));
    // TODO: ancestors
    // body
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        switch (_tok.type) {
        case tok::Typedef:
            node->body()->add(parse_type_def());
            break;
        case tok::TypeIdent: {
            auto type = parse_type_name();
            if (!_tok.is(tok::Ident))
                diag("Unexpected token in class def, expecting name");
            auto name = tok_str();
            consume();
            if (_tok.is(tok::ParenL)) {
                node->body()->add(
                    parse_fun_def_rest(std::move(type), std::move(name)));
            } else {
                node->body()->add(
                    parse_var_def_list_rest(std::move(type), std::move(name)));
            }
        } break;
        default:
            diag("Unexpected token in class def");
        }
    }
    expect(tok::BraceR);
    return node;
}

ast::Ptr<ast::TypeDef> Parser::parse_type_def() {
    assert(_tok.is(tok::Typedef));
    consume();
    auto type = parse_type_name();
    if (!_tok.is(tok::TypeIdent))
        diag("Unexpected token in parse_type_def, expecting type name");
    auto alias = tok_str();
    consume();
    expect(tok::Semicol);
    return tree<ast::TypeDef>(std::move(type), std::move(alias));
}

ast::Ptr<ast::VarDefList> Parser::parse_var_def_list_rest(
    ast::Ptr<ast::Expr>&& base_type, std::string&& first_name) {
    auto node = tree<ast::VarDefList>(std::move(base_type));
    // first ident
    std::string name{first_name};
    while (true) {
        // init value
        ast::Ptr<ast::Expr> expr;
        if (_tok.is(tok::Equal)) {
            consume();
            expr = parse_expr();
        }
        node->add(tree<ast::VarDef>(
            node->base_type(), std::move(name), std::move(expr)));
        // end of list?
        if (_tok.in(tok::Semicol, tok::Eof))
            break;
        // ,
        expect(tok::Comma);
        if (!_tok.is(tok::Ident))
            diag(
                "unexpected token in var def list, expecting name after comma");
        // next ident
        name = tok_str();
        consume();
    }
    expect(tok::Semicol);
    return node;
}

ast::Ptr<ast::FunDef>
Parser::parse_fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name) {
    assert(_tok.type == tok::ParenL);
    auto params = parse_param_list();
    auto block = parse_block();
    return tree<ast::FunDef>(
        std::move(name), std::move(ret_type), std::move(params),
        std::move(block));
}

ast::Ptr<ast::Block> Parser::parse_block() {
    auto node = tree<ast::Block>();
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof))
        node->add(parse_stmt());
    expect(tok::BraceR);
    return node;
}

ast::Ptr<ast::Stmt> Parser::parse_stmt() {
    switch (_tok.type) {
    case tok::TypeIdent: {
        auto type = parse_type_name();
        if (_tok.is(tok::Period)) {
            return parse_type_op_rest(std::move(type));
        } else if (_tok.is(tok::Ident)) {
            auto first_name = tok_str();
            consume();
            return parse_var_def_list_rest(
                std::move(type), std::move(first_name));
        } else {
            diag("Unexpected token after type name");
        }
        expect(tok::Semicol);
        return tree<ast::EmptyStmt>();
    }
    case tok::If:
        return parse_if();
    case tok::For:
        return parse_for();
    case tok::While:
        return parse_while();
    case tok::BraceL:
        return parse_block();
    case tok::Semicol:
        consume();
        return tree<ast::EmptyStmt>();
    default:
        auto expr = parse_expr();
        expect(tok::Semicol);
        return expr;
    }
}

ast::Ptr<ast::If> Parser::parse_if() {
    assert(_tok.is(tok::If));
    // if
    consume();
    // (cond)
    expect(tok::ParenL);
    auto cond = parse_expr();
    expect(tok::ParenR);
    // then
    auto if_branch = parse_stmt();
    ast::Ptr<ast::Stmt> else_branch{};
    // else
    if (_tok.is(tok::Else)) {
        consume();
        else_branch = parse_stmt();
    }
    return tree<ast::If>(
        std::move(cond), std::move(if_branch), std::move(else_branch));
}

ast::Ptr<ast::For> Parser::parse_for() {
    assert(_tok.is(tok::For));
    // for (
    consume();
    expect(tok::ParenL);
    // <init>;
    auto init = parse_stmt(); // TODO: enforce appropriate type
    // <init>; <cond>;
    ast::Ptr<ast::Expr> cond{};
    if (!_tok.is(tok::Semicol))
        cond = parse_expr();
    expect(tok::Semicol);
    // <init>; <cond>; <upd>
    ast::Ptr<ast::Expr> upd{};
    if (!_tok.is(tok::ParenR))
        upd = parse_expr();
    // )
    expect(tok::ParenR);
    return tree<ast::For>(
        std::move(init), std::move(cond), std::move(upd), parse_stmt());
}

ast::Ptr<ast::While> Parser::parse_while() {
    assert(_tok.is(tok::While));
    // while (
    consume();
    expect(tok::ParenL);
    auto cond = parse_expr();
    // )
    expect(tok::ParenR);
    return tree<ast::While>(std::move(cond), parse_stmt());
}

ast::Ptr<ast::ParamList> Parser::parse_param_list() {
    assert(_tok.is(tok::ParenL));
    consume();
    auto node = tree<ast::ParamList>();
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        node->add(parse_param());
        if (_tok.is(tok::Comma)) {
            consume();
            if (_tok.is(tok::ParenR))
                diag("Trailing comma in param list");
        }
    }
    expect(tok::ParenR);
    return node;
}

ast::Ptr<ast::Param> Parser::parse_param() {
    auto type = parse_type_name();
    if (!_tok.is(tok::Ident))
        diag("Expecting identifiers");
    auto name = tok_str();
    consume();
    ast::Ptr<ast::Expr> default_value{};
    if (_tok.is(tok::Equal)) {
        consume();
        default_value = parse_expr();
    }
    return tree<ast::Param>(
        std::move(name), std::move(type), std::move(default_value));
}

ast::Ptr<ast::Expr> Parser::parse_expr() { return parse_expr_climb(0); }

ast::Ptr<ast::Expr> Parser::parse_expr_climb(ops::Prec min_prec) {
    Op op = Op::None;
    // unary prefix
    op = tok::unary_pre_op(_tok.type);
    auto lhs = (op != Op::None) ? consume(),
         tree<ast::UnaryPreOp>(op, parse_expr_climb(ops::prec(op)))
        : parse_expr_lhs();
    if (!lhs)
        return lhs;
    // binary or suffix
    while (true) {
        // binary?
        op = _tok.bin_op();
        if (op == Op::None || ops::prec(op) < min_prec) {
            // suffix?
            op = _tok.unary_post_op();
            if (op == Op::None || ops::prec(op) < min_prec)
                break;
            consume();
            lhs = tree<ast::UnaryPostOp>(op, std::move(lhs));
            continue;
        }
        switch (op) {
        case Op::FunCall:
            lhs = parse_funcall(std::move(lhs));
            break;
        case Op::ArrayAccess:
            lhs = parse_array_access(std::move(lhs));
        case Op::MemberAccess:
            lhs = parse_member_access(std::move(lhs));
            break;
        default:
            consume();
            lhs = tree<ast::BinaryOp>(
                op, std::move(lhs), parse_expr_climb(ops::right_prec(op)));
        }
    }
    return lhs;
}

ast::Ptr<ast::Expr> Parser::parse_expr_lhs() {
    switch (_tok.type) {
    case tok::TypeIdent:
        return parse_type_op();
    case tok::Ident:
        return parse_ident();
    case tok::True:
    case tok::False:
        return parse_bool_lit();
    case tok::Number:
        return parse_num_lit();
    case tok::String:
        return parse_str_lit();
    case tok::ParenL:
        return parse_paren_expr_or_cast();
    default:
        diag("unexpected token, expected expr");
        return {};
    }
}

ast::Ptr<ast::Expr> Parser::parse_paren_expr_or_cast() {
    assert(_tok.is(tok::ParenL));
    consume();
    ast::Ptr<ast::Expr> inner{};
    if (_tok.is(tok::TypeIdent)) {
        // maybe cast
        auto type_name = parse_type_name();
        if (_tok.is(tok::ParenR)) {
            // cast
            consume();
            return tree<ast::Cast>(std::move(type_name), parse_expr());
        }
        // type op
        inner = parse_type_op_rest(std::move(type_name));
    } else {
        inner = parse_expr();
    }
    expect(tok::ParenR);
    return tree<ast::ParenExpr>(std::move(inner));
}

ast::Ptr<ast::TypeOpExpr> Parser::parse_type_op() {
    assert(_tok.is(tok::TypeIdent));
    auto type = parse_type_name();
    assert(type);
    return parse_type_op_rest(std::move(type));
}

ast::Ptr<ast::TypeOpExpr>
Parser::parse_type_op_rest(ast::Ptr<ast::TypeName>&& type) {
    expect(tok::Period);
    auto type_op = _tok.type_op();
    if (type_op == TypeOp::None)
        diag("unexpected token in type op expr");
    return tree<ast::TypeOpExpr>(std::move(type), type_op);
}

ast::Ptr<ast::TypeName> Parser::parse_type_name() {
    if (!_tok.is(tok::TypeIdent))
        diag("unexpected token in type name");
    auto node = tree<ast::TypeName>(parse_type_spec());
    while (_tok.is(tok::Period)) {
        consume();
        if (!_tok.is(tok::TypeIdent))
            diag("Unexpected token in type name");
        node->add(parse_type_ident());
    }
    return node;
}

ast::Ptr<ast::TypeSpec> Parser::parse_type_spec() {
    assert(_tok.is(tok::TypeIdent));
    auto ident = tree<ast::TypeIdent>(tok_str());
    consume();
    ast::Ptr<ast::ArgList> args{};
    if (_tok.is(tok::ParenL))
        args = parse_args();
    return tree<ast::TypeSpec>(std::move(ident), std::move(args));
}

ast::Ptr<ast::FunCall> Parser::parse_funcall(ast::Ptr<ast::Expr>&& obj) {
    auto args = parse_args();
    return tree<ast::FunCall>(std::move(obj), std::move(args));
}

ast::Ptr<ast::ArgList> Parser::parse_args() {
    assert(_tok.is(tok::ParenL));
    consume();
    auto args = tree<ast::ArgList>();
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        args->add(parse_expr());
        if (_tok.is(tok::Comma)) {
            consume();
            if (_tok.is(tok::ParenR))
                diag("Trailing comma in param list");
        }
    }
    expect(tok::ParenR);
    return args;
}

ast::Ptr<ast::ArrayAccess>
Parser::parse_array_access(ast::Ptr<ast::Expr>&& array) {
    assert(_tok.is(tok::BracketL));
    consume();
    auto index = parse_expr();
    expect(tok::BracketR);
    return tree<ast::ArrayAccess>(std::move(array), std::move(index));
}

ast::Ptr<ast::MemberAccess>
Parser::parse_member_access(ast::Ptr<ast::Expr>&& obj) {
    assert(_tok.is(tok::Period));
    consume();
    if (!_tok.in(tok::Ident, tok::TypeIdent)) {
        diag("expecting name or type name");
        return {};
    }
    auto node = tree<ast::MemberAccess>(std::move(obj), tok_str());
    consume();
    return node;
}

ast::Ptr<ast::TypeIdent> Parser::parse_type_ident() {
    assert(_tok.is(tok::TypeIdent));
    auto node = tree<ast::TypeIdent>(tok_str());
    consume();
    return node;
}

ast::Ptr<ast::Ident> Parser::parse_ident() {
    assert(_tok.is(tok::Ident));
    auto node = tree<ast::Ident>(tok_str());
    consume();
    return node;
}

ast::Ptr<ast::BoolLit> Parser::parse_bool_lit() {
    assert(_tok.in(tok::True, tok::False));
    auto node = tree<ast::BoolLit>(_tok.is(tok::True));
    consume();
    return node;
}

ast::Ptr<ast::NumLit> Parser::parse_num_lit() {
    auto res = detail::parse_num_str(tok_str());
    if (res.second.error != detail::ParseNumStatus::Ok)
        diag("Number parse error");
    auto node = tree<ast::NumLit>(std::move(res.first));
    consume();
    return node;
}

ast::Ptr<ast::StrLit> Parser::parse_str_lit() {
    assert(_tok.is(tok::String));
    auto node = tree<ast::StrLit>(detail::parse_str(tok_str()));
    consume();
    return node;
}

template <typename N, typename... Args>
ast::Ptr<N> Parser::tree(Args&&... args) {
    return ast::make<N>(std::forward<Args>(args)...);
}

std::string Parser::tok_str() {
    // assert(_tok.in(tok::Ident, tok::TypeIdent, tok::Number, tok::String));
    return std::string(_sm.str_at(_tok.loc_id, _tok.size));
}

} // namespace ulam
