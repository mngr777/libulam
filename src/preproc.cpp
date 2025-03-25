#include "src/parser/string.hpp"
#include <cassert>
#include <libulam/context.hpp>
#include <libulam/preproc.hpp>
#include <libulam/src_mngr.hpp>
#include <libulam/token.hpp>

#define DEBUG_PREPROC // TEST
#ifdef DEBUG_PREPROC
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Preproc] "
#endif
#include "src/debug.hpp"

namespace ulam {

void Preproc::main_file(Path path) {
    assert(_stack.empty());
    auto src = _ctx.sm().file(std::move(path));
    push(src);
}

void Preproc::main_string(std::string text, Path path) {
    assert(_stack.empty());
    auto src = _ctx.sm().string(std::move(text), std::move(path));
    push(src);
}

void Preproc::add_string(std::string text, Path path) {
    _ctx.sm().string(std::move(text), std::move(path));
}

Preproc& Preproc::operator>>(Token& token) {
    assert(!_stack.empty());
    while (true) {
        lex(token);
        token.orig_type = token.type;
        switch (token.type) {
        case tok::Ulam:
            preproc_ulam();
            break;
        case tok::Use:
            preproc_use();
            break;
        case tok::Load:
            if (!preproc_load())
                return *this;
            break;
        case tok::Comment:
        case tok::MlComment:
            // ignore comments
            break;
        case tok::Self:
        case tok::Super:
            token.type = tok::Ident;
            return *this;
        case tok::SelfClass:
        case tok::SuperClass:
            token.type = tok::TypeIdent;
            return *this;
        case tok::IntT:
        case tok::UnsignedT:
        case tok::BoolT:
        case tok::UnaryT:
        case tok::BitsT:
        case tok::AtomT:
        case tok::VoidT:
        case tok::StringT:
            token.type = tok::BuiltinTypeIdent;
            return *this;
        case tok::__File:
        case tok::__FilePath:
        case tok::__Class:
        case tok::__Func:
            token.type = tok::String;
            return *this;
        case tok::__Line:
            token.type = tok::Number;
            return *this;
        case tok::Eof:
            _stack.pop();
            if (_stack.empty())
                return *this;
            break;
        default:
            return *this;
        }
    }
}

const Preproc::Path& Preproc::current_path() const {
    assert(!_stack.empty());
    return _stack.top().first->path();
}

void Preproc::push(Src* src) {
    assert(src);
    assert(src->content().start());
    _stack.emplace(src, Lex{*this, _ctx.sm(), src->id(), src->content()});
}

Lex& Preproc::lexer() {
    assert(!_stack.empty());
    return _stack.top().second;
}

void Preproc::lex(Token& token) { return lexer().lex(token); }

template <typename... Ts>
bool Preproc::expect(Token& token, tok::Type type, Ts... stop) {
    lex(token);
    if (token.is(type))
        return true;
    while (true) {
        lex(token);
        if (token.in(stop...))
            break;
        diag(token.loc_id, "Unexpected token");
    }
    return false;
}

void Preproc::preproc_ulam() {
    Token token;
    if (expect(token, tok::Number, tok::Semicol, tok::Eof)) {
        _state.version = 5; // TODO
        expect(token, tok::Semicol, tok::Eof);
    }
}

void Preproc::preproc_use() {
    Token token;
    if (expect(token, tok::TypeIdent, tok::Semicol, tok::Eof)) {
        expect(token, tok::Semicol, tok::Eof);
    }
}

bool Preproc::preproc_load() {
    auto& sm = _ctx.sm();
    auto& diag = _ctx.diag();

    Token path_tok;
    lexer().lex_path(path_tok);

    auto path = sm.str_at(path_tok.loc_id, path_tok.size);
    // bool global = (path[0] == '<'); // TODO
    path = detail::parse_str(diag, path_tok.loc_id, path);

    auto src = sm.src(path);
    if (!src)
        src = sm.file(path);
    if (src) {
        push(src);
    } else {
        _ctx.diag().fatal(path_tok.loc_id, path_tok.size, "file not found");
        return false;
    }
    return true;
}

} // namespace ulam
