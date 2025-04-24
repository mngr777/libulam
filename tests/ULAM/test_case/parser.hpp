#pragma once
#include "../parser.hpp"
#include <string_view>

class TestCaseParser : public Parser {
public:
    using Parser::Parser;

    void move_to_file_name();
    const std::string_view read_file_name();

    void skip_comments();
    void skip_line() { read_line(); }
};
