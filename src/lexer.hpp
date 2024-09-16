#pragma once
#include <memory_resource>
#include <string_view>
#include "libulam/token.hpp"

namespace ulam {

class Context;
class Source;

class Lexer {
public:
    Lexer(Context& ctx, Source& src, std::pmr::memory_resource* res):
        _ctx(ctx), _src(src) {}

    Lexer& operator>>(Token& token);
    bool next();
    Token get();
    const Token& peek();

private:
    Context& _ctx;
    Source& _src;
    Token _cur;
    std::string_view _line;
};

} // namespace ulam

