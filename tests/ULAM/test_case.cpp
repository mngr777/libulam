#include "tests/ULAM/test_case.hpp"
#include "tests/ULAM/compiler.hpp"
#include <cassert>
#include <fstream>
#include <ios>
#include <sstream>
#include <stdexcept>

TestCase::TestCase(const Path& stdlib_dir, const Path& path):
    _stdlib_dir{stdlib_dir} {
    load(path);
    parse();
}

void TestCase::run() {
    assert(_srcs.size() > 0);
    std::stringstream out;
    Compiler compiler;

    // add .inc srcs
    for (auto [path, text] : _inc_srcs)
        compiler.add_str_src(std::string{text}, path);

    // parse .ulam srcs
    bool has_empty = false;
    for (auto [path, text] : _srcs) {
        has_empty = has_empty || path.filename() == "Empty.ulam";
        compiler.parse_module_str(std::string{text}, path);
    }

    // stdlib
    compiler.parse_module_file(_stdlib_dir / "UrSelf.ulam");
    if (!has_empty)
        compiler.parse_module_file(_stdlib_dir / "Empty.ulam");

    // analyze
    auto program = compiler.analyze();
    if (!program)
        throw std::invalid_argument("failed to analyze program");
    compiler.compile(out);
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

    auto move_to_file_name = [&]() {
        while (true) {
            move_to("#");
            if (text[pos] == '#' &&
                (text[pos + 1] == ':' || text[pos + 1] == '>')) {
                pos += 2;
                break;
            }
            skip("#");
        }
    };

    auto read_file_name = [&]() -> std::string_view {
        if (!is_upper())
            error("file name must start with upper case letter");
        auto start = pos++;
        while (is_alnum() || text[pos] == '_' || text[pos] == '.')
            ++pos;
        return text.substr(start, pos - start);
    };

    auto add_src = [&](Path path, std::string_view text) {
        auto ext = path.extension();
        if (ext == ".ulam") {
            _srcs.emplace_back(std::move(path), text);
        } else if (ext == ".inc") {
            _inc_srcs.emplace_back(std::move(path), text);
        } else {
            error("source file extension must be `.ulam' or `.inc'");
        }
    };

    // answer mark
    move_to("#!");
    skip("#!\n");
    skip_comments();

    // "Exit status: <status>\n"
    move_to("Exit status:");
    skip("Exit status:");
    skip_spaces();
    _exit_status = read_int();
    skip("\n");

    // answer
    _answer = read_until("#");
    skip_comments();

    // NOTE: main file is marked with #>, but is not necessarily the first one
    // do we need to know which is main?

    move_to_file_name();
    skip_spaces();
    while (true) {
        auto name = read_file_name();
        skip_spaces();
        skip("\n");
        auto src = read_until("\n#");
        ++pos;
        add_src(name, src);
        skip_comments();
        if (text[pos] == '#' && text[pos + 1] == '.')
            break; // #.
        // next file
        move_to_file_name();
        skip_spaces();
    }
}
