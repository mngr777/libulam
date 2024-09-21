#pragma once
#include "libulam/token.hpp"
#include <cstdint>

namespace ulam::lex {
namespace dfa {

using CatFlags = std::uint8_t;

namespace cat {
constexpr CatFlags None = 0;
constexpr CatFlags Any = ~0;
constexpr CatFlags Alpha = 1 << 0;
constexpr CatFlags Digit = 1 << 1;
constexpr CatFlags Other = 1 << 2;
constexpr CatFlags Space = 1 << 3;
constexpr CatFlags Alnum = Alpha | Digit;
} // namespace cat

enum class Result { None, TokenStart, TokenEnd };

CatFlags char_cat(char ch);

struct Edge {
    char chr{'\0'};
    CatFlags cat{cat::None};
    std::uint16_t next;

    bool match(char ch) { return (cat & char_cat(ch)) || (!cat && ch == chr); }
};

struct Node {
    Result result{Result::None};
    tok::Type value;
    std::uint16_t first_edge;
};

} // namespace dfa

class Dfa {
public:
    Dfa();

    dfa::Result step(char ch);

    const tok::Type type() { return _type; }

private:
    const unsigned* _start_states;
    const dfa::Node* _nodes;
    const dfa::Edge* _edges;

    std::uint16_t _state;
    tok::Type _type;
};

} // namespace ulam::lex
