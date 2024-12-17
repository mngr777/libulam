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

Ptr<ast::ModuleDef> Parser::parse_module_file(const std::filesystem::path& path) {
    _pp.main_file(path);
    consume();
    return parse_module();
}

Ptr<ast::ModuleDef> Parser::parse_module_str(std::string text, std::string name) {
    _pp.main_string(text, name);
    consume();
    return parse_module();
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
        _ctx.diag().emit(Diag::Error, _tok.loc_id, _tok.size, text);
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
    while (!_tok.in(tok::Eof, stop...))
        consume();
}

void Parser::unexpected() {
    auto text = std::string("unexpected ") + _tok.type_name();
    _ctx.diag().emit(Diag::Error, _tok.loc_id, _tok.size, text);
}

void Parser::diag(std::string text) {
    _ctx.diag().emit(Diag::Error, _tok.loc_id, _tok.size, std::move(text));
}

void Parser::diag(const Token& token, std::string text) {
    _ctx.diag().emit(Diag::Error, token.loc_id, token.size, std::move(text));
}

void Parser::diag(loc_id_t loc_id, std::size_t size, std::string text) {
    _ctx.diag().emit(Diag::Error, loc_id, size, text);
}

Ptr<ast::ModuleDef> Parser::parse_module() {
    auto node = tree_loc<ast::ModuleDef>(_tok.loc_id);
    while (!_tok.is(tok::Eof)) {
        switch (_tok.type) {
        case tok::Local:
        case tok::Typedef:
        case tok::Constant:
        case tok::TypeIdent:
            parse_module_var_or_type_def(ref(node));
            break;
        case tok::Element:
        case tok::Quark:
        case tok::Transient:
        case tok::Union: {
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

void Parser::parse_module_var_or_type_def(Ref<ast::ModuleDef> node) {
    assert(_tok.in(tok::Local, tok::Typedef, tok::Constant, tok::TypeIdent));
    // is marked as local?
    if (_tok.is(tok::Local)) {
        consume();
    } else if (_tok.is(tok::Typedef)) {
        diag("module typedefs must be marked as local");
    } else {
        assert(_tok.in(tok::Constant, tok::TypeIdent));
        diag("module constants must be marked as local");
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

Ptr<ast::ClassDef> Parser::parse_class_def() {
    auto node = parse_class_def_head();
    if (node)
        parse_class_def_body(ref(node));
    return node;
}

Ptr<ast::ClassDef> Parser::parse_class_def_head() {
    assert(_tok.in(tok::Element, tok::Quark, tok::Transient, tok::Union));

    // element/quark/transient/union
    auto kind = _tok.class_kind();
    auto loc_id = _tok.loc_id;
    consume();

    // name
    ast::Str name;
    if (match(tok::TypeIdent))
        name = tok_ast_str();
    consume();

    // params
    Ptr<ast::ParamList> params{};
    if (_tok.is(tok::ParenL)) {
        params = parse_param_list();
        if (!params)
            panic(tok::Colon, tok::Plus, tok::BraceL);
    }

    // ancestors
    Ptr<ast::TypeNameList> ancestors{};
    if (_tok.in(tok::Plus, tok::Colon))
        ancestors = parse_class_ancestor_list();

    return tree_loc<ast::ClassDef>(
        loc_id, kind, name, std::move(params), std::move(ancestors));
}

Ptr<ast::TypeNameList> Parser::parse_class_ancestor_list() {
    assert(_tok.in(tok::Plus, tok::Colon));
    auto loc_id = _tok.loc_id;
    consume();

    // class Foo : <anc_1> + <anc_2>
    auto node = make<ast::TypeNameList>();
    while (!_tok.in(tok::BraceL, tok::Eof)) {
        // type
        auto type_name = parse_type_name();
        if (!type_name) {
            panic(tok::TypeIdent, tok::BraceL);
            continue;
        }
        node->add(std::move(type_name));

        // +
        if (_tok.is(tok::Plus)) {
            auto plus = _tok;
            consume();
            if (_tok.is(tok::BraceL))
                diag(plus, "trailing plus in ancestor list");
        }
    }
    if (node->child_num() == 0) {
        diag(loc_id, 1, "ancestor list is empty");
        return {};
    }
    return node;
}

void Parser::parse_class_def_body(Ref<ast::ClassDef> node) {
    // body
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        bool ok = false;
        switch (_tok.type) {
        case tok::Typedef: {
            auto type_def = parse_type_def();
            if (type_def) {
                node->body()->add(std::move(type_def));
                ok = true;
            }
        } break;
        case tok::BuiltinTypeIdent:
        case tok::TypeIdent: {
            ok = parse_class_var_or_fun_def(node->body());
        } break;
        case tok::Constant: {
            consume();
            auto vars = parse_var_def_list(true);
            if (vars) {
                node->body()->add(std::move(vars));
                ok = true;
            }
        } break;
        default:
            unexpected();
        }
        if (!ok) {
            panic(tok::Semicol, tok::BraceR);
            consume_if(tok::Semicol);
        }
    }
    if (!expect(tok::BraceR)) {
        panic(tok::BraceR);
        consume_if(tok::BraceR);
    }
}

Ptr<ast::TypeDef> Parser::parse_type_def() {
    assert(_tok.is(tok::Typedef));
    consume();

    // type
    auto type_name = parse_type_name();
    if (!type_name)
        return {};

    // alias expr
    auto type_expr = parse_type_expr();
    if (!type_expr)
        return {};

    // ;
    expect(tok::Semicol);
    return tree<ast::TypeDef>(std::move(type_name), std::move(type_expr));
}

bool Parser::parse_class_var_or_fun_def(Ref<ast::ClassDefBody> node) {
    assert(_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent));

    // type
    Ptr<ast::FunRetType> ret_type{}; // maybe return type
    auto type = parse_type_name();
    if (!type)
        return false;

    // [] (return type only)
    Ptr<ast::ExprList> array_dims{};
    auto brace_loc_id = NoLocId; // to complain if not a fun
    if (_tok.is(tok::BracketL)) {
        brace_loc_id = _tok.loc_id;
        array_dims = parse_array_dims();
        if (!array_dims)
            return false;
        ret_type =
            make<ast::FunRetType>(std::move(type), std::move(array_dims));
        type = {};
    }

    // [&] name
    bool is_ref = parse_is_ref();
    if (!match(tok::Ident))
        return false;
    auto name = tok_ast_str();
    consume();

    if (_tok.is(tok::ParenL)) {
        // fun def (param list)
        if (!ret_type) {
            // the type is a ret type after all
            assert(type);
            ret_type =
                make<ast::FunRetType>(std::move(type), std::move(array_dims));
        }
        ret_type->set_is_ref(is_ref);
        auto fun = parse_fun_def_rest(std::move(ret_type), name);
        if (!fun)
            return false;
        node->add(std::move(fun));

    } else {
        // var def list
        if (!type) {
            // `Type[..]` used instead of maybe `Type var[..]`
            assert(ret_type);
            diag(brace_loc_id, 1, "invalid variable definition syntax");
            return false;
        }
        auto vars =
            parse_var_def_list_rest(std::move(type), false, name, is_ref);
        if (!vars)
            return false;
        node->add(std::move(vars));
    }
    return true;
}

Ptr<ast::VarDefList> Parser::parse_var_def_list(bool is_const) {
    // type
    auto type = parse_type_name();
    if (!type) {
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
        consume_if(tok::Semicol);
        return {};
    }

    // [&] first name
    bool first_is_ref = parse_is_ref();
    if (!match(tok::Ident)) {
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
        return {};
    }
    auto first_name = tok_ast_str();
    consume();
    return parse_var_def_list_rest(
        std::move(type), is_const, first_name, first_is_ref);
}

Ptr<ast::VarDefList> Parser::parse_var_def_list_rest(
    Ptr<ast::TypeName>&& type,
    bool is_const,
    ast::Str first_name,
    bool first_is_ref) {
    auto node = tree<ast::VarDefList>(std::move(type));
    node->set_is_const(is_const);

    // first var
    auto first = parse_var_def_rest(first_name, first_is_ref);
    if (!first) {
        panic(tok::Semicol);
        consume_if(tok::Semicol);
        return {};
    }
    node->add(std::move(first));

    // rest of vars
    while (!_tok.in(tok::Semicol, tok::Eof)) {
        // ,
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            consume();
            if (is_const) {
                diag(comma, "constant lists are not allowed"); // keep going
            } else if (_tok.is(tok::Semicol)) {
                diag(comma, "trailing comma in variable definition list");
                break;
            }
        } else {
            unexpected();
        }

        // var
        auto var_def = parse_var_def();
        if (!var_def) {
            panic(tok::Semicol);
            break;
        }
        node->add(std::move(var_def));
    }
    // ;
    expect(tok::Semicol);
    return node;
}

Ptr<ast::VarDef> Parser::parse_var_def() {
    // [&] name
    bool is_ref = parse_is_ref();
    if (!match(tok::Ident))
        return {};
    auto name = tok_ast_str();
    consume();
    return parse_var_def_rest(name, is_ref);
}

Ptr<ast::VarDef> Parser::parse_var_def_rest(ast::Str name, bool is_ref) {
    // []
    Ptr<ast::ExprList> array_dims{};
    if (_tok.is(tok::BracketL)) {
        array_dims = parse_array_dims();
        if (!array_dims)
            return {};
    }

    // value
    Ptr<ast::Expr> expr;
    if (_tok.is(tok::Equal)) {
        consume();
        expr = parse_expr();
    }

    auto node = tree<ast::VarDef>(name, std::move(array_dims), std::move(expr));
    node->set_is_ref(is_ref);
    return node;
}

Ptr<ast::FunDef>
Parser::parse_fun_def_rest(Ptr<ast::FunRetType>&& ret_type, ast::Str name) {
    assert(_tok.type == tok::ParenL);
    assert(ret_type);

    // params
    auto params = parse_param_list();
    if (!params)
        panic(tok::BraceL, tok::BraceR, tok::Semicol);

    // marked as native?
    // NOTE: actually being native is defined by not having a body
    bool is_native = false;
    if (_tok.is(tok::Native)) {
        is_native = true;
        consume();
    }

    if (!_tok.in(tok::Semicol, tok::BraceL)) {
        unexpected();
        panic(tok::Semicol, tok::BraceL, tok::BraceR);
        if (!_tok.in(tok::Semicol, tok::BraceL))
            return {};
    }

    Ptr<ast::FunDefBody> body;
    if (_tok.is(tok::BraceL)) {
        // body
        if (is_native) {
            diag("function with a body is marked as native");
        }
        body = make<ast::FunDefBody>();
        parse_as_block(ref(body));
    } else {
        // no body
        if (!is_native) {
            diag("function without a body must be marked as native");
        }
        assert(_tok.is(tok::Semicol));
        consume();
    }

    return tree<ast::FunDef>(
        name, std::move(ret_type), std::move(params), std::move(body));
}

Ptr<ast::Block> Parser::parse_block() {
    auto node = tree<ast::Block>();
    parse_as_block(ref(node));
    return node;
}

void Parser::parse_as_block(Ref<ast::Block> node) {
    expect(tok::BraceL);
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        auto stmt = parse_stmt();
        if (stmt)
            node->add(std::move(stmt));
    }
    expect(tok::BraceR);
}

Ptr<ast::Stmt> Parser::parse_stmt() {
    switch (_tok.type) {
    case tok::Typedef:
        return parse_type_def();
    case tok::BuiltinTypeIdent:
    case tok::TypeIdent: {
        auto type = parse_type_name();
        if (_tok.is(tok::Period)) {
            // FIXME: there is also e.g. class var access
            consume();
            auto expr = parse_type_op_rest(std::move(type), {});
            return tree<ast::ExprStmt>(std::move(expr));
        } else if (_tok.in(tok::Ident, tok::Amp)) {
            // [&] first name
            bool first_is_ref = parse_is_ref();
            auto first_name = tok_ast_str();
            consume();
            return parse_var_def_list_rest(
                std::move(type), false, first_name, first_is_ref);
        } else {
            unexpected();
            panic(tok::Semicol);
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
            return {};
        return tree<ast::ExprStmt>(std::move(expr));
    }
}

Ptr<ast::If> Parser::parse_if() {
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
    Ptr<ast::Stmt> else_branch{};
    // else
    if (_tok.is(tok::Else)) {
        consume();
        else_branch = parse_stmt();
    }
    return tree<ast::If>(
        std::move(cond), std::move(if_branch), std::move(else_branch));
}

Ptr<ast::For> Parser::parse_for() {
    assert(_tok.is(tok::For));
    // for (
    consume();
    expect(tok::ParenL);
    // <init>;
    auto init = parse_stmt(); // TODO: enforce appropriate type
    // <init>; <cond>;
    Ptr<ast::Expr> cond{};
    if (!_tok.is(tok::Semicol))
        cond = parse_expr();
    expect(tok::Semicol);
    // <init>; <cond>; <upd>
    Ptr<ast::Expr> upd{};
    if (!_tok.is(tok::ParenR))
        upd = parse_expr();
    // )
    expect(tok::ParenR);
    return tree<ast::For>(
        std::move(init), std::move(cond), std::move(upd), parse_stmt());
}

Ptr<ast::While> Parser::parse_while() {
    assert(_tok.is(tok::While));
    // while (
    consume();
    expect(tok::ParenL);
    auto cond = parse_expr();
    // )
    expect(tok::ParenR);
    return tree<ast::While>(std::move(cond), parse_stmt());
}

Ptr<ast::Return> Parser::parse_return() {
    assert(_tok.is(tok::Return));
    consume();
    Ptr<ast::Expr> expr{};
    if (!_tok.is(tok::Semicol)) {
        expr = parse_expr();
        if (!expect(tok::Semicol))
            panic(tok::Semicol, tok::BraceR);
    }
    consume_if(tok::Semicol);
    return tree<ast::Return>(std::move(expr));
}

Ptr<ast::ParamList> Parser::parse_param_list() {
    assert(_tok.is(tok::ParenL));
    // (
    auto node = tree<ast::ParamList>();
    node->set_loc_id(_tok.loc_id);
    consume();
    bool requires_value = false;
    Ref<ast::Param> prev{};
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        // param
        auto param = parse_param(requires_value);
        if (!param)
            return {};
        if (requires_value) {
            if (!param->has_default_value()) {
                // pretend we didn't see previous value
                assert(prev);
                prev->replace_default_value({});
                requires_value = false;
            }
        } else {
            requires_value = param->has_default_value();
        }
        prev = ref(param);
        node->add(std::move(param));
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

Ptr<ast::Param> Parser::parse_param(bool requires_value) {
    bool is_const = false;
    if (_tok.is(tok::Constant)) {
        is_const = true;
        consume();
    }

    // type
    auto type = parse_type_name();
    if (!type)
        return {};

    // [&] name
    bool is_ref = parse_is_ref();
    if (!match(tok::Ident))
        return {};
    auto name = tok_ast_str();
    consume();

    // array dimensions
    Ptr<ast::ExprList> array_dims{};
    if (_tok.is(tok::BracketL)) {
        array_dims = parse_array_dims();
        if (!array_dims)
            return {};
    }

    // value
    Ptr<ast::Expr> default_value{};
    if (_tok.is(tok::Equal)) {
        consume();
        default_value = parse_expr();
    } else if (requires_value) {
        // TODO: name location
        diag("missing default value");
    }

    auto node = tree<ast::Param>(
        name, std::move(type), std::move(array_dims), std::move(default_value));
    node->set_is_const(is_const);
    node->set_is_ref(is_ref);
    return node;
}

Ptr<ast::Expr> Parser::parse_expr() { return parse_expr_climb(0); }

Ptr<ast::Expr> Parser::parse_expr_climb(ops::Prec min_prec) {
    Ptr<ast::Expr> lhs;

    // unary prefix?
    Op op = tok::unary_pre_op(_tok.type);
    if (op != Op::None) {
        auto loc_id = _tok.loc_id;
        consume();
        lhs =
            tree_loc<ast::UnaryPreOp>(loc_id, op, parse_expr_climb(ops::prec(op)));
    } else {
        lhs = parse_expr_lhs();
    }
    if (!lhs)
        return {};
    return parse_expr_climb_rest(std::move(lhs), min_prec);
}

Ptr<ast::Expr> Parser::parse_expr_climb_rest(Ptr<ast::Expr>&& lhs, ops::Prec min_prec) {
    // binary or suffix
    while (true) {
        // binary?
        Op op = _tok.bin_op();
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
            break;
        case Op::MemberAccess:
            lhs = parse_member_access_or_type_op(std::move(lhs));
            break;
        default:
            consume();
            lhs = tree<ast::BinaryOp>(
                op, std::move(lhs), parse_expr_climb(ops::right_prec(op)));
        }
        if (!lhs)
            return {};
    }
    return std::move(lhs);
}

Ptr<ast::Expr> Parser::parse_expr_lhs() {
    Ptr<ast::Expr> expr{};
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
        panic(tok::Semicol);
        return {};
    }
}

Ptr<ast::Expr> Parser::parse_paren_expr_or_cast() {
    assert(_tok.is(tok::ParenL));
    consume();
    Ptr<ast::Expr> inner{};
    if (_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent)) {
        // cast or type op
        auto type_name = parse_type_name(true /* maybe type op */);
        if (!type_name)
            return {};
        if (_tok.is(tok::ParenR)) {
            // cast
            consume();
            auto expr = parse_expr();
            if (!expr)
                return {};
            return tree<ast::Cast>(std::move(type_name), std::move(expr));
        }
        // type op
        if (_tok.type_op() == TypeOp::None)
            return {};
        inner = parse_type_op_rest(std::move(type_name), {});
        assert(inner);
        inner = parse_expr_climb_rest(std::move(inner), 0);
    } else {
        inner = parse_expr();
    }
    expect(tok::ParenR);
    return tree<ast::ParenExpr>(std::move(inner));
}

Ptr<ast::TypeOpExpr> Parser::parse_type_op() {
    assert(_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent));
    auto type = parse_type_name(true /* maybe type op */);
    assert(type);
    return parse_type_op_rest(std::move(type), {});
}

Ptr<ast::TypeOpExpr>
Parser::parse_type_op_rest(Ptr<ast::TypeName>&& type, Ptr<ast::Expr>&& expr) {
    assert((bool)type != (bool)expr);
    auto type_op = _tok.type_op();
    if (type_op == TypeOp::None) {
        unexpected();
        return {};
    }
    auto loc_id = _tok.loc_id;
    consume();
    return tree_loc<ast::TypeOpExpr>(
        loc_id, type_op, std::move(type), std::move(expr));
}

Ptr<ast::TypeExpr> Parser::parse_type_expr() {
    // &
    bool is_ref{false};
    loc_id_t amp_loc_id{NoLocId};
    if (_tok.is(tok::Amp)) {
        is_ref = true;
        amp_loc_id = _tok.loc_id;
        consume();
    }

    // ident
    auto ident = parse_type_ident();
    if (!ident)
        return {};

    // array dimensions
    Ptr<ast::ExprList> array_dims{};
    if (_tok.is(tok::BracketL)) {
        array_dims = parse_array_dims();
        if (!array_dims)
            return {};
    }

    auto node = tree<ast::TypeExpr>(std::move(ident), std::move(array_dims));
    node->set_is_ref(is_ref);
    node->set_amp_loc_id(amp_loc_id);
    return node;
}

Ptr<ast::ExprList> Parser::parse_array_dims() {
    assert(_tok.is(tok::BracketL));
    auto node = tree_loc<ast::ExprList>(_tok.loc_id);
    consume();
    while (!_tok.is(tok::Eof)) {
        auto expr = parse_expr();
        if (!expect(tok::BracketR) || !expr)
            return {};
        node->add(std::move(expr));
        if (!_tok.is(tok::BracketL))
            return node;
        consume();
    }
    return {};
}

Ptr<ast::TypeName> Parser::parse_type_name(bool maybe_type_op) {
    if (!_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent)) {
        unexpected();
        return {};
    }
    auto loc_id = _tok.loc_id;
    auto node = tree_loc<ast::TypeName>(loc_id, parse_type_spec());
    while (_tok.is(tok::Period)) {
        consume();
        if (maybe_type_op && _tok.type_op() != TypeOp::None)
            break; // NOTE: '.' consumed
        if (!match(tok::TypeIdent))
            return {};
        node->add(parse_type_ident());
    }
    return node;
}

Ptr<ast::TypeSpec> Parser::parse_type_spec() {
    assert(_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent));
    Ptr<ast::TypeIdent> ident{};
    BuiltinTypeId builtin_type_id = NoBuiltinTypeId;
    if (_tok.is(tok::BuiltinTypeIdent)) {
        builtin_type_id = _tok.builtin_type_id();
        consume();
    } else {
        ident = parse_type_ident();
    }
    Ptr<ast::ArgList> args{};
    if (_tok.is(tok::ParenL))
        args = parse_arg_list();
    return (builtin_type_id == NoBuiltinTypeId)
               ? tree<ast::TypeSpec>(std::move(ident), std::move(args))
               : tree<ast::TypeSpec>(builtin_type_id, std::move(args));
}

Ptr<ast::FunCall> Parser::parse_funcall(Ptr<ast::Expr>&& obj) {
    assert(_tok.is(tok::ParenL));
    auto args = parse_arg_list();
    auto loc_id = args->loc_id();
    return tree_loc<ast::FunCall>(loc_id, std::move(obj), std::move(args));
}

Ptr<ast::ArgList> Parser::parse_arg_list() {
    assert(_tok.is(tok::ParenL));
    auto loc_id = _tok.loc_id;
    consume();
    auto args = tree_loc<ast::ArgList>(loc_id);
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

Ptr<ast::ArrayAccess> Parser::parse_array_access(Ptr<ast::Expr>&& array) {
    assert(_tok.is(tok::BracketL));
    auto loc_id = _tok.loc_id;
    consume();
    auto index = parse_expr();
    if (!index)
        return {};
    expect(tok::BracketR);
    return tree_loc<ast::ArrayAccess>(loc_id, std::move(array), std::move(index));
}

Ptr<ast::Expr> Parser::parse_member_access_or_type_op(Ptr<ast::Expr>&& obj) {
    assert(_tok.is(tok::Period));
    auto op_loc_id = _tok.loc_id;
    consume();
    if (_tok.is(tok::Ident))
        return parse_member_access_rest(std::move(obj), op_loc_id);
    auto type_op = _tok.type_op();
    if (type_op != TypeOp::None)
        return parse_type_op_rest({}, std::move(obj));
    unexpected();
    return {};
}

Ptr<ast::MemberAccess>
Parser::parse_member_access_rest(Ptr<ast::Expr>&& obj, loc_id_t op_loc_id) {
    assert(_tok.is(tok::Ident));
    return tree_loc<ast::MemberAccess>(op_loc_id, std::move(obj), parse_ident());
}

Ptr<ast::TypeIdent> Parser::parse_type_ident() {
    assert(_tok.is(tok::TypeIdent));
    auto str = tok_ast_str();
    consume();
    return tree<ast::TypeIdent>(str);

}

Ptr<ast::Ident> Parser::parse_ident() {
    assert(_tok.is(tok::Ident));
    auto str = tok_ast_str();
    consume();
    return tree<ast::Ident>(str);
}

bool Parser::parse_is_ref() {
    bool is_ref = _tok.is(tok::Amp);
    if (is_ref)
        consume();
    return is_ref;
}

Ptr<ast::BoolLit> Parser::parse_bool_lit() {
    assert(_tok.in(tok::True, tok::False));
    bool value = _tok.is(tok::True);
    auto loc_id = _tok.loc_id;
    consume();
    return tree_loc<ast::BoolLit>(loc_id, value);
}

Ptr<ast::NumLit> Parser::parse_num_lit() {
    assert(_tok.is(tok::Number));
    auto number = detail::parse_num_str(_ctx.diag(), _tok.loc_id, tok_str());
    consume();
    return tree<ast::NumLit>(std::move(number));
}

Ptr<ast::StrLit> Parser::parse_str_lit() {
    assert(_tok.is(tok::String));
    auto str = detail::parse_str(_ctx.diag(), _tok.loc_id, tok_str());
    consume();
    return tree<ast::StrLit>(std::move(str));
}

template <typename N, typename... Args> Ptr<N> Parser::tree(Args&&... args) {
    return make<N>(std::forward<Args>(args)...);
}

template <typename N, typename... Args>
Ptr<N> Parser::tree_loc(loc_id_t loc_id, Args&&... args) {
    auto node = make<N>(std::forward<Args>(args)...);
    node->set_loc_id(loc_id);
    return node;
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
    return _str_pool.put(tok_str());
}

} // namespace ulam
