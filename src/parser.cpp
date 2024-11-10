#include "libulam/ast/nodes/stmt.hpp"
#include "libulam/diag.hpp"
#include "src/parser/number.hpp"
#include "src/parser/string.hpp"
#include <cassert>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/context.hpp>
#include <libulam/parser.hpp>
#include <libulam/token.hpp>
#include <string>

#ifdef DEBUG_PARSER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Parser] "
#endif
#include "src/debug.hpp"

// FIXME: panic calls

namespace ulam {

ast::Ptr<ast::Module> Parser::parse_file(const std::filesystem::path& path) {
    _pp.main_file(path);
    _pp >> _tok;
    return parse_module();
}

ast::Ptr<ast::Module> Parser::parse_string(const std::string& text) {
    _pp.main_string(text);
    _pp >> _tok;
    return parse_module();
}

void Parser::start_module(ast::Ref<ast::Module> module) {
    assert(!_module);
    _module = module;
}

void Parser::end_module() {
    assert(_module);
    _module = {};
}

void Parser::start_class(ast::Ref<ast::ClassDef> class_def) {
    assert(_module);
    assert(!_class);
    auto type = ulam::make<Class>(class_def, class_def->kind());
    // TODO: add to scope
    // class_def->set_type(ast::ref(type));
    _class = class_def;
}

void Parser::end_class() {
    assert(_class);
    _class = {};
}

template <typename... Ts> void Parser::consume(Ts... types) {
    if constexpr (sizeof...(types) == 0) {
        _pp >> _tok;
    } else if (_tok.in(types...)) {
        _pp >> _tok;
    }
}

bool Parser::match(tok::Type type) {
    if (!_tok.is(type)) {
        auto text = std::string("unexpected ") + _tok.type_name() +
                    ", expecting " + tok::type_name(type);
        _ctx.diag().emit(diag::Error, _tok.loc_id, _tok.size, text);
        return false;
    }
    return true;
}

bool Parser::expect(tok::Type type) {
    bool is_match = match(type);
    if (is_match)
        consume();
    return is_match;
}

template <typename... Ts> void Parser::panic(Ts... stop) {
    while (!_tok.in(stop...) && !eof())
        consume();
}

void Parser::unexpected() {
    auto text = std::string("unexpected ") + _tok.type_name();
    _ctx.diag().emit(diag::Error, _tok.loc_id, _tok.size, text);
}

void Parser::diag(std::string text) {
    _ctx.diag().emit(diag::Error, _tok.loc_id, _tok.size, std::move(text));
}

void Parser::diag(const Token& token, std::string text) {
    _ctx.diag().emit(diag::Error, token.loc_id, token.size, std::move(text));
}

bool Parser::eof() { return _tok.is(tok::Eof); }

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
            unexpected();
            panic(tok::Semicol);
            consume(tok::Semicol);
        }
    }
    return node;
}

ast::Ptr<ast::ClassDef> Parser::parse_class_def() {
    auto node = parse_class_def_head();
    if (node)
        parse_class_def_body(ast::ref(node));
    return node;
}

ast::Ptr<ast::ClassDef> Parser::parse_class_def_head() {
    assert(_tok.in(tok::Element, tok::Quark, tok::Transient));
    // element/quark/transient TODO: union
    auto kind = _tok.class_kind();
    consume();
    // name
    std::string name;
    if (match(tok::TypeIdent))
        name = tok_str();
    consume();
    // params
    ast::Ptr<ast::ParamList> params{};
    if (_tok.is(tok::ParenL))
        params = parse_param_list();
    // TODO: ancestors
    assert(_module);
    auto node = tree<ast::ClassDef>(_module, kind, std::move(name), std::move(params));
    start_class(ast::ref(node));
    return node;
}

void Parser::parse_class_def_body(ast::Ref<ast::ClassDef> node) {
    // body
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        switch (_tok.type) {
        case tok::Typedef: {
            auto type_def = parse_type_def();
            if (type_def)
                node->body()->add(std::move(type_def));
        } break;
        case tok::TypeIdent: {
            // fun or var def
            // type
            auto type = parse_type_name();
            if (!type) {
                panic(tok::Semicol, tok::BraceL, tok::BraceR);
                consume(tok::Semicol);
                break;
            }
            // name
            if (!match(tok::Ident)) {
                panic(tok::Semicol, tok::BraceL, tok::BraceR);
                consume(tok::Semicol);
                break;
            }
            auto name = tok_str();
            consume();
            if (_tok.is(tok::ParenL)) {
                auto fun = parse_fun_def_rest(std::move(type), std::move(name));
                if (fun)
                    node->body()->add(std::move(fun));
            } else {
                auto vars = parse_var_def_list_rest(std::move(type), std::move(name));
                if (vars)
                    node->body()->add(std::move(vars));
            }
        } break;
        default:
            unexpected();
            panic(tok::Semicol, tok::BraceL, tok::BraceR);
        }
    }
    if (!expect(tok::BraceR)) {
        panic(tok::BraceL, tok::BraceR);
        consume(tok::BraceR);
    }
}

ast::Ptr<ast::TypeDef> Parser::parse_type_def() {
    // typedef
    assert(_tok.is(tok::Typedef));
    consume();
    // type
    auto type = parse_type_name();
    if (!type)
        return {};
    // alias
    if (!match(tok::TypeIdent)) {
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
        return {};
    }
    auto alias = tok_str();
    consume();
    // ;
    expect(tok::Semicol);
    return tree<ast::TypeDef>(std::move(type), std::move(alias));
}

ast::Ptr<ast::VarDefList> Parser::parse_var_def_list_rest(
    ast::Ptr<ast::Expr>&& base_type, std::string&& first_name) {
    auto node = tree<ast::VarDefList>(std::move(base_type));
    // first ident
    std::string name{first_name};
    while (true) {
        // value
        ast::Ptr<ast::Expr> expr;
        if (_tok.is(tok::Equal)) {
            consume();
            expr = parse_expr();
        }
        // add var
        node->add(tree<ast::VarDef>(
            node->base_type(), std::move(name), std::move(expr)));
        // end of list?
        if (_tok.in(tok::Semicol, tok::Eof))
            break;
        // ,
        // if next is ident, pretend there is a comma
        if (!expect(tok::Comma) && !_tok.is(tok::Ident)) {
            panic(tok::Semicol);
            consume(tok::Semicol);
            break;
        }
        // next ident
        if (!match(tok::Ident)) {
            panic(tok::Semicol);
            consume(tok::Semicol);
            break;
        }
        name = tok_str();
        consume();
    }
    // ;
    expect(tok::Semicol);
    return node;
}

ast::Ptr<ast::FunDef>
Parser::parse_fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name) {
    assert(_tok.type == tok::ParenL);
    // params
    auto params = parse_param_list();
    if (!params)
        panic(tok::BraceL, tok::BraceR, tok::Semicol);
    // body
    auto block = parse_block();
    if (!block) {
        panic(tok::BraceL, tok::BraceR, tok::Semicol);
        consume(tok::Semicol);
        return {};
    }
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
            unexpected();
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
        if (!expect(tok::Semicol))
            panic(tok::Semicol, tok::BraceL, tok::BraceR);
        if (!expr)
            return tree<ast::EmptyStmt>();
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
    if (!expect(tok::ParenR)) {
        panic(tok::ParenR, tok::BraceL);
        consume(tok::ParenR);
    }
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

ast::Ptr<ast::ParamList> Parser::parse_param_list () {
    assert(_tok.is(tok::ParenL));
    // (
    consume();
    auto node = tree<ast::ParamList>();
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        // param
        node->add(parse_param());
        // ,
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            if (_tok.is(tok::ParenR))
                diag(comma, "trailing comma in parameter list");
        }
    }
    // )
    expect(tok::ParenR);
    return node;
}

ast::Ptr<ast::Param> Parser::parse_param() {
    // type
    auto type = parse_type_name();
    if (!match(tok::Ident)) {
        panic(tok::Ident, tok::Comma, tok::ParenR);
        if (!_tok.is(tok::Ident))
            return {};
    }
    // name
    auto name = tok_str();
    consume();
    // value
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
        unexpected();
        return {};
    }
}

ast::Ptr<ast::Expr> Parser::parse_paren_expr_or_cast() {
    assert(_tok.is(tok::ParenL));
    consume();
    ast::Ptr<ast::Expr> inner{};
    if (_tok.is(tok::TypeIdent)) {
        // cast or type op
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
    if (type_op == TypeOp::None) {
        unexpected();
        return {};
    }
    return tree<ast::TypeOpExpr>(std::move(type), type_op);
}

ast::Ptr<ast::TypeName> Parser::parse_type_name() {
    if (!match(tok::TypeIdent))
        return {};
    auto node = tree<ast::TypeName>(parse_type_spec());
    while (_tok.is(tok::Period)) {
        consume();
        if (!match(tok::TypeIdent))
            return {};
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
        args = parse_arg_list();
    return tree<ast::TypeSpec>(std::move(ident), std::move(args));
}

ast::Ptr<ast::FunCall> Parser::parse_funcall(ast::Ptr<ast::Expr>&& obj) {
    auto args = parse_arg_list();
    return tree<ast::FunCall>(std::move(obj), std::move(args));
}

ast::Ptr<ast::ArgList> Parser::parse_arg_list() {
    assert(_tok.is(tok::ParenL));
    consume();
    auto args = tree<ast::ArgList>();
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        args->add(parse_expr());
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            if (_tok.is(tok::ParenR))
                diag(comma, "trailing comma in argument list");
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
        unexpected();
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
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
        diag("number parse error");
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
    return std::string(_ctx.sm().str_at(_tok.loc_id, _tok.size));
}

} // namespace ulam
