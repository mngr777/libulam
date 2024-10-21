#include "src/preproc.hpp"
#include "libulam/token.hpp"
#include <cassert>

namespace ulam {

void Preproc::main_file(std::filesystem::path path) {
    assert(_stack.empty());
    auto src = _sm.file(std::move(path));
    push(src);
}

void Preproc::main_str(std::string text) {
    assert(_stack.empty());
    auto src = _sm.str(std::move(text));
    push(src);
}

Preproc& Preproc::operator>>(Token& token) {
    assert(!_stack.empty());
    while (true) {
        lex(token);
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
        case tok::Eof:
            if (!_stack.empty()) {
                _stack.pop();
            } else {
                return *this;
            }
        default:
            return *this;
        }
    }
}

void Preproc::push(Src* src) {
    assert(src);
    _stack.emplace(src, Lex{_sm, src->id(), src->content()});
}

Lex& Preproc::lexer() {
    assert(!_stack.empty());
    return _stack.top().second;
}

void Preproc::lex(Token& token) {
    return lexer().lex(token);
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
    if (expect(token, tok::TypeName, tok::Semicol, tok::Eof)) {
        // TODO: load
        expect(token, tok::Semicol, tok::Eof);
    }
}

bool Preproc::preproc_load() {
    auto path = lexer().lex_path();
    auto src = _sm.file(path);
    if (src)
        push(src);
    return src;
}

} // namespace ulam
