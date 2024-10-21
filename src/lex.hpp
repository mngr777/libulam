#pragma once
#include "libulam/src_loc.hpp"
#include "libulam/token.hpp"
#include "src/memory/buf.hpp"

namespace ulam {

class Src;
class SrcMngr;
class Preproc;

class Lex {
public:
    Lex(Preproc& pp, SrcMngr& sm, src_id_t src_id, const mem::BufRef buf):
        _pp{pp},
        _sm{sm},
        _src_id{src_id},
        _buf{buf},
        _cur{buf.start()},
        _line{buf.start()} {}

    void lex(Token& token);
    void lex_path(Token& token);

private:
    char cur() const { return *_cur; }
    chr_t chr() const { return _cur + 1 - _line; }
    bool at(char ch) const { return *_cur == ch; }

    char next(std::size_t off = 1) const;
    char prev() const;

    void advance(std::size_t steps = 1);
    void skip_whitespace();
    void newline(std::size_t size = 1);

    loc_id_t loc_id();
    void start(Token& token);
    void complete(tok::Type type);

    void lex_str(char closing);
    void lex_comment();
    void lex_number();
    void lex_word();

    Preproc& _pp;
    SrcMngr& _sm;
    const src_id_t _src_id;
    const mem::BufRef _buf;

    const char* _cur;
    const char* _line;
    const char* _token_start;
    Token* _token;

    linum_t _linum{1};
    chr_t _chr{1};

    bool _expect_path{false};
};

} // namespace ulam
