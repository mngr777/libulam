#include "tests/ULAM/test_case.hpp"
#include "tests/ULAM/compiler.hpp"
#include <cassert>
#include <fstream>
#include <ios>
#include <sstream>
#include <stdexcept>

TestCase::TestCase(const std::filesystem::path& path) {
    load(path);
    parse();
}

void TestCase::run() {
    assert(_srcs.size() > 0);
    std::stringstream out;
    Compiler compiler;
    for (auto pair : _srcs) {
        auto [name, text] = pair;
        compiler.parse_module_str(std::string{text}, std::string{name});
    }
    // analyze
    auto program = compiler.analyze();
    if (!program)
        throw std::invalid_argument("failed to analyze program");
    compiler.compile(out, _srcs[0].first);
}

void TestCase::load(const std::filesystem::path& path) {
    if (std::ifstream is{path}) {
        is.seekg(0, std::ios_base::end);
        std::size_t size = is.tellg();
        std::string text(size + 1, '\0');
        is.seekg(0);
        if (is.read(&text[0], size)) {
            std::swap(_text, text);
            return;
        }
    }
    throw std::invalid_argument("failed to read input file");
}

void TestCase::parse() {
    std::size_t pos{0}; // , end_pos{0};
    std::string_view text{_text};

    auto error = [&](const std::string& str) {
        throw std::invalid_argument(
            str + ", around (chr:" + std::to_string(pos) + ") '" +
            std::string{text.substr(pos, 16)} + "'");
    };

    auto move_to = [&](const std::string& str) {
        auto n = text.find(str, pos);
        if (n == std::string::npos)
            error(str + " not found");
        pos = n;
    };

    auto read_until = [&](const std::string& str) -> std::string_view {
        auto n = text.find(str, pos);
        if (n == std::string::npos)
            error(str + " not found");
        auto sub = text.substr(pos, (n - pos));
        pos = n;
        return sub;
    };

    auto skip_spaces = [&]() {
        while (text[pos] == ' ')
            ++pos;
    };

    auto skip_line = [&]() {
        move_to("\n");
        ++pos;
    };

    auto skip_comments = [&]() {
        while (text[pos] == '#' && text[pos + 1] == '#')
            skip_line();
    };

    auto is_digit = [&]() { return '0' <= text[pos] && text[pos] <= '9'; };
    auto is_upper = [&]() { return 'A' <= text[pos] && text[pos] <= 'Z'; };
    auto is_lower = [&]() { return 'a' <= text[pos] && text[pos] <= 'z'; };
    auto is_alpha = [&]() { return is_upper() || is_lower(); };
    auto is_alnum = [&]() { return is_alpha() || is_digit(); };

    auto skip = [&](const std::string& str) {
        auto n = pos;
        for (std::size_t i = 0; i < str.size(); ++i)
            if (text[n++] != str[i])
                error(str + " not found");
        pos = n;
    };

    auto read_int = [&]() -> int {
        int sign = 1;
        if (text[pos] == '-') {
            sign = -1;
            ++pos;
        }
        if (!is_digit())
            error("not an integer");
        int value = 0;
        while (is_digit())
            value = value * 10 + (text[pos++] - '0');
        return sign * value;
    };

    // NOTE: ModuleName.ulam without extension
    auto read_file_name = [&]() -> std::string_view {
        if (!is_upper())
            error("file name must start with upper case letter");
        auto start = pos++;
        while (is_alnum())
            ++pos;
        auto name = text.substr(start, pos - start);
        skip(".ulam");
        return name;
    };

    // answer mark
    move_to("#!");
    skip("#!\n");

    // "Exit status: <status>\n"
    skip("Exit status:");
    skip_spaces();
    _exit_status = read_int();
    skip("\n");

    // answer
    _answer = read_until("#");
    // skip_comments();

    // main file
    move_to("#>");
    skip("#>");
    skip_spaces();
    while (true) {
        auto name = read_file_name();
        skip_spaces();
        skip("\n");
        auto src = read_until("#");
        _srcs.emplace_back(name, src);
        skip_comments();
        if (text[pos] == '#' && text[pos + 1] == '.')
            break; // #.
        // next file
        move_to("#:");
        skip("#:");
        skip_spaces();
    }
}
