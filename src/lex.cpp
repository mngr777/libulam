#include "src/lex.hpp"
#include "libulam/token.hpp"
#include "src/detail/str.hpp"
#include "src/preproc.hpp"
#include "src/src_mngr.hpp"
#include <string_view>

namespace ulam {

void Lex::lex(Token& token) {
    skip_whitespace();
    start(token);
    char ch = cur();
    advance();
    switch (ch) {
    case '\0':
        complete(tok::Eof);
        break;
    case '!':
        if (at('=')) {
            advance();
            complete(tok::ExclEqual);
        } else {
            complete(tok::Excl);
        }
        break;
    case '"':
        lex_str('"');
        break;
    case '#':
        complete(tok::Sharp);
        break;
    case '$':
        complete(tok::Dollar);
        break;
    case '%':
        if (at('=')) {
            advance();
            complete(tok::PercentEqual);
        } else {
            complete(tok::Percent);
        }
        break;
    case '&':
        if (at('&')) {
            advance();
            complete(tok::AmpAmp);
        } else if (at('=')) {
            advance();
            complete(tok::AmpEqual);
        } else {
            complete(tok::Amp);
        }
        break;
    case '\'':
        lex_str('\'');
        break;
    case '(':
        complete(tok::ParenL);
        break;
    case ')':
        complete(tok::ParenR);
        break;
    case '*':
        if (at('=')) {
            advance();
            complete(tok::AstEqual);
        } else {
            complete(tok::Amp);
        }
        break;
    case '+':
        if (at('=')) {
            advance();
            complete(tok::PlusEqual);
        } else if (detail::is_digit(cur())) {
            complete(tok::PlusSign);
        } else {
            complete(tok::Plus);
        }
        break;
    case ',':
        complete(tok::Comma);
        break;
    case '-':
        if (at('=')) {
            advance();
            complete(tok::MinusEqual);
        } else if (detail::is_digit(cur())) {
            complete(tok::MinusSign);
        } else {
            complete(tok::Minus);
        }
        break;
    case '.':
        if (at('.') && next(1) == '.') {
            advance(2);
            complete(tok::Ellipsis);
        } else {
            complete(tok::Dot);
        }
        break;
    case '/':
        if (at('/') || at('*')) {
            lex_comment();
        } else if (at('=')) {
            advance();
            complete(tok::SlashEqual);
        } else {
            complete(tok::Slash);
        }
    case ':':
        complete(tok::Colon);
        break;
    case ';':
        complete(tok::Semicol);
        break;
    case '<':
        if (_expect_path) {
            lex_str('>');
        } else if (at('<')) {
            advance();
            if (at('=')) {
                advance();
                complete(tok::LessLessEqual);
            } else {
                complete(tok::LessLess);
            }
        } else {
            complete(tok::Less);
        }
        break;
    case '=':
        if (at('=')) {
            advance();
            complete(tok::EqualEqual);
        } else {
            complete(tok::Equal);
        }
        break;
    case '>':
        if (at('>')) {
            advance();
            if (at('=')) {
                advance();
                complete(tok::GreaterGreaterEqual);
            } else {
                complete(tok::GreaterGreater);
            }
        } else {
            complete(tok::Greater);
        }
        break;
    case '?':
        complete(tok::Quest);
        break;
    case '@':
        lex_word();
        break;
    case '[':
        complete(tok::BracketL);
        break;
    case '\\':
        if (at('\n')) {
            advance();
            lex(token);
        } else if (at('\r') && next() == '\n') {
            advance(2);
            lex(token);
        } else {
            complete(tok::UnexpectedChar);
        }
        break;
    case ']':
        complete(tok::BracketR);
    case '^':
        if (at('=')) {
            advance();
            complete(tok::CaretEqual);
        } else {
            complete(tok::Caret);
        }
        break;
    case '`':
        complete(tok::UnexpectedChar);
        break;
    case '{':
        complete(tok::BraceL);
        break;
    case '|':
        if (at('|')) {
            advance();
            complete(tok::PipePipe);
        } else if (at('=')) {
            advance();
            complete(tok::PipeEqual);
        } else {
            complete(tok::Pipe);
        }
        break;
    case '}':
        complete(tok::BraceR);
        break;
    case '~':
        complete(tok::Tilde);
        break;
    default:
        if (detail::is_digit(ch)) {
            lex_number();
        } else if (detail::is_word(ch)) {
            lex_word();
        } else {
            complete(tok::UnexpectedChar);
        }
    }
}

void Lex::lex_path(Token& token) {
    _expect_path = true;
    lex(token);
    _expect_path = false;
}

char Lex::next(std::size_t off) const {
    assert(off > 0 && _cur + off < _buf.end());
    return _cur[off];
}

char Lex::prev() const {
    assert(_cur > _buf.start());
    return _cur[-1];
}

void Lex::advance(std::size_t steps) {
    assert(_cur + steps < _buf.end());
    _cur += steps;
}

void Lex::skip_whitespace() {
    while (true) {
        switch (cur()) {
        case ' ':
        case '\t':
            advance();
            break;
        case '\r':
            if (next() == '\n')
                newline(2);
            break;
        case '\n':
            newline();
            break;
        default:
            return;
        }
    }
}

loc_id_t Lex::loc_id() {
    return _sm.loc_id(_src_id, _cur, _linum, chr());
}

void Lex::start(Token& token) {
    _token_start = _cur;
    _token = &token;
    token.loc_id = loc_id();
}

void Lex::complete(tok::Type type) {
    assert(_token);
    _token->type = type;
    _token->size = _cur - _token_start;
}

void Lex::newline(std::size_t size) {
    assert(size < 3);
    advance(size);
    ++_linum;
    _line = _cur;
}

void Lex::lex_str(char closing) {
    bool esc = false;
    while (true) {
        switch (cur()) {
        case '\0':
            goto Done;
        case '\r':
            if (next() == '\n') {
                if (!esc)
                    goto Done; // unterminated
                advance();
                break;
            }
            break;
        case '\n':
            if (!esc)
                goto Done; // unterminated
            break;
        case '"':
            if (closing == '"' && !esc) {
                advance();
                goto Done;
            }
            break;
        case '\'':
            if (closing == '\'' && !esc) {
                advance();
                goto Done;
            }
            break;
        case '>':
            // include path in angled brackets
            if (closing == '>' && !esc) {
                advance();
                goto Done;
            }
        case '\\':
            if (!esc) {
                esc = true;
                advance();
                continue;
            }
            break;
        }
        advance();
        esc = false;
    }
Done:
    complete(tok::String);
}

void Lex::lex_comment() {
    bool esc = false;
    while (true) {
        switch (cur()) {
        case '\0':
            return;
        case '\r':
            if (next() == '\n' && !esc)
                return;
        case '\n':
            return;
        case '\\':
            if (!esc) {
                esc = true;
                advance();
                continue;
            }
            break;
        }
        advance();
        esc = false;
    }
}

void Lex::lex_number() {
    assert(_token_start);
    assert(detail::is_digit(_token_start[0]));
    bool hex = false;
    if (_token_start[0] == '0') {
        // Scroll over `0x' or `0b' prefix if there's a digit after
        hex = at('x');
        if ((hex || at('b')) && detail::is_digit(next()))
            advance();
    }
    while (hex ? detail::is_xdigit(cur()) : detail::is_digit(cur()))
        advance();
    // Allow `u' suffix if there were some digits before it
    if ((at('u') || at('U')) && detail::is_digit(prev()))
        advance();
    // Check if separated from next token if required
    if (detail::is_word(cur()) || detail::is_digit(cur()))
        _pp.diag(loc_id(), "Number not separated");
    complete(tok::Number);
}

void Lex::lex_word() {
    assert(_token_start);
    assert(detail::is_word(cur()));
    while (detail::is_word(cur()))
        advance();
    auto type = tok::type_by_word(
        {_token_start, static_cast<std::size_t>(_cur - _token_start)});
    complete(type);
}

} // namespace ulam
