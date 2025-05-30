#include <cassert>
#include <libulam/context.hpp>
#include <libulam/diag.hpp>
#include <libulam/parser.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/token.hpp>
#include <src/parser/number.hpp>
#include <src/parser/string.hpp>
#include <string>

#define DEBUG_PARSER // TEST
#ifdef DEBUG_PARSER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Parser] "
#endif
#include "src/debug.hpp"

namespace ulam {

Ptr<ast::ModuleDef> Parser::parse_module_file(const Path& path) {
    _pp.main_file(path);
    consume();
    return parse_module(path.stem().string());
}

Ptr<ast::ModuleDef>
Parser::parse_module_str(const std::string& text, const Path& path) {
    _pp.main_string(text, path);
    consume();
    return parse_module(path.stem().string());
}

void Parser::add_str_src(const std::string& text, const Path& path) {
    _pp.add_string(text, path);
}

Ptr<ast::Block> Parser::parse_stmts(std::string text) {
    _pp.main_string(text, ""); // TODO: stream source
    consume();
    auto block = tree<ast::Block>();
    parse_as_block(ref(block), true /* implicit braces */);
    return block;
}

void Parser::consume() {
    if (_back.empty()) {
        _pp >> _tok;
    } else {
        _tok = _back.top();
        _back.pop();
    }
}

void Parser::putback(Token token) {
    _back.push(_tok);
    _tok = token;
}

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
    while (!_tok.in(tok::Eof, stop...)) {
        if (_tok.in(tok::ParenL, tok::BraceL, tok::BracketL)) {
            // consume everything until closing paren/brace/bracket,
            // ignore mismatches
            consume();
            unsigned open = 1;
            while (open > 0 && !_tok.is(tok::Eof)) {
                if (_tok.in(tok::ParenL, tok::BraceL, tok::BracketL)) {
                    ++open;
                } else if (_tok.in(tok::ParenR, tok::BraceR, tok::BracketR)) {
                    --open;
                }
                consume();
            }
        } else {
            consume();
        }
    }
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

Ptr<ast::ModuleDef> Parser::parse_module(const std::string_view name) {
    auto node = tree_loc<ast::ModuleDef>(_tok.loc_id);
    node->set_ulam_version(_pp.version());
    node->set_name_id(_str_pool.put(name));
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
            panic(
                tok::Semicol, tok::Local, tok::Typedef, tok::Constant,
                tok::TypeIdent, tok::Element, tok::Quark, tok::Transient,
                tok::Union);
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
        auto var_def_list = parse_var_def_list(VarIsConst);
        if (var_def_list)
            node->add(std::move(var_def_list));
    }
}

Ptr<ast::ClassDef> Parser::parse_class_def() {
    auto node = parse_class_def_head();
    if (node) {
        _cur_cls_def = ref(node);
        parse_class_def_body(ref(node));
        _cur_cls_def = {};
    }
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

    // class Foo : <anc_1> + <anc_2> + ...
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
        case tok::Local:
        case tok::BuiltinTypeIdent:
        case tok::TypeIdent:
        case tok::Virtual:
        case tok::Override: {
            ok = parse_class_var_or_fun_def(node->body());
        } break;
        case tok::Constant:
        case tok::Parameter: {
            auto flags = VarIsConst;
            if (_tok.is(tok::Parameter))
                flags |= VarIsParameter;
            consume();
            auto vars = parse_var_def_list(flags);
            if (vars) {
                node->body()->add(std::move(vars));
                ok = true;
            }
        } break;
        default:
            unexpected();
        }
        if (!ok) {
            panic(
                tok::Semicol, tok::BraceR, tok::Typedef, tok::Local,
                tok::BuiltinTypeIdent, tok::TypeIdent, tok::Virtual,
                tok::Override, tok::Constant);
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

    Ptr<ast::TypeName> type_name{};
    Ptr<ast::TypeExpr> type_expr{};
    Ptr<ast::TypeDef> node{};

    // type
    type_name = parse_type_name();
    if (!type_name)
        goto Panic;

    // alias expr
    type_expr = parse_type_expr();
    if (!type_expr)
        goto Panic;

    // ;
    expect(tok::Semicol);
    node = tree<ast::TypeDef>(std::move(type_name), std::move(type_expr));
    if (_cur_cls_def)
        node->set_is_in_tpl(_cur_cls_def->is_tpl());
    return node;

Panic:
    panic(tok::Semicol);
    consume_if(tok::Semicol);
    return {};
}

bool Parser::parse_class_var_or_fun_def(Ref<ast::ClassDefBody> node) {
    assert(_tok.in(
        tok::Local, tok::BuiltinTypeIdent, tok::TypeIdent, tok::Virtual,
        tok::Override));

    // virtual/@Override
    bool is_virtual = false;
    bool is_override = false;
    loc_id_t virtual_loc_id = NoLocId;
    loc_id_t override_loc_id = NoLocId;
    while (_tok.in(tok::Virtual, tok::Override)) {
        if (_tok.is(tok::Virtual)) {
            if (is_virtual) {
                diag(_tok, "duplicate `virtual' keyword");
                continue;
            }
            is_virtual = true;
            virtual_loc_id = _tok.loc_id;
        } else {
            assert(_tok.is(tok::Override));
            if (is_override) {
                diag(_tok, "duplicate `@Override' keyword");
                continue;
            }
            is_override = true;
            override_loc_id = NoLocId;
        }
        consume();
    }

    Ptr<ast::FunRetType> ret_type{}; // definitely return type
    Ptr<ast::TypeName> type{};
    Ptr<ast::ExprList> array_dims{};
    bool is_ref = false;
    ast::Str name{};
    tok::Type op_tok_type = tok::Eof;
    bool is_op = false;

    // type
    type = parse_type_name();
    if (!type)
        return false;

    // constructor?
    if (type->is_self() && _tok.is(tok::ParenL)) {
        // check if marked virtual/@Override
        if (is_virtual || is_override) {
            auto loc_id = is_virtual ? virtual_loc_id : override_loc_id;
            std::string_view kw{
                tok::type_str(is_virtual ? tok::Virtual : tok::Override)};
            diag(loc_id, kw.size(), "constructors cannot be virtual");
        }
        // parse
        auto fun = parse_constructor_def_rest(type->first()->ident()->name());
        if (!fun)
            return false;
        node->add(std::move(fun));
        return true;
    }

    // [] (return type only)
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

    // &
    is_ref = parse_is_ref();

    // name/"operator"<op>
    is_op = _tok.is(tok::Operator);
    if (is_op) {
        std::tie(name, op_tok_type) = parse_op_fun_name();
        if (name.empty())
            return false;

    } else if (match(tok::Ident)) {
        name = tok_ast_str();
        consume();
    } else {
        return false;
    }

    if (_tok.is(tok::ParenL)) {
        // fun def (param list)
        if (!ret_type) {
            // the type is a ret type after all
            assert(type);
            ret_type =
                make<ast::FunRetType>(std::move(type), std::move(array_dims));
        }
        ret_type->set_is_ref(is_ref);
        auto fun =
            is_op
                ? parse_op_fun_def_rest(std::move(ret_type), name, op_tok_type)
                : parse_fun_def_rest(std::move(ret_type), name);
        if (!fun)
            return false;
        // virtual/@Override
        fun->set_is_marked_virtual(is_virtual);
        fun->set_is_marked_override(is_override);
        node->add(std::move(fun));

    } else {
        // force error if parsing an operator or virtual/@Override is present
        if (is_op || is_virtual || is_override) {
            expect(tok::ParenL);
            return false;
        }

        // var def list
        if (!type) {
            // `Type[..]` used instead of maybe `Type var[..]`
            assert(ret_type);
            diag(brace_loc_id, 1, "invalid variable definition syntax");
            return false;
        }
        auto vars = parse_var_def_list_rest(std::move(type), name, is_ref);
        if (!vars)
            return false;
        node->add(std::move(vars));
    }
    return true;
}

std::pair<ast::Str, tok::Type> Parser::parse_op_fun_name() {
    auto name_loc_id = _tok.loc_id;
    consume();
    tok::Type op_tok_type = _tok.type;
    if (!_tok.is_overloadable_op()) {
        unexpected();
        return {{}, tok::Eof};
    }
    consume();

    // [] closing bracket
    if (op_tok_type == tok::BracketL && !expect(tok::BracketR))
        return {{}, tok::Eof};

    // make name str
    std::string op_str{tok::type_str(op_tok_type)};
    if (op_tok_type == tok::BracketL)
        op_str = "[]";
    str_id_t name_id = _str_pool.put(std::string{"operator"} + op_str);
    return {{name_id, name_loc_id}, op_tok_type};
}

Ptr<ast::VarDefList> Parser::parse_var_def_list(var_flags_t flags) {
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
        std::move(type), first_name, first_is_ref, flags);
}

Ptr<ast::VarDefList> Parser::parse_var_def_list_rest(
    Ptr<ast::TypeName>&& type,
    ast::Str first_name,
    bool first_is_ref,
    var_flags_t flags) {
    bool is_const = flags & VarIsConst;
    bool is_parameter = flags & VarIsParameter;
    auto node = tree<ast::VarDefList>(std::move(type));
    node->set_is_const(is_const);
    node->set_is_parameter(is_parameter);

    // first var
    auto first = parse_var_def_rest(first_name, first_is_ref, flags);
    if (!first)
        return {};
    node->add(std::move(first));

    // rest of vars
    while (!_tok.in(tok::Semicol, tok::Eof)) {
        // ,
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            consume();
            if (is_const) {
                auto message =
                    std::string{is_parameter ? "parameter" : "constant"} +
                    " lists are not allowed";
                diag(comma, message); // keep going
            } else if (_tok.is(tok::Semicol)) {
                diag(comma, "trailing comma in variable definition list");
                break;
            }
        } else {
            unexpected();
        }

        // var
        auto var_def = parse_var_def(flags);
        if (!var_def)
            break;
        node->add(std::move(var_def));
    }
    // ;
    expect(tok::Semicol);
    return node;
}

Ptr<ast::VarDef> Parser::parse_var_def(var_flags_t flags) {
    // [&] name
    bool is_ref = parse_is_ref();
    if (!match(tok::Ident))
        return {};
    auto name = tok_ast_str();
    consume();
    return parse_var_def_rest(name, is_ref, flags);
}

Ptr<ast::VarDef>
Parser::parse_var_def_rest(ast::Str name, bool is_ref, var_flags_t flags) {
    // []
    Ptr<ast::ExprList> array_dims{};
    if (_tok.is(tok::BracketL)) {
        array_dims = parse_array_dims(true);
        if (!array_dims)
            return {};
    }

    // value
    auto [init, ok] = parse_init(false, ref(array_dims), flags);
    if (!ok)
        return {};

    auto node = tree_loc<ast::VarDef>(
        name.loc_id(), name, std::move(array_dims), std::move(init));
    node->set_is_ref(is_ref);
    node->set_is_const(flags & VarIsConst);
    node->set_is_parameter(flags & VarIsParameter);
    if (_cur_cls_def)
        node->set_is_in_tpl(_cur_cls_def->is_tpl());
    return node;
}

Ptr<ast::FunDef>
Parser::parse_fun_def_rest(Ptr<ast::FunRetType>&& ret_type, ast::Str name) {
    assert(_tok.type == tok::ParenL);

    bool ok = true;

    // params
    auto params = parse_param_list(true);
    if (!params) {
        panic(tok::BraceL, tok::Native, tok::Semicol);
        ok = false;
    }

    // marked as native?
    bool is_native = false;
    if (_tok.is(tok::Native)) {
        is_native = true;
        consume();
    }

    if (!_tok.in(tok::Semicol, tok::BraceL)) {
        unexpected();
        return {};
    }

    auto fun = tree<ast::FunDef>(
        name, std::move(ret_type), std::move(params), Ptr<ast::FunDefBody>{});

    // body
    Ptr<ast::FunDefBody> body;
    if (_tok.is(tok::BraceL)) {
        // body
        if (is_native) {
            diag("function with a body is marked as native");
            ok = false;
        }
        _cur_fun_def = ref(fun);
        body = make<ast::FunDefBody>();
        parse_as_block(ref(body));
        _cur_fun_def = {};
    } else {
        // no body (must be either native or pure virtual)
        assert(_tok.is(tok::Semicol));
        consume();
    }

    if (params && params->has_ellipsis() && !is_native) {
        diag(
            params->ellipsis_loc_id(), 1,
            "only native functions can have ellipsis parameter");
        params->set_ellipsis_loc_id(NoLocId);
        ok = false;
    }

    // ignore functions with syntax errors
    if (!ok)
        return {};

    if (body)
        fun->replace_body(std::move(body));

    // handle operator aliases, e.g. `aref`
    Op op = ops::fun_name_op(_str_pool.get(fun->name_id()));
    if (op != Op::None) {
        fun->set_is_op_alias(true);
        fun->set_op(op);
    }
    fun->set_is_native(is_native);
    if (_cur_cls_def)
        fun->set_is_in_tpl(_cur_cls_def->is_tpl());
    return fun;
}

Ptr<ast::FunDef> Parser::parse_op_fun_def_rest(
    Ptr<ast::FunRetType>&& ret_type, ast::Str name, tok::Type op_tok_type) {
    assert(tok::is_overloadable_op(op_tok_type));

    auto fun = parse_fun_def_rest(std::move(ret_type), name);
    if (!fun || fun->is_op())
        return fun;

    Op op = Op::None;
    if (fun->param_num() == 0) {
        op = tok::unary_pre_op(op_tok_type);
    } else {
        op = tok::bin_op(op_tok_type);
        if (op == Op::None)
            op = tok::unary_post_op(op_tok_type);
        // NOTE: parameter num/types to be checked later
    }
    assert(op != Op::None);
    fun->set_op(op);
    return fun;
}

Ptr<ast::FunDef> Parser::parse_constructor_def_rest(ast::Str name) {
    auto fun = parse_fun_def_rest({}, name);
    if (!fun)
        return {};
    fun->set_is_constructor(true);

    // cannot be default
    if (fun->param_num() == 0) {
        diag(
            fun->params()->loc_id(), 1, "default constructors are not allowed");
        return {};
    }
    return fun;
}

Ptr<ast::ParamList> Parser::parse_param_list(bool allow_ellipsis) {
    assert(_tok.is(tok::ParenL));
    // (
    auto node = tree_loc<ast::ParamList>(_tok.loc_id);
    consume();
    bool requires_value = false;
    Ref<ast::Param> prev{};
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        // ...
        if (_tok.is(tok::Ellipsis)) {
            auto loc_id = _tok.loc_id;
            consume();
            if (allow_ellipsis) {
                if (node->has_ellipsis())
                    diag(_tok, "duplicate ellipsis");
                node->set_ellipsis_loc_id(loc_id);
                if (!_tok.is(tok::ParenR)) {
                    diag(loc_id, 1, "ellipsis must be the last parameter");
                    goto Panic;
                }
            } else {
                diag(loc_id, 1, "ellipsis is not allowed in this context");
                goto Panic;
            }
            continue;
        }

        // param
        auto param = parse_param(requires_value);
        if (!param)
            goto Panic;
        if (requires_value) {
            if (!param->has_init()) {
                // pretend we didn't see previous value
                assert(prev);
                prev->replace_init({});
                requires_value = false;
            }
        } else {
            requires_value = param->has_init();
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
    if (!expect(tok::ParenR))
        goto Panic;
    return node;

Panic:
    panic(tok::ParenR);
    consume_if(tok::ParenR);
    return {};
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
        array_dims = parse_array_dims(true);
        if (!array_dims)
            return {};
    }

    // value
    auto [init, ok] = parse_init(requires_value, ref(array_dims));
    if (!ok)
        return {};

    auto node = tree_loc<ast::Param>(
        name.loc_id(), name, std::move(type), std::move(array_dims),
        std::move(init));
    node->set_is_const(is_const);
    node->set_is_ref(is_ref);
    return node;
}

// NOTE: starts at `=' or `(' for constructor call
std::pair<Ptr<ast::InitValue>, bool> Parser::parse_init(
    bool is_required, Ref<ast::ExprList> array_dims, var_flags_t flags) {

    bool is_array = (bool)array_dims;
    bool allow_constr = !is_array && flags & VarAllowConstructorInit;

    bool has_equal = _tok.is(tok::Equal);
    if (has_equal) {
        allow_constr = false; // not constructor init syntax
        consume();
    }
    auto loc_id = _tok.loc_id;

    auto make_list_or_map = [&](auto&& list_map_v) {
        auto val = list_map_v.accept(
            [&](Ptr<ast::InitList>& list) {
                if (!list)
                    return Ptr<ast::InitValue>{};
                return tree_loc<ast::InitValue>(loc_id, std::move(list));
            },
            [&](Ptr<ast::InitMap>& map) {
                if (!map)
                    return Ptr<ast::InitValue>{};
                return tree_loc<ast::InitValue>(loc_id, std::move(map));
            });
        auto ok = (bool)val;
        return std::pair{std::move(val), ok};
    };

    if (allow_constr && _tok.is(tok::ParenL)) {
        // Type var(...)
        return make_list_or_map(parse_init_list_or_map());

    } else if (has_equal) {
        if (_tok.is(tok::BraceL)) {
            // Type var = {...}
            // NOTE: arrays cannot have init map, to be checked later
            return make_list_or_map(parse_init_list_or_map());

        } else {
            // Type var = <value>;
            auto expr = parse_expr();
            if (!expr)
                return {};
            return {tree_loc<ast::InitValue>(loc_id, std::move(expr)), true};
        }
    } else if (is_array && array_dims->has_empty()) {
        diag("array value is required to fill array dimensions");
        return {Ptr<ast::InitValue>{}, false};
    } else if (is_required) {
        diag("missing init value");
        return {Ptr<ast::InitValue>{}, false};
    }
    return {Ptr<ast::InitValue>{}, true};
}

detail::Variant<Ptr<ast::InitList>, Ptr<ast::InitMap>>
Parser::parse_init_list_or_map() {
    assert(_tok.in(tok::BraceL, tok::ParenL));
    using Res = decltype(parse_init_list_or_map());

    if (_tok.is(tok::ParenL))
        return Res{parse_init_list()};

    bool is_map = false;
    {
        auto brace_tok = _tok;
        consume();
        is_map = _tok.is(tok::Period);
        putback(brace_tok);
    }
    return is_map ? Res{parse_init_map()} : Res{parse_init_list()};
}

Ptr<ast::InitList> Parser::parse_init_list() {
    assert(_tok.in(tok::BraceL, tok::ParenL));
    // { or (
    auto node = tree_loc<ast::InitList>(_tok.loc_id);
    node->set_is_constr_call(_tok.is(tok::ParenL));
    tok::Type closing = _tok.is(tok::BraceL) ? tok::BraceR : tok::ParenR;
    consume();
    while (!_tok.is(closing)) {
        if (_tok.is(tok::BraceL)) {
            // list or map
            auto list_map_v = parse_init_list_or_map();
            bool ok = list_map_v.accept([&](auto&& ptr) { return (bool)ptr; });
            if (!ok)
                goto Panic;
            list_map_v.accept(
                [&](Ptr<ast::InitList>& list) { node->add(std::move(list)); },
                [&](Ptr<ast::InitMap>& map) { node->add(std::move(map)); });

        } else {
            // expr
            auto expr = parse_expr(ExprStopAtComma);
            if (!expr)
                goto Panic;
            node->add(std::move(expr));
        }

        // ,
        if (!_tok.is(closing) && !expect(tok::Comma))
            goto Panic;
    }
    // } or )
    if (!expect(closing))
        goto Panic;
    return node;

Panic:
    panic(closing);
    consume_if(closing);
    return {};
}

Ptr<ast::InitMap> Parser::parse_init_map() {
    assert(_tok.is(tok::BraceL));
    // {
    auto node = tree_loc<ast::InitMap>(_tok.loc_id);
    consume();
    while (!_tok.is(tok::BraceR)) {
        // .<label>
        if (!expect(tok::Period))
            goto Panic;
        if (!match(tok::Ident))
            goto Panic;
        auto name_id = tok_str_id();
        // duplicate?
        if (node->has(name_id)) {
            diag("duplicate label");
            goto Panic;
        }
        consume();

        // =
        if (!expect(tok::Equal))
            goto Panic;

        if (_tok.is(tok::BraceL)) {
            // list or map
            auto list_map_v = parse_init_list_or_map();
            bool ok = list_map_v.accept([&](auto&& ptr) { return (bool)ptr; });
            if (!ok)
                goto Panic;
            list_map_v.accept(
                [&](Ptr<ast::InitList>& list) {
                    node->add(name_id, std::move(list));
                },
                [&](Ptr<ast::InitMap>& map) {
                    node->add(name_id, std::move(map));
                });
        } else {
            // expr
            auto expr = parse_expr(ExprStopAtComma);
            if (!expr)
                goto Panic;
            node->add(name_id, std::move(expr));
        }

        // ,
        if (!_tok.is(tok::BraceR) && !expect(tok::Comma))
            goto Panic;
    }
    if (!expect(tok::BraceR))
        goto Panic;

    return node;

Panic:
    panic(tok::BraceR);
    consume_if(tok::BraceR);
    return {};
}

Ptr<ast::Block> Parser::parse_block() {
    auto node = tree<ast::Block>();
    parse_as_block(ref(node));
    return node;
}

void Parser::parse_as_block(Ref<ast::Block> node, bool implicit_braces) {
    // {
    if (!implicit_braces) {
        if (!expect(tok::BraceL))
            return;
    }
    // stmts
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        auto stmt = parse_stmt();
        if (stmt)
            node->add(std::move(stmt));
    }
    // }
    if (!implicit_braces) {
        if (!expect(tok::BraceR)) {
            panic(tok::BraceR);
            consume_if(tok::BraceR);
        }
    }
}

Ptr<ast::Stmt> Parser::parse_stmt() {
    switch (_tok.type) {
    case tok::Typedef:
        return parse_type_def();
    case tok::Local:
        return parse_stmt_local();
    case tok::BuiltinTypeIdent:
    case tok::TypeIdent:
        return parse_stmt_type();
    case tok::Constant:
        consume();
        return parse_var_def_list(VarIsConst | VarAllowConstructorInit);
    case tok::If:
        return parse_if_or_as_if();
    case tok::For:
        return parse_for();
    case tok::While:
        return parse_while();
    case tok::Which:
        return parse_which();
    case tok::Return:
        return parse_return();
    case tok::Break:
        return parse_break();
    case tok::Continue:
        return parse_continue();
    case tok::BraceL:
        return parse_block();
    case tok::Semicol:
        consume();
        return tree<ast::EmptyStmt>();
    default:
        return parse_expr_stmt();
    }
}

Ptr<ast::Stmt> Parser::parse_stmt_local() {
    assert(_tok.is(tok::Local));
    // local
    auto local = _tok;
    consume();

    // .
    if (!match(tok::Period))
        return {};
    auto period = _tok;
    consume();

    // Type/ident
    switch (_tok.type) {
    case tok::BuiltinTypeIdent:
    case tok::TypeIdent:
        putback(period);
        putback(local);
        return parse_stmt_type();
    case tok::Ident:
        putback(period);
        putback(local);
        return parse_expr_stmt();
    default:
        unexpected();
        return {};
    }
}

Ptr<ast::Stmt> Parser::parse_stmt_type() {
    auto type_name = parse_type_name(true);
    if (_tok.is(tok::Period)) {
        consume();
        Ptr<ast::Expr> expr{};
        if (_tok.is(tok::Ident)) {
            // Type.<ident>
            expr = parse_class_const_access_rest(std::move(type_name));
        } else if (_tok.is_type_op()) {
            // Type.<type-op>
            expr = parse_type_op_rest(std::move(type_name), {});
        } else {
            unexpected();
            return {};
        }
        return tree<ast::ExprStmt>(std::move(expr));
    } else if (_tok.in(tok::Ident, tok::Amp)) {
        // [&] first name
        bool first_is_ref = parse_is_ref();
        auto first_name = tok_ast_str();
        consume();
        return parse_var_def_list_rest(
            std::move(type_name), first_name, first_is_ref,
            VarAllowConstructorInit);
    } else {
        unexpected();
        return {};
    }
    expect(tok::Semicol);
    return tree<ast::EmptyStmt>();
}

Ptr<ast::ExprStmt> Parser::parse_expr_stmt() {
    auto expr = parse_expr();
    if (!expr)
        return {};
    if (!expect(tok::Semicol)) {
        panic(tok::Semicol);
        consume_if(tok::Semicol);
    }
    return tree<ast::ExprStmt>(std::move(expr));
}

Ptr<ast::Stmt> Parser::parse_if_or_as_if() {
    assert(_tok.is(tok::If));
    // if
    auto loc_id = _tok.loc_id;
    consume();
    // (expr
    if (!expect(tok::ParenL))
        return {};
    bool ok = true;
    Ptr<ast::Expr> expr{};
    Ptr<ast::Ident> ident{};
    Ptr<ast::TypeName> type{};
    if (_tok.is(tok::Ident)) {
        ident = parse_ident(IdentAllowSelfOrSuper);
        if (_tok.is(tok::As)) {
            consume();
            // if (ident as Type
            type = parse_type_name();
        } else {
            // if (expr
            expr = parse_expr_climb_rest(std::move(ident), 0);
        }
    } else {
        expr = parse_expr();
    }
    ok = ok && (expr || type);
    // )
    if (!expect(tok::ParenR)) {
        panic(tok::ParenR);
        consume_if(tok::ParenR);
    }
    // then
    auto if_branch = parse_stmt();
    ok = ok && if_branch;
    Ptr<ast::Stmt> else_branch{};
    // else
    if (_tok.is(tok::Else)) {
        consume();
        else_branch = parse_stmt();
        ok = ok && else_branch;
    }

    if (!ok)
        return {};

    if (type) {
        assert(ident);
        return tree_loc<ast::IfAs>(
            loc_id, std::move(ident), std::move(type), std::move(if_branch),
            std::move(else_branch));
    }
    return tree_loc<ast::If>(
        loc_id, std::move(expr), std::move(if_branch), std::move(else_branch));
}

Ptr<ast::For> Parser::parse_for() {
    assert(_tok.is(tok::For));

    // for (
    consume();
    if (!expect(tok::ParenL))
        return {};

    bool ok = true;

    // <init>;
    auto init = parse_stmt(); // TODO: enforce appropriate type
    ok = ok && init;

    // <init>; <cond>;
    Ptr<ast::Expr> cond{};
    if (!_tok.is(tok::Semicol)) {
        cond = parse_expr();
        ok = ok && cond;
    }
    ok = expect(tok::Semicol) && ok;

    // <init>; <cond>; <upd>
    Ptr<ast::Expr> upd{};
    if (!_tok.is(tok::ParenR)) {
        upd = parse_expr();
        ok = ok && upd;
    }

    // )
    if (!expect(tok::ParenR)) {
        ok = false;
        panic(tok::ParenR);
        consume_if(tok::ParenR);
    }

    // body
    auto body = parse_stmt();
    ok = ok && body;

    if (!ok)
        return {};

    return tree<ast::For>(
        std::move(init), std::move(cond), std::move(upd), std::move(body));
}

Ptr<ast::While> Parser::parse_while() {
    assert(_tok.is(tok::While));
    // while
    consume();
    bool ok = true;

    // (cond
    if (!expect(tok::ParenL))
        return {};
    auto cond = parse_expr();
    ok = ok && cond;

    // )
    if (!expect(tok::ParenR)) {
        ok = false;
        panic(tok::ParenR);
        consume_if(tok::ParenR);
    }

    // body
    auto body = parse_stmt();
    ok = ok && body;

    if (!ok)
        return {};

    return tree<ast::While>(std::move(cond), std::move(body));
}

Ptr<ast::Which> Parser::parse_which() {
    assert(_tok.is(tok::Which));
    // which
    auto loc_id = _tok.loc_id;
    consume();
    bool ok = true;

    // (expr
    if (!expect(tok::ParenL))
        return {};
    auto expr = parse_expr();
    ok = ok && expr;

    // )
    if (!expect(tok::ParenR)) {
        ok = false;
        panic(tok::ParenR);
        consume_if(tok::ParenR);
    }

    Ptr<ast::Which> node{};
    if (ok)
        node = tree_loc<ast::Which>(loc_id, std::move(expr));

    // case list
    // {
    if (!expect(tok::BraceL))
        return {};
    bool has_default = false;
    while (!_tok.in(tok::BraceR, tok::Eof)) {
        // parse case
        Ptr<ast::WhichCase> case_{};
        if (_tok.in(tok::Case, tok::Otherwise))
            case_ = parse_which_case();

        // success?
        if (!case_) {
            panic(tok::BraceR);
            consume_if(tok::BraceR);
            return {};
        }

        // duplicate `otherwise'?
        if (case_->is_default()) {
            if (has_default)
                diag(case_->loc_id(), 1, "multiple default cases");
            has_default = true;
        }

        if (node) {
            node->add(std::move(case_));
        }
    }
    // }
    if (!expect(tok::BraceR)) {
        assert(_tok.is(tok::Eof));
        return {};
    }
    return node;
}

Ptr<ast::WhichCase> Parser::parse_which_case() {
    assert(_tok.in(tok::Case, tok::Otherwise));

    bool is_default = _tok.is(tok::Otherwise);

    // case/otherwise
    auto loc_id = _tok.loc_id;
    consume();

    // expr
    Ptr<ast::Expr> expr{};
    if (!is_default) {
        expr = parse_expr();
        if (!expr)
            return {};
    }

    // :
    if (!expect(tok::Colon))
        return {};

    // stmt
    Ptr<ast::Stmt> stmt{};
    if (!_tok.in(tok::Case, tok::Otherwise)) {
        stmt = parse_stmt();
        if (!stmt)
            return {};
    }

    return tree_loc<ast::WhichCase>(loc_id, std::move(expr), std::move(stmt));
}

Ptr<ast::Return> Parser::parse_return() {
    assert(_tok.is(tok::Return));
    auto loc_id = _tok.loc_id;
    consume();
    bool ok = true;

    // expr
    Ptr<ast::Expr> expr{};
    if (!_tok.is(tok::Semicol)) {
        expr = parse_expr();
        ok = ok && expr;
    }

    // ;
    expect(tok::Semicol);

    if (!ok)
        return {};

    return tree_loc<ast::Return>(loc_id, std::move(expr));
}

Ptr<ast::Break> Parser::parse_break() {
    assert(_tok.is(tok::Break));
    auto loc_id = _tok.loc_id;
    consume();
    expect(tok::Semicol);
    return tree_loc<ast::Break>(loc_id);
}

Ptr<ast::Continue> Parser::parse_continue() {
    assert(_tok.is(tok::Continue));
    auto loc_id = _tok.loc_id;
    consume();
    expect(tok::Semicol);
    return tree_loc<ast::Continue>(loc_id);
}

Ptr<ast::Expr> Parser::parse_expr(expr_flags_t flags) {
    return parse_expr_climb(0, flags);
}

Ptr<ast::Expr>
Parser::parse_expr_climb(ops::Prec min_prec, expr_flags_t flags) {
    Ptr<ast::Expr> lhs;

    // unary prefix?
    Op op = tok::unary_pre_op(_tok.type);
    if (op != Op::None) {
        auto loc_id = _tok.loc_id;
        consume();
        lhs = tree_loc<ast::UnaryOp>(
            loc_id, op, parse_expr_climb(ops::prec(op), flags));
    } else {
        lhs = parse_expr_lhs();
    }
    if (!lhs)
        return {};
    return parse_expr_climb_rest(std::move(lhs), min_prec, flags);
}

Ptr<ast::Expr> Parser::parse_expr_climb_rest(
    Ptr<ast::Expr>&& lhs, ops::Prec min_prec, expr_flags_t flags) {
    // binary or suffix
    while (true) {
        // comma?
        if (_tok.is(tok::Comma) && flags & ExprStopAtComma)
            break;

        // binary?
        Op op = _tok.bin_op();
        auto op_loc_id = _tok.loc_id;
        if (op == Op::None || ops::prec(op) < min_prec) {
            if (op != Op::None)
                break;
            // suffix?
            op = _tok.unary_post_op();
            if (op == Op::None || ops::prec(op) < min_prec)
                break;
            consume();
            Ptr<ast::TypeName> type_name{};
            if (op == Op::Is) {
                type_name = parse_type_name();
                if (!type_name)
                    return {};
            }
            lhs = tree_loc<ast::UnaryOp>(
                op_loc_id, op, std::move(lhs), std::move(type_name));
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
            lhs = parse_member_access_type_op_or_op_call(std::move(lhs));
            break;
        case Op::Ternary:
            lhs = parse_ternary(std::move(lhs));
            break;
        default:
            consume();
            lhs = tree_loc<ast::BinaryOp>(
                op_loc_id, op, std::move(lhs),
                parse_expr_climb(ops::right_prec(op)));
        }
        if (!lhs)
            return {};
    }
    return std::move(lhs);
}

Ptr<ast::Expr> Parser::parse_expr_lhs(expr_flags_t flags) {
    Ptr<ast::Expr> expr{};
    switch (_tok.type) {
    case tok::Local:
        return parse_expr_lhs_local(flags);
    case tok::BuiltinTypeIdent:
    case tok::TypeIdent:
        return parse_class_const_access_or_type_op();
    case tok::Ident:
        return parse_ident(IdentAllowSelfOrSuper | IdentAllowLocal);
    case tok::Operator:
        return parse_op_call();
    case tok::True:
    case tok::False:
        return parse_bool_lit();
    case tok::Number:
        return parse_num_lit();
    case tok::Char:
        return parse_char_lit();
    case tok::String:
        return parse_str_lit();
    case tok::ParenL:
        return parse_paren_expr_or_cast(flags);
    default:
        unexpected();
        panic(tok::Semicol);
        return {};
    }
}

Ptr<ast::Expr> Parser::parse_expr_lhs_local(expr_flags_t flags) {
    assert(_tok.is(tok::Local));
    // local
    auto local = _tok;
    consume();

    // .
    if (!match(tok::Period))
        return {};
    auto period = _tok;
    consume();

    // Type/ident
    switch (_tok.type) {
    case tok::BuiltinTypeIdent:
    case tok::TypeIdent:
        putback(period);
        putback(local);
        return parse_class_const_access_or_type_op();
    case tok::Ident:
        putback(period);
        putback(local);
        return parse_ident(IdentAllowSelfOrSuper | IdentAllowLocal);
    default:
        unexpected();
        return {};
    }
}

Ptr<ast::Expr> Parser::parse_paren_expr_or_cast(expr_flags_t flags) {
    assert(_tok.is(tok::ParenL));
    auto loc_id = _tok.loc_id;
    consume();
    Ptr<ast::Expr> inner{};
    if (_tok.in(tok::BuiltinTypeIdent, tok::TypeIdent)) {
        // (Type) | (Type.<type_op>
        auto full_type_name = parse_full_type_name(true);
        if (!full_type_name)
            goto Panic;
        if (_tok.is(tok::ParenR)) {
            // cast
            consume();
            auto expr = parse_expr_climb(ops::prec(Op::Cast), flags);
            if (!expr)
                goto Panic;
            return tree_loc<ast::Cast>(
                loc_id, std::move(full_type_name), std::move(expr));
        }
        // type op
        if (_tok.type_op() == TypeOp::None)
            goto Panic;
        auto type_name = full_type_name->replace_type_name({});
        inner = parse_type_op_rest(std::move(type_name), {});
        assert(inner);
        inner = parse_expr_climb_rest(
            std::move(inner), 0, flags & ~ExprStopAtComma);
    } else {
        inner = parse_expr(flags & ~ExprStopAtComma);
    }
    if (!inner || !expect(tok::ParenR))
        goto Panic;
    return tree_loc<ast::ParenExpr>(loc_id, std::move(inner));

Panic:
    panic(tok::ParenR);
    consume_if(tok::ParenR);
    return {};
}

Ptr<ast::Expr> Parser::parse_class_const_access_or_type_op() {
    assert(_tok.in(tok::Local, tok::BuiltinTypeIdent, tok::TypeIdent));
    auto type_name = parse_type_name(true);
    if (_tok.is(tok::Ident))
        return parse_class_const_access_rest(std::move(type_name));
    return parse_type_op_rest(std::move(type_name), {});
}

Ptr<ast::TypeOpExpr>
Parser::parse_type_op_rest(Ptr<ast::TypeName>&& type, Ptr<ast::Expr>&& expr) {
    assert((bool)type != (bool)expr);

    // obj.Base.<type_op>
    Ptr<ast::TypeIdent> base{};
    if (_tok.is(tok::TypeIdent)) {
        if (!expr) {
            unexpected();
            return {};
        }
        base = parse_type_ident(TypeAllowSuper);
        if (!base || !expect(tok::Period))
            return {};
    }

    // op
    auto type_op = _tok.type_op();
    if (type_op == TypeOp::None) {
        unexpected();
        return {};
    }
    consume();

    // args (instanceof only)
    Ptr<ast::ArgList> args{};
    if (_tok.is(tok::ParenL)) {
        if (type_op != TypeOp::InstanceOf) {
            diag("only `instanceof` type operator can have arguments");
            consume();
            panic(tok::ParenR);
            return {};
        }
        args = parse_arg_list();
    }

    auto loc_id = type ? type->loc_id() : expr->loc_id();
    return tree_loc<ast::TypeOpExpr>(
        loc_id, type_op, std::move(type), std::move(expr), std::move(base),
        std::move(args));
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

Ptr<ast::ExprList> Parser::parse_array_dims(bool allow_empty) {
    assert(_tok.is(tok::BracketL));
    auto node = tree_loc<ast::ExprList>(_tok.loc_id);
    consume();
    while (!_tok.is(tok::Eof)) {
        Ptr<ast::Expr> expr;
        if (!allow_empty || !_tok.is(tok::BracketR))
            expr = parse_expr();
        if (!expect(tok::BracketR))
            return {};
        node->add(std::move(expr));
        if (!_tok.is(tok::BracketL))
            return node;
        consume();
    }
    return {};
}

Ptr<ast::FullTypeName> Parser::parse_full_type_name(bool maybe_type_op) {
    auto type_name = parse_type_name(maybe_type_op);
    if (!type_name)
        return {};

    Ptr<ast::ExprList> array_dims{};
    bool is_ref = false;
    if (!maybe_type_op || _tok.type_op() == TypeOp::None) {
        // []
        if (_tok.is(tok::BracketL)) {
            array_dims = parse_array_dims();
            if (!array_dims)
                return {};
        }

        // &
        is_ref = parse_is_ref();
    }
    auto node = tree_loc<ast::FullTypeName>(
        type_name->loc_id(), std::move(type_name), std::move(array_dims));
    node->set_is_ref(is_ref);
    return node;
}

Ptr<ast::TypeName> Parser::parse_type_name(bool maybe_type_op_or_const) {
    if (!_tok.in(tok::Local, tok::BuiltinTypeIdent, tok::TypeIdent)) {
        unexpected();
        return {};
    }
    auto loc_id = _tok.loc_id;
    auto node = tree_loc<ast::TypeName>(loc_id, parse_type_spec());
    while (_tok.is(tok::Period)) {
        consume();
        if (maybe_type_op_or_const &&
            (_tok.is(tok::Ident) || _tok.is_type_op())) {
            // either Type.<const-var-ident> or Type.<typeop>
            break; // NOTE: '.' consumed,
        }
        if (!_tok.is(tok::TypeIdent) || _tok.is_self_class()) {
            unexpected();
            return {};
        }
        node->add(parse_type_ident(TypeAllowSuper));
    }
    return node;
}

Ptr<ast::TypeSpec> Parser::parse_type_spec() {
    assert(_tok.in(tok::Local, tok::BuiltinTypeIdent, tok::TypeIdent));

    bool ok = true;

    // ident
    Ptr<ast::TypeIdent> ident{};
    bool is_self_or_super = false;
    BuiltinTypeId builtin_type_id = NoBuiltinTypeId;

    auto loc_id = _tok.loc_id;
    if (_tok.is(tok::BuiltinTypeIdent)) {
        builtin_type_id = _tok.builtin_type_id();
        consume();
    } else {
        ident =
            parse_type_ident(TypeAllowSelf | TypeAllowSuper | TypeAllowLocal);
        is_self_or_super = ident->is_self() || ident->is_super();
        ok = ok && ident;
    }

    // args
    Ptr<ast::ArgList> args{};
    if (_tok.is(tok::ParenL) && !is_self_or_super) {
        args = parse_arg_list();
        ok = ok && args;
    }

    if (!ok)
        return {};

    auto node = (builtin_type_id == NoBuiltinTypeId)
                    ? tree<ast::TypeSpec>(std::move(ident), std::move(args))
                    : tree<ast::TypeSpec>(builtin_type_id, std::move(args));
    node->set_loc_id(loc_id);
    return node;
}

Ptr<ast::FunCall> Parser::parse_funcall(Ptr<ast::Expr>&& callable) {
    assert(_tok.is(tok::ParenL));
    auto args = parse_arg_list();
    auto loc_id = args->loc_id();
    return tree_loc<ast::FunCall>(loc_id, std::move(callable), std::move(args));
}

Ptr<ast::FunCall> Parser::parse_op_call() {
    assert(_tok.is(tok::Operator));
    // operator
    consume();

    // operator<op>
    tok::Type op_tok_type = _tok.type;
    if (!_tok.is_overloadable_op()) {
        unexpected();
        return {};
    }
    consume();

    auto op_call = parse_funcall({});
    Op op = Op::None;
    if (op_call->arg_num() == 0) {
        op = tok::unary_pre_op(op_tok_type);
    } else {
        op = tok::bin_op(op_tok_type);
        if (op == Op::None)
            op = tok::unary_post_op(op_tok_type);
    }
    assert(op != Op::None);
    op_call->set_fun_op(op);
    return op_call;
}

Ptr<ast::ArgList> Parser::parse_arg_list() {
    assert(_tok.is(tok::ParenL));
    auto loc_id = _tok.loc_id;
    consume();
    auto args = tree_loc<ast::ArgList>(loc_id);
    while (!_tok.in(tok::ParenR, tok::Eof)) {
        auto expr = parse_expr();
        if (!expr)
            goto Panic;

        args->add(std::move(expr));
        if (_tok.is(tok::Comma)) {
            auto comma = _tok;
            consume();
            if (_tok.is(tok::ParenR))
                diag(comma, "trailing comma in argument list");
        }
    }
    if (!expect(tok::ParenR))
        goto Panic;
    return args;

Panic:
    panic(tok::ParenR);
    consume_if(tok::ParenR);
    return {};
}

Ptr<ast::ArrayAccess> Parser::parse_array_access(Ptr<ast::Expr>&& array) {
    assert(_tok.is(tok::BracketL));
    auto loc_id = _tok.loc_id;
    consume();

    // [index]
    auto index = parse_expr();
    if (!index || !expect(tok::BracketR)) {
        panic(tok::BracketR);
        consume_if(tok::BracketR);
        return {};
    }
    return tree_loc<ast::ArrayAccess>(
        loc_id, std::move(array), std::move(index));
}

Ptr<ast::Expr>
Parser::parse_member_access_type_op_or_op_call(Ptr<ast::Expr>&& obj) {
    assert(_tok.is(tok::Period));
    auto op_loc_id = _tok.loc_id;
    consume();

    if (_tok.in(tok::Ident, tok::Operator)) {
        // <obj>.ident | <obj>.operator<op>
        return parse_member_access_or_op_call_rest(std::move(obj), op_loc_id);
    }

    // <obj>.Type.
    if (_tok.is(tok::TypeIdent)) {
        // Type
        auto ident_tok = _tok;
        consume();
        // .
        if (!match(tok::Period))
            return {};
        auto period_tok = _tok;
        consume();
        auto cur = _tok;
        putback(period_tok);
        putback(ident_tok);
        if (cur.is_type_op()) {
            // <obj>.Type.<type_op>
            return parse_type_op_rest({}, std::move(obj));
        }
        // <obj>.Type.ident?
        return parse_member_access_or_op_call_rest(std::move(obj), op_loc_id);
    }

    // <obj>.<type_op>
    if (_tok.is_type_op())
        return parse_type_op_rest({}, std::move(obj));

    unexpected();
    return {};
}

Ptr<ast::Expr> Parser::parse_member_access_or_op_call_rest(
    Ptr<ast::Expr>&& obj, loc_id_t op_loc_id) {
    assert(_tok.in(tok::Ident, tok::TypeIdent, tok::Operator));

    // obj.Base.mem
    Ptr<ast::TypeIdent> base{};
    if (_tok.is(tok::TypeIdent)) {
        base = parse_type_ident(TypeAllowSuper);
        if (!base || !expect(tok::Period))
            return {};
    }

    Ptr<ast::Ident> ident{};
    tok::Type op_tok_type = tok::Eof;
    if (_tok.is(tok::Operator)) {
        consume();
        op_tok_type = _tok.type;
        if (!_tok.is_overloadable_op()) {
            unexpected();
            return {};
        }
        consume();

    } else {
        if (!match(tok::Ident))
            return {};

        ident = parse_ident();
        if (!ident)
            return {};
    }

    // obj.mem or obj.Base.mem
    auto mem_access = tree_loc<ast::MemberAccess>(
        op_loc_id, std::move(obj), std::move(ident), std::move(base));
    if (op_tok_type == tok::Eof)
        return mem_access;

    // obj.operator<op>()
    if (!match(tok::ParenL))
        return {};
    auto mem_access_ref = ref(mem_access);
    auto op_call = parse_funcall(std::move(mem_access));
    Op op = Op::None;
    if (op_call->arg_num() == 0) {
        op = tok::unary_pre_op(op_tok_type);
    } else {
        op = tok::bin_op(op_tok_type);
        if (op == Op::None)
            op = tok::unary_post_op(op_tok_type);
    }
    assert(op != Op::None);
    mem_access_ref->set_op(op);
    return op_call;
}

Ptr<ast::Ternary> Parser::parse_ternary(Ptr<ast::Expr>&& cond) {
    assert(_tok.is(tok::Quest));
    // ?
    auto loc_id = _tok.loc_id;
    consume();

    auto if_true = parse_expr();

    // :
    if (!expect(tok::Colon))
        return {};

    auto if_false = parse_expr();

    if (!if_true || !if_false)
        return {};

    return tree_loc<ast::Ternary>(
        loc_id, std::move(cond), std::move(if_true), std::move(if_false));
}

Ptr<ast::ClassConstAccess>
Parser::parse_class_const_access_rest(Ptr<ast::TypeName> type_name) {
    assert(_tok.is(tok::Ident));

    auto ident = parse_ident();
    if (!ident)
        return {};

    auto loc_id = ident->loc_id();
    return tree_loc<ast::ClassConstAccess>(
        loc_id, std::move(type_name), std::move(ident));
}

Ptr<ast::TypeIdent> Parser::parse_type_ident(type_flags_t flags) {
    assert(_tok.in(tok::Local, tok::TypeIdent));

    // local
    bool is_local = false;
    if (_tok.is(tok::Local)) {
        if (flags & TypeAllowLocal) {
            is_local = true;
        } else {
            diag("`local' is not allowed in this context");
        }
        consume();
        // .Type
        expect(tok::Period);
        if (!match(tok::TypeIdent))
            return {};
    }

    // Self/Super?
    auto str = tok_ast_str();
    bool is_self = _tok.is_self_class();
    bool is_super = _tok.is_super_class();
    consume();
    if (is_self) {
        if (!(flags & TypeAllowSelf)) {
            diag(str.loc_id(), 1, "`Self' is not allowed in this context");
            return {};
        }
        if (is_local) {
            diag(str.loc_id(), 1, "`Self' cannot be module-local");
            return {};
        }
    } else if (is_super) {
        if (!(flags & TypeAllowSuper)) {
            diag(str.loc_id(), 1, "`Super' is not allowed in this context");
            return {};
        }
        if (is_local) {
            diag(str.loc_id(), 1, "`Super' cannot be module-local");
            return {};
        }
    }

    auto ident = tree<ast::TypeIdent>(str);
    ident->set_is_self(is_self);
    ident->set_is_super(is_super);
    ident->set_is_local(is_local);
    return ident;
}

Ptr<ast::Ident> Parser::parse_ident(ident_flags_t flags) {
    assert(_tok.in(tok::Ident, tok::Local));

    // local
    bool is_local = false;
    if (_tok.is(tok::Local)) {
        if (flags & IdentAllowLocal) {
            is_local = true;
        } else {
            diag("`local' is not allowed in this context");
        }
        consume();
        // .ident
        expect(tok::Period);
        if (!match(tok::Ident))
            return {};
    }

    // ident
    auto str = tok_ast_str();
    bool is_self = _tok.is_self();
    bool is_super = _tok.is_super();
    consume();
    if (is_self && !(flags & IdentAllowSelfOrSuper)) {
        diag(str.loc_id(), 1, "`self' is not allowed in this context");
        return {};
    }
    if (is_super && !(flags & IdentAllowSelfOrSuper)) {
        diag(str.loc_id(), 1, "`super' is not allowed in this context");
        return {};
    }
    auto ident = tree_loc<ast::Ident>(str.loc_id(), str);
    ident->set_is_self(is_self);
    ident->set_is_super(is_super);
    ident->set_is_local(is_local);
    return ident;
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
    auto loc_id = _tok.loc_id;
    auto number = num_lit_number();
    consume();
    return tree_loc<ast::NumLit>(loc_id, std::move(number));
}

Ptr<ast::NumLit> Parser::parse_char_lit() {
    assert(_tok.is(tok::Char));
    auto loc_id = _tok.loc_id;
    auto number = detail::parse_char_str(_ctx.diag(), _tok.loc_id, tok_str());
    consume();
    return tree_loc<ast::NumLit>(loc_id, std::move(number));
}

Ptr<ast::StrLit> Parser::parse_str_lit() {
    assert(_tok.is(tok::String));
    auto loc_id = _tok.loc_id;
    auto text = str_lit_text();
    consume();
    auto str_id = _text_pool.put(text);
    return tree_loc<ast::StrLit>(loc_id, String{str_id});
}

Number Parser::num_lit_number() {
    assert(_tok.is(tok::Number));
    switch (_tok.orig_type) {
    case tok::Number:
        return detail::parse_num_str(_ctx.diag(), _tok.loc_id, tok_str());
    case tok::__Line:
        return {Radix::Decimal, (Unsigned)_ctx.sm().loc(_tok.loc_id).linum()};
    default:
        assert(false);
    }
}

std::string Parser::str_lit_text() {
    assert(_tok.is(tok::String));
    switch (_tok.orig_type) {
    case tok::String:
        return detail::parse_str(_ctx.diag(), _tok.loc_id, tok_str());
    case tok::__File:
        return _pp.current_path().filename();
    case tok::__FilePath:
        return _pp.current_path();
    case tok::__Class:
        if (!_cur_cls_def) {
            diag("__CLASS__ macro defined outside of class body");
            return {};
        }
        return std::string{_str_pool.get(_cur_cls_def->name_id())};
    case tok::__Func:
        if (!_cur_fun_def) {
            diag("__FUN__ macro outside of function body");
            return {};
        }
        return std::string{_str_pool.get(_cur_fun_def->name_id())};
    default:
        assert(false);
    }
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

const std::string_view Parser::tok_str() {
    assert(_tok.in(
        tok::Ident, tok::TypeIdent, tok::Number, tok::Char, tok::String));
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
