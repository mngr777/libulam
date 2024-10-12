#include "src/lexer.hpp"
#include "libulam/types.hpp"
#include "src/context.hpp"
#include "src/source.hpp"
#include <cassert>

#ifdef DEBUG_LEXER
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Lexer] "
#endif
#include "src/debug.hpp"

namespace ulam {

Lexer::Lexer(Context& ctx, SourceStream& ss): _ctx(ctx), _ss(ss) { next(); }

Lexer& Lexer::operator>>(Token& token) {
    token = _token;
    next();
    return *this;
}

Token Lexer::get() {
    Token token;
    *this >> token;
    return token;
}

const Token& Lexer::peek() const { return _token; }

void Lexer::next() {
    if (next_start()) {
        next_end();
    } else {
        _token = {}; // done
    }
}

bool Lexer::next_start() {
    while (!_ss.eof()) {
        char ch = '\0';
        _ss.get(ch);
        if (_dfa.step(ch) == lex::Dfa::TokenStart) {
            _token.loc_id = _ss.last_loc_id();
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
                ch != '\n' && ch != '\0' &&
                "Token doesn't end on '\n' or '\0'");
            continue;
        }

        // Commit type
        _token.type = _dfa.type();
        // Store name, number or string
        switch (_token.type) {
        case tok::Name:
            _token.str_id = _ctx.store_name_str(_ss.substr());
            break;
        case tok::Number:
        case tok::String:
            _token.str_id = _ctx.store_value_str(_ss.substr());
            break;
        default:
            _token.str_id = NoStrId;
        }
        // Reuse last char
        if (!_ss.eof() && ch != '\n')
            _ss.unget();
        break;
    }
}

} // namespace ulam
