#include "libulam/lex.hpp"
#include "libulam/preproc.hpp"
#include "libulam/src_mngr.hpp"
#include "libulam/token.hpp"
#include "src/detail/string.hpp"
#include <string_view>

namespace ulam {

void Lex::lex(Token& token) {
    skip_whitespace();
    start(token);
    char ch = _cur[0];
    advance();
    switch (ch) {
    case '\0':
        if (_cur != _buf.end())
            _pp.diag(_tok->loc_id, "\\0 before end of input");
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
        } else if (at('+')) {
            advance();
            complete(tok::PlusPlus);
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
        } else if (at('-')) {
            advance();
            complete(tok::MinusMinus);
        } else {
            complete(tok::Minus);
        }
        break;
    case '.':
        if (at('.') && _cur[1] == '.') {
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
            newline(1);
            lex(token);
        } else if (at('\r') && _cur[1] == '\n') {
            newline(2);
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

void Lex::advance(std::size_t len) {
    assert(_cur + len <= _buf.end());
    _cur += len;
}

void Lex::skip_whitespace() {
    while (true) {
        switch (_cur[0]) {
        case ' ':
        case '\t':
            advance();
            break;
        case '\r':
            if (_cur[1] == '\n')
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

loc_id_t Lex::loc_id() { return _sm.loc_id(_src_id, _cur, _linum, chr()); }

void Lex::start(Token& token) {
    _tok_start = _cur;
    _tok = &token;
    token.loc_id = loc_id();
}

void Lex::complete(tok::Type type) {
    assert(_tok);
    _tok->type = type;
    _tok->size = _cur - _tok_start;
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
        switch (_cur[0]) {
        case '\0':
            goto Done;
        case '\r':
            if (_cur[1] == '\n') {
                newline(2);
                if (!esc)
                    goto Done; // unterminated
            } else {
                advance();
            }
            break;
        case '\n':
            newline(1);
            if (!esc)
                goto Done; // unterminated
            break;
        case '"':
            advance();
            if (closing == '"' && !esc)
                goto Done;
            break;
        case '\'':
            advance();
            if (closing == '\'' && !esc)
                goto Done;
            break;
        case '>':
            // include path in angled brackets
            advance();
            if (closing == '>' && !esc)
                goto Done;
        case '\\':
            advance();
            if (!esc) {
                esc = true;
                continue;
            }
            break;
        default:
            advance();
        }
        esc = false;
    }
Done:
    complete(tok::String);
}

void Lex::lex_comment() {
    bool esc = false;
    while (true) {
        switch (_cur[0]) {
        case '\0':
            return;
        case '\r':
            if (_cur[1] == '\n' && !esc)
                newline(2);
            return;
        case '\n':
            newline(1);
            return;
        case '\\':
            advance();
            if (!esc) {
                esc = true;
                continue;
            }
            break;
        default:
            advance();
        }
        esc = false;
    }
}

void Lex::lex_number() {
    assert(_tok_start);
    assert(detail::is_digit(_tok_start[0]));
    bool hex = false;
    if (_tok_start[0] == '0') {
        // Scroll over `0x' or `0b' prefix if there's a digit after
        hex = at('x');
        if ((hex || at('b')) && detail::is_digit(_cur[1]))
            advance();
    }
    while (hex ? detail::is_xdigit(_cur[0]) : detail::is_digit(_cur[0]))
        advance();
    // Allow `u' suffix if there were some digits before it
    if ((at('u') || at('U')) && detail::is_digit(_cur[-1]))
        advance();
    // Check if separated from next token if required
    if (detail::is_word(_cur[0]) || detail::is_digit(_cur[0]))
        _pp.diag(loc_id(), "Number not separated");
    complete(tok::Number);
}

void Lex::lex_word() {
    assert(_tok_start);
    assert(_cur[-1] == '@' || detail::is_word(_cur[-1]));
    while (detail::is_word(_cur[0]))
        advance();
    auto type = tok::type_by_keyword(
        {_tok_start, static_cast<std::size_t>(_cur - _tok_start)});
    complete(type);
}

} // namespace ulam
