#include "libulam/parser.hpp"
#include "libulam/token.hpp"
#include <cassert>
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

ast::Ptr<ast::Module> Parser::parse_str(const std::string& text) {
    _pp.main_str(text);
    _pp >> _tok;
    return parse_module();
}

void Parser::consume() {
    debug() << _tok.type_name() << " ->\n";
    _pp >> _tok;
    debug() << "-> " << _tok.type_name() << "\n";
}

void Parser::expect(tok::Type type) {
    // TODO
    if (_tok.type != type)
        throw std::invalid_argument(
            std::string("Unexpected token: ") + _tok.type_name() +
            " expecting " + tok::type_name(type));
    consume();
}

bool Parser::eof() { return _tok.is(tok::Eof); }

void Parser::diag(std::string message) {
    throw std::invalid_argument(message); // TEST
}

ast::Ptr<ast::Module> Parser::parse_module() {
    auto node = tree<ast::Module>();
    switch (_tok.type) {
    case tok::Element:
    case tok::Quark:
    case tok::Transient:
        node->add(parse_class_def());
        break;
    default:
        diag("unexpected token in module");
    }
    return node;
}

ast::Ptr<ast::ClassDef> Parser::parse_class_def() {
    assert(
        _tok.is(tok::Element) || _tok.is(tok::Quark) ||
        _tok.is(tok::Transient));
    debug() << "class_def " << _tok.type_name() << "\n";
    auto kind = _tok.class_kind();
    consume();
    auto node = tree<ast::ClassDef>(kind, tok_str());
    consume();
    // TODO: template params, ancestors
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        switch (_tok.type) {
        case tok::Typedef:
            node->body()->add(parse_type_def());
            break;
        case tok::TypeName: {
            auto type = parse_expr();
            if (!_tok.is(tok::Name))
                diag("Unexpected token in class def, expecting name");
            auto name_ = tok_str();
            consume();
            if (_tok.is(tok::ParenL)) {
                node->body()->add(
                    parse_fun_def_rest(std::move(type), std::move(name_)));
            } else {
                node->body()->add(
                    parse_var_def_list_rest(std::move(type), std::move(name_)));
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
    assert(_tok.type == tok::Typedef);
    debug() << "type_def\n";
    consume();
    auto type = parse_expr();
    if (!_tok.is(tok::TypeName))
        diag("Unexpected token in parse_type_def, expecting type name");
    auto alias = tok_str();
    consume();
    expect(tok::Semicol);
    return tree<ast::TypeDef>(std::move(type), std::move(alias));
}

ast::Ptr<ast::VarDefList> Parser::parse_var_def_list_rest(
    ast::Ptr<ast::Expr>&& base_type, std::string&& first_name) {
    debug() << "var_def_list_rest\n";
    auto node = tree<ast::VarDefList>(std::move(base_type));
    std::string name{first_name};
    while (true) {
        ast::Ptr<ast::Expr> expr;
        if (_tok.is(tok::Equal)) {
            consume();
            expr = parse_expr();
        }
        node->add(tree<ast::VarDef>(
            node->base_type(), std::move(name), std::move(expr)));
        if (_tok.in(tok::Semicol, tok::Eof))
            break;
        expect(tok::Comma);
        if (!_tok.is(tok::Name))
            diag(
                "unexpected token in var def list, expecting name after comma");
        name = tok_str();
        consume();
    }
    expect(tok::Semicol);
    return node;
}

ast::Ptr<ast::FunDef>
Parser::parse_fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name) {
    debug() << "fun_def_rest\n";
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
        if (_tok.is(tok::BraceL)) {
            node->add(parse_block());
        } else {
            node->add(parse_stmt());
        }
    expect(tok::BraceR);
    return node;
}

ast::Ptr<ast::Stmt> Parser::parse_stmt() {
    switch (_tok.type) {
    case tok::If:
        break;
    case tok::For:
        break;
    case tok::While:
        break;
    default:
        break;
    }
    return nullptr;
}

ast::Ptr<ast::If> Parser::parse_if() {
    debug() << "if\n";
    expect(tok::If);
    expect(tok::ParenL);
    auto expr = parse_expr();
    expect(tok::ParenR);
    return nullptr;
}

ast::Ptr<ast::For> Parser::parse_for() {
    debug() << "for\n";
    expect(tok::For);
    return nullptr;
}

ast::Ptr<ast::While> Parser::parse_while() {
    debug() << "while\n";
    expect(tok::While);
    return nullptr;
}

ast::Ptr<ast::ParamList> Parser::parse_param_list() {
    debug() << "param_list\n";
    assert(_tok.is(tok::ParenL));
    consume();
    auto node = tree<ast::ParamList>();
    while (!_tok.is(tok::ParenR)) {
        // TODO: define and use parse_param
        auto type = parse_expr();
        auto name_ = tok_str();
        consume();
        ast::Ptr<ast::Expr> default_value = nullptr;
        if (_tok.is(tok::Equal)) {
            consume();
            default_value = parse_expr();
        }
        node->add(tree<ast::Param>(
            std::move(name_), std::move(type), std::move(default_value)));
        if (_tok.is(tok::Comma))
            consume();
    }
    expect(tok::ParenR);
    return node;
}

ast::Ptr<ast::Expr> Parser::parse_expr() {
    debug() << "expr\n";
    Op pre = tok::unary_pre_op(_tok.type);
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
        Op op = _tok.bin_op();
        if (ops::prec(op) < min_prec)
            break;
        debug() << "op: " << ops::str(op) << "\n";
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

ast::Ptr<ast::Expr> Parser::parse_cast_expr() {
    debug() << "cast_expr " << _tok.type_name() << "\n";
    switch (_tok.type) {
    case tok::TypeName:
        return parse_type_name();
    case tok::Name:
        return parse_name();
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
    }
    return nullptr;
}

ast::Ptr<ast::Expr> Parser::parse_paren_expr_or_cast() {
    debug() << "paren_expr\n";
    assert(_tok.is(tok::ParenL));
    // TODO: cast
    consume();
    auto inner = parse_expr();
    expect(tok::ParenR);
    return tree<ast::ParenExpr>(std::move(inner));
}

ast::Ptr<ast::FunCall> Parser::parse_funcall(ast::Ptr<ast::Expr>&& obj) {
    debug() << "funcall\n";
    expect(tok::ParenL);
    auto args = parse_args();
    expect(tok::ParenR);
    return tree<ast::FunCall>(std::move(obj), std::move(args));
}

ast::Ptr<ast::ArgList> Parser::parse_args() {
    debug() << "parse_args\n";
    auto args = tree<ast::ArgList>();
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        // argument
        auto expr = parse_expr();
        if (expr) {
            args->add(std::move(expr));
        } else {
            while (_tok.in(tok::ParenR, tok::Eof))
                ;
            break;
        }
        // ,?
        if (!_tok.is(tok::ParenR)) {
            expect(tok::Comma);
            if (_tok.is(tok::ParenR))
                diag("unexpected ), expecting expr");
        }
    }
    return args;
}

ast::Ptr<ast::ArrayAccess>
Parser::parse_array_access(ast::Ptr<ast::Expr>&& array) {
    debug() << "array_access\n";
    assert(_tok.is(tok::BracketL));
    consume();
    auto index = parse_expr();
    expect(tok::BracketR);
    return tree<ast::ArrayAccess>(std::move(array), std::move(index));
}

ast::Ptr<ast::MemberAccess>
Parser::parse_member_access(ast::Ptr<ast::Expr>&& obj) {
    debug() << "member_access\n";
    assert(_tok.is(tok::Dot));
    consume();
    if (!_tok.in(tok::Name, tok::TypeName)) {
        diag("expecting name or type name");
        return nullptr;
    }
    auto node = tree<ast::MemberAccess>(std::move(obj), tok_str());
    consume();
    return node;
}

ast::Ptr<ast::TypeName> Parser::parse_type_name() {
    debug() << "type_name: " << tok_str() << "\n";
    assert(_tok.is(tok::TypeName));
    auto node = tree<ast::TypeName>(tok_str());
    consume();
    return node;
}

ast::Ptr<ast::Name> Parser::parse_name() {
    debug() << "name: " << tok_str() << "\n";
    assert(_tok.is(tok::Name));
    auto node = tree<ast::Name>(tok_str());
    consume();
    return node;
}

ast::Ptr<ast::BoolLit> Parser::parse_bool_lit() {
    debug() << "bool_lit: " << tok_str() << "\n";
    assert(_tok.in(tok::True, tok::False));
    auto node = tree<ast::BoolLit>(_tok.is(tok::True));
    consume();
    return node;
}

ast::Ptr<ast::NumLit> Parser::parse_num_lit() {
    debug() << "num_lit: " << tok_str() << "\n";
    assert(_tok.is(tok::Number));
    auto node = tree<ast::NumLit>();
    consume();
    return node;
}

ast::Ptr<ast::StrLit> Parser::parse_str_lit() {
    debug() << "bool_lit: " << tok_str() << "\n";
    assert(_tok.is(tok::String));
    auto node = tree<ast::StrLit>();
    consume();
    return node;
}

template <typename N, typename... Args> ast::Ptr<N> Parser::tree(Args&&... args) {
    return ast::make<N>(std::forward<Args>(args)...);
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

std::string Parser::tok_str() {
    assert(_tok.in(tok::Name, tok::TypeName, tok::Number, tok::String));
    return std::string(_sm.str_at(_tok.loc_id, _tok.size));
}

} // namespace ulam
