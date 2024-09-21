#pragma once
#include "libulam/token.hpp"
#include "lexer/dfa.hpp"

namespace ulam {

class SourceStream;

class Lexer {
public:
    explicit Lexer(SourceStream& ss);

    Lexer& operator>>(Token& token);
    Token get();
    const Token& peek();

private:
    void next();
    bool next_start();
    void next_end();

    SourceStream& _ss;
    lex::Dfa _dfa;
    Token _token;
};

} // namespace ulam
