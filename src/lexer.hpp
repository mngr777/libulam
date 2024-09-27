#pragma once
#include "lexer/dfa.hpp"
#include "libulam/token.hpp"

namespace ulam {

class Context;
class SourceStream;

class Lexer {
public:
    Lexer(Context& ctx, SourceStream& ss);

    Lexer& operator>>(Token& token);
    Token get();
    const Token& peek();

private:
    void next();
    bool next_start();
    void next_end();

    Context& _ctx;
    SourceStream& _ss;
    lex::Dfa _dfa;
    Token _token;
};

} // namespace ulam
