#include "src/context.hpp"
#include "src/lexer.hpp"

namespace ulam {

Lexer& Lexer::operator>>(Token& token) {
    // TODO
    return *this;
}

bool Lexer::next() {
    // TODO
    return false;
}

Token Lexer::get() {
    // TODO
    return {};
}

const Token& Lexer::peek() {
    // TODO
    return _cur;
}

} // namespace ulam

