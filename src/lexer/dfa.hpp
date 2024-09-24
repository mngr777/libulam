#pragma once
#include "libulam/token.hpp"
#include <cstdint>

namespace ulam::lex {
namespace dfa {

using ClassFlags = std::uint8_t;

namespace cls {
constexpr ClassFlags None = 0;
constexpr ClassFlags Any = ~0;
constexpr ClassFlags Alpha = 1 << 0;
constexpr ClassFlags Digit = 1 << 1;
constexpr ClassFlags WordNonAlnum = 1 << 2; // non-alnum word chars i.e. '_'
constexpr ClassFlags Space = 1 << 3;
constexpr ClassFlags Other = 1 << 4;
constexpr ClassFlags Word = Alpha | Digit | WordNonAlnum;
constexpr ClassFlags Alnum = Alpha | Digit;
} // namespace cls

ClassFlags char_class(char ch);

struct Edge {
    char chr{'\0'};
    ClassFlags cls{cls::None};
    std::uint16_t next;

    bool match(char ch) {
        return (cls & char_class(ch)) || (!cls && ch == chr);
    }
};

struct Node {
    bool is_final;
    tok::Type value;
    std::uint16_t first_edge;
};

} // namespace dfa

class Dfa {
public:
    enum Result { None, TokenStart, TokenEnd };

    Dfa();

    Result step(char ch);

    const tok::Type type() { return _type; }

private:
    const unsigned* _start_states;
    const dfa::Node* _nodes;
    const dfa::Edge* _edges;

    std::uint16_t _state;
    tok::Type _type;
};

} // namespace ulam::lex
