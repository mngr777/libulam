#include "src/lexer/dfa.hpp"
#include <cassert>
#include <iostream>

namespace ulam::lex {
namespace dfa {
namespace {
const CatFlags CharCatFlags[128] = {
    cat::Space, /* 00 */
    cat::Space, /* 01 */
    cat::Space, /* 02 */
    cat::Space, /* 03 */
    cat::Space, /* 04 */
    cat::Space, /* 05 */
    cat::Space, /* 06 */
    cat::Space, /* 07 */
    cat::Space, /* 08 */
    cat::Space, /* 09 */
    cat::Space, /* 0a */
    cat::Space, /* 0b */
    cat::Space, /* 0c */
    cat::Space, /* 0d */
    cat::Space, /* 0e */
    cat::Space, /* 0f */
    cat::Space, /* 10 */
    cat::Space, /* 11 */
    cat::Space, /* 12 */
    cat::Space, /* 13 */
    cat::Space, /* 14 */
    cat::Space, /* 15 */
    cat::Space, /* 16 */
    cat::Space, /* 17 */
    cat::Space, /* 18 */
    cat::Space, /* 19 */
    cat::Space, /* 1a */
    cat::Space, /* 1b */
    cat::Space, /* 1c */
    cat::Space, /* 1d */
    cat::Space, /* 1e */
    cat::Space, /* 1f */
    cat::Space, /* 20 ' ' */
    cat::Other, /* 21 '!' */
    cat::Other, /* 22 '"' */
    cat::Other, /* 23 '#' */
    cat::Other, /* 24 '$' */
    cat::Other, /* 25 '%' */
    cat::Other, /* 26 '&' */
    cat::Other, /* 27 ''' */
    cat::Other, /* 28 '(' */
    cat::Other, /* 29 ')' */
    cat::Other, /* 2a '*' */
    cat::Other, /* 2b '+' */
    cat::Other, /* 2c ',' */
    cat::Other, /* 2d '-' */
    cat::Other, /* 2e '.' */
    cat::Other, /* 2f '/' */
    cat::Digit, /* 30 '0' */
    cat::Digit, /* 31 '1' */
    cat::Digit, /* 32 '2' */
    cat::Digit, /* 33 '3' */
    cat::Digit, /* 34 '4' */
    cat::Digit, /* 35 '5' */
    cat::Digit, /* 36 '6' */
    cat::Digit, /* 37 '7' */
    cat::Digit, /* 38 '8' */
    cat::Digit, /* 39 '9' */
    cat::Other, /* 3a ':' */
    cat::Other, /* 3b ';' */
    cat::Other, /* 3c '<' */
    cat::Other, /* 3d '=' */
    cat::Other, /* 3e '>' */
    cat::Other, /* 3f '?' */
    cat::Other, /* 40 '@' */
    cat::Alpha, /* 41 'A' */
    cat::Alpha, /* 42 'B' */
    cat::Alpha, /* 43 'C' */
    cat::Alpha, /* 44 'D' */
    cat::Alpha, /* 45 'E' */
    cat::Alpha, /* 46 'F' */
    cat::Alpha, /* 47 'G' */
    cat::Alpha, /* 48 'H' */
    cat::Alpha, /* 49 'I' */
    cat::Alpha, /* 4a 'J' */
    cat::Alpha, /* 4b 'K' */
    cat::Alpha, /* 4c 'L' */
    cat::Alpha, /* 4d 'M' */
    cat::Alpha, /* 4e 'N' */
    cat::Alpha, /* 4f 'O' */
    cat::Alpha, /* 50 'P' */
    cat::Alpha, /* 51 'Q' */
    cat::Alpha, /* 52 'R' */
    cat::Alpha, /* 53 'S' */
    cat::Alpha, /* 54 'T' */
    cat::Alpha, /* 55 'U' */
    cat::Alpha, /* 56 'V' */
    cat::Alpha, /* 57 'W' */
    cat::Alpha, /* 58 'X' */
    cat::Alpha, /* 59 'Y' */
    cat::Alpha, /* 5a 'Z' */
    cat::Other, /* 5b '[' */
    cat::Other, /* 5c '\' */
    cat::Other, /* 5d ']' */
    cat::Other, /* 5e '^' */
    cat::Other, /* 5f '_' */
    cat::Other, /* 60 '`' */
    cat::Alpha, /* 61 'a' */
    cat::Alpha, /* 62 'b' */
    cat::Alpha, /* 63 'c' */
    cat::Alpha, /* 64 'd' */
    cat::Alpha, /* 65 'e' */
    cat::Alpha, /* 66 'f' */
    cat::Alpha, /* 67 'g' */
    cat::Alpha, /* 68 'h' */
    cat::Alpha, /* 69 'i' */
    cat::Alpha, /* 6a 'j' */
    cat::Alpha, /* 6b 'k' */
    cat::Alpha, /* 6c 'l' */
    cat::Alpha, /* 6d 'm' */
    cat::Alpha, /* 6e 'n' */
    cat::Alpha, /* 6f 'o' */
    cat::Alpha, /* 70 'p' */
    cat::Alpha, /* 71 'q' */
    cat::Alpha, /* 72 'r' */
    cat::Alpha, /* 73 's' */
    cat::Alpha, /* 74 't' */
    cat::Alpha, /* 75 'u' */
    cat::Alpha, /* 76 'v' */
    cat::Alpha, /* 77 'w' */
    cat::Alpha, /* 78 'x' */
    cat::Alpha, /* 79 'y' */
    cat::Alpha, /* 7a 'z' */
    cat::Other, /* 7b '{' */
    cat::Other, /* 7c '|' */
    cat::Other, /* 7d '}' */
    cat::Other, /* 7e '~' */
    cat::Space};

#if __has_include("src/gen/lexer/dfa.gen.cpp")
#    include "src/gen/lexer/dfa.gen.cpp"
#else
#    error Missing file "src/gen/lexer/dfa.gen.cpp"
unsigned StartStates[1] = {{}};
Node NodeList[1] = {{}};
Edge EdgeList[1] = {{}};
#endif
} // namespace

CatFlags char_cat(char ch) {
    assert(ch >= 0);
    return CharCatFlags[(int)ch];
}

} // namespace dfa

Dfa::Dfa():
    _start_states(dfa::StartStates),
    _nodes(dfa::NodeList),
    _edges(dfa::EdgeList),
    _state(0),
    _type(tok::None) {}

dfa::Result Dfa::step(char ch) {
    assert(ch >= 0);
    // std::cout << ch;

    if (_state == 0) {
        // Starting state
        _state = _start_states[(int)ch];
        // if (_state != 0)
        //     std::cout << "[start: " << _state << "]";
        return (_state != 0) ? dfa::Result::TokenStart : dfa::Result::None;
    }

    // Move to next state
    unsigned edge = _nodes[_state].first_edge;
    while (!dfa::EdgeList[edge].match(ch))
        ++edge;
    _state = dfa::EdgeList[edge].next;

    const dfa::Node& node = dfa::NodeList[_state];
    if (node.result == dfa::Result::TokenEnd) {
        // std::cout << "[end: " << _state << "]";
        _type = node.value;
        _state = 0; // back to initial
    }
    return node.result;
}

} // namespace ulam::lex
