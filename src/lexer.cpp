#include "src/lexer.hpp"
#include "libulam/types.hpp"
#include "src/source.hpp"
#include <cassert>

namespace ulam {

Lexer::Lexer(SourceStream& ss): _ss(ss) { next(); }

Lexer& Lexer::operator>>(Token& token) {
    token = _token;
    next();
    return *this;
}

Token Lexer::get() {
    Token token = _token;
    next();
    return token;
}

const Token& Lexer::peek() { return _token; }

void Lexer::next() {
    if (next_start()) {
        next_end();
    } else {
        _token = {}; // done
    }
}

bool Lexer::next_start() {
    char ch;
    while (!_ss.eof()) {
        _ss.get(ch);
        if (_dfa.step(ch) == lex::Dfa::TokenStart) {
            _token.loc_id = _ss.loc_id();
            return true;
        }
    }
    return false;
}

void Lexer::next_end() {
    while (true) {
        char ch = '\0';
        if (!_ss.eof())
            _ss.get(ch);
        if (_dfa.step(ch) != lex::Dfa::TokenEnd) {
            assert(
                ch != '\n' && ch != '\0' && "Token not done on '\n' or '\0'");
            continue;
        }

        // Commit type
        _token.type = _dfa.type();
        // Store name, number or string
        switch (_token.type) {
        case tok::Name:
        case tok::Number:
        case tok::String:
            _token.str_id = _ss.str_id();
            break;
        default:
            _token.str_id = NoStrId;
        }
        // Reuse last char
        if (ch != '\0' && ch != '\n')
            _ss.putback(ch);
        return;
    }
}

} // namespace ulam
