#include <cassert>
#include <libulam/context.hpp>
#include <libulam/preproc.hpp>
#include <libulam/src_mngr.hpp>
#include <libulam/token.hpp>

namespace ulam {

void Preproc::main_file(std::filesystem::path path) {
    assert(_stack.empty());
    auto src = _ctx.sm().file(std::move(path));
    push(src);
}

void Preproc::main_string(std::string text) {
    assert(_stack.empty());
    auto src = _ctx.sm().string(std::move(text));
    push(src);
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
            if (!preproc_load()) {
                // TODO: emit error
                return *this;
            }
            break;
        case tok::Comment:
            // ignore comments
            break;
        case tok::Eof:
            _stack.pop();
            if (_stack.empty())
                return *this;
            break;
        case tok::IntT:
        case tok::UnsignedT:
        case tok::BoolT:
        case tok::UnaryT:
        case tok::BitsT:
        case tok::AtomT:
        case tok::VoidT:
        case tok::StringT:
            token.type = tok::BuiltinTypeIdent;
            // fallthru
        default:
            return *this;
        }
    }
}

void Preproc::push(Src* src) {
    assert(src);
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
        // TODO: load
        expect(token, tok::Semicol, tok::Eof);
    }
}

bool Preproc::preproc_load() {
    Token path;
    lexer().lex_path(path);
    auto src = _ctx.sm().file(_ctx.sm().str_at(path.loc_id, path.size));
    if (src)
        push(src);
    return src;
}

} // namespace ulam
