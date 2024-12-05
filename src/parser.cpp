#include "libulam/ast/nodes/expr.hpp"
#include "libulam/ast/nodes/module.hpp"
#include "libulam/semantic/type/builtin_type_id.hpp"
#include "libulam/semantic/type/class_kind.hpp"
#include <cassert>
#include <libulam/context.hpp>
#include <libulam/diag.hpp>
#include <libulam/parser.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/token.hpp>
#include <src/parser/number.hpp>
#include <src/parser/string.hpp>
#include <string>

#ifdef DEBUG_PARSER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Parser] "
#endif
#include "src/debug.hpp"

// FIXME: panic calls

namespace ulam {

void Parser::parse_file(const std::filesystem::path& path) {
    _pp.main_file(path);
    parse();
}

void Parser::parse_string(const std::string& text) {
    _pp.main_string(text);
    parse();
}

ast::Ptr<ast::Root> Parser::move_ast() {
    ast::Ptr<ast::Root> ast;
    std::swap(ast, _ast);
    return ast;
}

void Parser::parse() {
    _pp >> _tok;
    _ast->add(parse_module());
}

void Parser::consume() { _pp >> _tok; }

void Parser::consume_if(tok::Type type) {
    if (_tok.is(type))
        consume();
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

void Parser::diag(loc_id_t loc_id, std::size_t size, std::string text) {
    _ctx.diag().emit(diag::Error, loc_id, size, text);
}

bool Parser::eof() { return _tok.is(tok::Eof); }

ast::Ptr<ast::ModuleDef> Parser::parse_module() {
    auto node = tree<ast::ModuleDef>();
    while (!_tok.is(tok::Eof)) {
        switch (_tok.type) {
        case tok::Local:
        case tok::Typedef:
        case tok::Constant:
        case tok::TypeIdent:
            parse_module_var_or_type_def(ast::ref(node));
            break;
        case tok::Element:
        case tok::Quark:
        case tok::Transient: {
            auto class_def = parse_class_def();
            if (class_def)
                node->add(std::move(class_def));
        } break;
        default:
            unexpected();
            panic(tok::Semicol);
            consume_if(tok::Semicol);
        }
    }
    return node;
}

void Parser::parse_module_var_or_type_def(ast::Ref<ast::ModuleDef> node) {
    assert(_tok.in(tok::Local, tok::Typedef, tok::Constant, tok::TypeIdent));
    // is marked as local?
    if (_tok.is(tok::Local)) {
        consume();
    } else if (_tok.is(tok::Typedef)) {
        diag("module typedefs must be marked as local");
    } else {
        assert(_tok.in(tok::Constant, tok::TypeIdent));
        diag("module variables must be marked as local");
    }

    if (_tok.is(tok::Typedef)) {
        // typedef
        auto type_def = parse_type_def();
        if (type_def)
            node->add(std::move(type_def));
    } else {
        // vars
        assert(_tok.in(tok::Constant, tok::TypeIdent));
        if (_tok.is(tok::Constant)) {
            consume();
        } else {
            diag("module variables must be constants");
        }
        auto var_def_list = parse_var_def_list(true);
        if (var_def_list)
            node->add(std::move(var_def_list));
    }
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
    auto loc_id = _tok.loc_id;
    consume();
    // name
    ast::Str name;
    if (match(tok::TypeIdent))
        name = tok_ast_str();
    consume();
    // params
    ast::Ptr<ast::ParamList> params{};
    if (_tok.is(tok::ParenL)) {
        params = parse_param_list();
        if (params && kind == ClassKind::Element) {
            diag(params->loc_id(), 1, "element can not be a template");
            params = {};
        }
    }
    // TODO: ancestors
    auto node = tree<ast::ClassDef>(kind, name, std::move(params));
    node->set_loc_id(loc_id);
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
        case tok::BuiltinTypeIdent:
        case tok::TypeIdent: {
            // fun or var def
            // type
            auto type = parse_type_name();
            if (!type) {
                panic(tok::Semicol, tok::BraceL, tok::BraceR);
                consume_if(tok::Semicol);
                break;
            }
            // name
            if (!match(tok::Ident)) {
                panic(tok::Semicol, tok::BraceL, tok::BraceR);
                consume_if(tok::Semicol);
                break;
            }
            auto name = tok_ast_str();
            consume();
            if (_tok.is(tok::ParenL)) {
                auto fun = parse_fun_def_rest(std::move(type), name);
                if (fun)
                    node->body()->add(std::move(fun));
            } else {
                auto vars =
                    parse_var_def_list_rest(std::move(type), name, false);
                if (vars)
                    node->body()->add(std::move(vars));
            }
        } break;
        case tok::Constant: {
            consume();
            auto vars = parse_var_def_list(true);
            if (vars)
                node->body()->add(std::move(vars));
        } break;
        default:
            unexpected();
            panic(tok::Semicol, tok::BraceL, tok::BraceR);
        }
    }
    if (!expect(tok::BraceR)) {
        panic(tok::BraceL, tok::BraceR);
        consume_if(tok::BraceR);
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
    auto name = tok_ast_str();
    consume();
    // ;
    expect(tok::Semicol);
    return tree<ast::TypeDef>(std::move(type), name);
}

ast::Ptr<ast::VarDefList> Parser::parse_var_def_list(bool is_const) {
    // type
    auto type = parse_type_name();
    if (!type) {
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
        consume_if(tok::Semicol);
        return {};
    }
    // name
    if (!match(tok::Ident)) {
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
        return {};
    }
    auto first_name = tok_ast_str();
    consume();
    return parse_var_def_list_rest(std::move(type), first_name, is_const);
}

ast::Ptr<ast::VarDefList> Parser::parse_var_def_list_rest(
    ast::Ptr<ast::TypeName>&& type, ast::Str first_name, bool is_const) {
    auto node = tree<ast::VarDefList>(std::move(type));
    node->set_is_const(is_const);
    // first ident
    auto name = first_name;
    while (true) {
        // value
        ast::Ptr<ast::Expr> expr;
        if (_tok.is(tok::Equal)) {
            consume();
            expr = parse_expr();
        }
        // add var
        node->add(tree<ast::VarDef>(name, std::move(expr)));
        // end of list?
        if (_tok.in(tok::Semicol, tok::Eof))
            break;
        // ,
        // if next is ident, pretend there is a comma
        if (!expect(tok::Comma) && !_tok.is(tok::Ident)) {
            panic(tok::Semicol);
            consume_if(tok::Semicol);
            break;
        }
        // next ident
        if (!match(tok::Ident)) {
            panic(tok::Semicol);
            consume_if(tok::Semicol);
            break;
        }
        name = tok_ast_str();
        consume();
    }
    // ;
    expect(tok::Semicol);
    return node;
}

ast::Ptr<ast::FunDef>
Parser::parse_fun_def_rest(ast::Ptr<ast::TypeName>&& ret_type, ast::Str name) {
    assert(_tok.type == tok::ParenL);
    // params
    auto params = parse_param_list();
    if (!params)
        panic(tok::BraceL, tok::BraceR, tok::Semicol);
    // body
    auto body = ast::make<ast::FunDefBody>();
    parse_as_block(ast::ref(body));
    return tree<ast::FunDef>(
        name, std::move(ret_type), std::move(params), std::move(body));
}

ast::Ptr<ast::Block> Parser::parse_block() {
    auto node = tree<ast::Block>();
    parse_as_block(ast::ref(node));
    return node;
}

void Parser::parse_as_block(ast::Ref<ast::Block> node) {
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        auto stmt = parse_stmt();
        if (stmt)
            node->add(std::move(stmt));
    }
    expect(tok::BraceR);
}

ast::Ptr<ast::Stmt> Parser::parse_stmt() {
    switch (_tok.type) {
    case tok::BuiltinTypeIdent:
    case tok::TypeIdent: {
        auto type = parse_type_name();
        if (_tok.is(tok::Period)) {
            // FIXME
            return parse_type_op_rest(std::move(type));
        } else if (_tok.is(tok::Ident)) {
            auto first_name = tok_ast_str();
            consume();
            return parse_var_def_list_rest(std::move(type), first_name, false);
        } else {
            unexpected();
        }
        expect(tok::Semicol);
        return tree<ast::EmptyStmt>();
    }
    case tok::Constant:
        consume();
        return parse_var_def_list(true);
    case tok::If:
        return parse_if();
    case tok::For:
        return parse_for();
    case tok::While:
        return parse_while();
    case tok::Return:
        return parse_return();
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
        consume_if(tok::ParenR);
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

ast::Ptr<ast::Return> Parser::parse_return() {
    assert(_tok.is(tok::Return));
    consume();
    ast::Ptr<ast::Expr> expr{};
    if (!_tok.is(tok::Semicol)) {
        expr = parse_expr();
        if (!expect(tok::Semicol))
            panic(tok::Semicol, tok::BraceR);
    }
    consume_if(tok::Semicol);
    return tree<ast::Return>(std::move(expr));
}

ast::Ptr<ast::ParamList> Parser::parse_param_list() {
    assert(_tok.is(tok::ParenL));
    // (
    auto node = tree<ast::ParamList>();
    node->set_loc_id(_tok.loc_id);
    consume();
    bool requires_value = false;
    ast::Ref<ast::Param> prev{};
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        // param
        auto param = parse_param(requires_value);
        prev = ast::ref(param);
        if (!param)
            return {};
        if (requires_value) {
            if (!param->has_value()) {
                // pretend we didn't see previous value
                prev->replace_value({});
                requires_value = false;
            }
        } else {
            requires_value = param->has_value();
        }
        // ,
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            consume();
            if (_tok.is(tok::ParenR))
                diag(comma, "trailing comma in parameter list");
        }
    }
    // )
    expect(tok::ParenR);
    return node;
}

ast::Ptr<ast::Param> Parser::parse_param(bool requires_value) {
    bool is_const = false;
    if (_tok.is(tok::Constant)) {
        is_const = true;
        consume();
    }
    // type
    auto type = parse_type_name();
    if (!match(tok::Ident)) {
        panic(tok::Ident, tok::Comma, tok::ParenR);
        if (!_tok.is(tok::Ident))
            return {};
    }
    // name
    auto name = tok_ast_str();
    consume();
    // value
    ast::Ptr<ast::Expr> default_value{};
    if (_tok.is(tok::Equal)) {
        consume();
        default_value = parse_expr();
    } else if (requires_value) {
        // TODO: name location
        diag("missing default value");
    }
    auto node =
        tree<ast::Param>(name, std::move(type), std::move(default_value));
    node->set_is_const(is_const);
    return node;
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
    case tok::BuiltinTypeIdent:
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
    if (_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent)) {
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
    assert(_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent));
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
    if (!_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent)) {
        unexpected();
        return {};
    }
    auto loc_id = _tok.loc_id;
    auto node = tree<ast::TypeName>(parse_type_spec());
    node->set_loc_id(loc_id);
    while (_tok.is(tok::Period)) {
        consume();
        if (!match(tok::TypeIdent))
            return {};
        node->add(parse_type_ident());
    }
    return node;
}

ast::Ptr<ast::TypeSpec> Parser::parse_type_spec() {
    assert(_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent));
    ast::Ptr<ast::TypeIdent> ident{};
    BuiltinTypeId builtin_type_id = NoBuiltinTypeId;
    if (_tok.is(tok::BuiltinTypeIdent)) {
        builtin_type_id = _tok.builtin_type_id();
        consume();
    } else {
        ident = parse_type_ident();
    }
    ast::Ptr<ast::ArgList> args{};
    if (_tok.is(tok::ParenL))
        args = parse_arg_list();
    return (builtin_type_id == NoBuiltinTypeId)
               ? tree<ast::TypeSpec>(std::move(ident), std::move(args))
               : tree<ast::TypeSpec>(builtin_type_id, std::move(args));
}

ast::Ptr<ast::FunCall> Parser::parse_funcall(ast::Ptr<ast::Expr>&& obj) {
    auto args = parse_arg_list();
    return tree<ast::FunCall>(std::move(obj), std::move(args));
}

ast::Ptr<ast::ArgList> Parser::parse_arg_list() {
    assert(_tok.is(tok::ParenL));
    auto loc_id = _tok.loc_id;
    consume();
    auto args = tree<ast::ArgList>();
    args->set_loc_id(loc_id);
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        args->add(parse_expr());
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            consume();
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
    return tree<ast::MemberAccess>(std::move(obj), parse_ident());
}

ast::Ptr<ast::TypeIdent> Parser::parse_type_ident() {
    assert(_tok.is(tok::TypeIdent));
    auto node = tree<ast::TypeIdent>(tok_ast_str());
    node->set_loc_id(_tok.loc_id);
    consume();
    return node;
}

ast::Ptr<ast::Ident> Parser::parse_ident() {
    assert(_tok.is(tok::Ident));
    auto node = tree<ast::Ident>(tok_ast_str());
    node->set_loc_id(_tok.loc_id);
    consume();
    return node;
}

ast::Ptr<ast::BoolLit> Parser::parse_bool_lit() {
    assert(_tok.in(tok::True, tok::False));
    auto node = tree<ast::BoolLit>(_tok.is(tok::True));
    node->set_loc_id(_tok.loc_id);
    consume();
    return node;
}

ast::Ptr<ast::NumLit> Parser::parse_num_lit() {
    auto number = detail::parse_num_str(_ctx.diag(), _tok.loc_id, tok_str());
    consume();
    return tree<ast::NumLit>(std::move(number));
}

ast::Ptr<ast::StrLit> Parser::parse_str_lit() {
    assert(_tok.is(tok::String));
    auto str = detail::parse_str(_ctx.diag(), _tok.loc_id, tok_str());
    consume();
    return tree<ast::StrLit>(std::move(str));
}

template <typename N, typename... Args>
ast::Ptr<N> Parser::tree(Args&&... args) {
    return ast::make<N>(std::forward<Args>(args)...);
}

std::string_view Parser::tok_str() {
    assert(_tok.in(tok::Ident, tok::TypeIdent, tok::Number, tok::String));
    return _ctx.sm().str_at(_tok.loc_id, _tok.size);
}

ast::Str Parser::tok_ast_str() {
    assert(_tok.in(tok::Ident, tok::TypeIdent));
    return {tok_str_id(), _tok.loc_id};
}

str_id_t Parser::tok_str_id() {
    assert(_tok.in(tok::Ident, tok::TypeIdent));
    return _ast->ctx().str_id(tok_str());
}

} // namespace ulam
