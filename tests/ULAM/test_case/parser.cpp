#include "./parser.hpp"

void TestCaseParser::move_to_file_name() {
    while (true) {
        move_to('#', SearchOrEof);
        advance();
        if (eof() || at('.'))
            error("file name not found");

        // NOTE: main file is marked with #>, but is not necessarily the first
        // one do we need to know which is main?
        if (at(':') || at('>')) {
            advance();
            skip_spaces();
            break;
        }
    }
}

const std::string_view TestCaseParser::read_file_name() {
    if (!at_upper())
        error("file name must start with upper case letter");
    auto start = pos();
    while (at_alnum() || at('_') || at('.'))
        advance();
    return substr_from(start);
}

void TestCaseParser::skip_comments() {
    while (at("##"))
        skip_line();
}
