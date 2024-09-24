#include "src/lexer/dfa.hpp"
#include <cassert>

namespace ulam::lex {
namespace dfa {
namespace {

#if __has_include("src/gen/lexer/dfa.gen.cpp")
#    include "src/gen/lexer/dfa.gen.cpp"
#else
#    error Missing file "src/gen/lexer/dfa.gen.cpp"
ClassFlags CharClassTable[1] = {};
unsigned StartStates[1] = {{}};
Node NodeList[1] = {{}};
Edge EdgeList[1] = {{}};
#endif
} // namespace

ClassFlags char_class(char ch) {
    assert(ch >= 0);
    return CharClassTable[(int)ch];
}

} // namespace dfa

Dfa::Dfa():
    _start_states(dfa::StartStates),
    _nodes(dfa::NodeList),
    _edges(dfa::EdgeList),
    _state(0),
    _type(tok::None) {}

Dfa::Result Dfa::step(char ch) {
    assert(ch >= 0);

    if (_state == 0) {
        // Starting state
        _state = _start_states[(int)ch];
        return (_state != 0) ? Result::TokenStart : Result::None;
    }

    // Move to next state
    unsigned edge = _nodes[_state].first_edge;
    while (!dfa::EdgeList[edge].match(ch))
        ++edge;
    _state = dfa::EdgeList[edge].next;

    const dfa::Node& node = dfa::NodeList[_state];
    if (node.is_final) {
        _type = node.value;
        _state = 0; // back to initial
    }
    return node.is_final ? Result::TokenEnd : Result::None;
}

} // namespace ulam::lex
