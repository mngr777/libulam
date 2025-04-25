#include "./test_case.hpp"
#include "./compiler.hpp"
#include "./test_case/parser.hpp"
#include <cassert>
#include <fstream>
#include <ios>
#include <iostream> // TEST
#include <libulam/sema/eval/except.hpp>
#include <sstream>
#include <stdexcept>

namespace {

std::map<std::string, Answer> parse_answers(const std::string_view text) {
    std::map<std::string, Answer> answers;
    std::size_t pos{0};
    while (pos < text.size()) {
        auto nl_pos = text.find('\n', pos);
        auto line = (nl_pos != std::string::npos)
                        ? text.substr(pos, nl_pos - pos)
                        : text.substr(pos);
        pos += line.size() + 1;
        auto answer = parse_answer(line);
        if (answer.is_tpl())
            continue; // TODO: compare template postfix
        auto name = answer.class_name();
        assert(answers.count(name) == 0);
        answers.emplace(std::move(name), std::move(answer));
    }
    return answers;
}

} // namespace

TestCase::TestCase(const Path& stdlib_dir, const Path& path, flags_t flags):
    _stdlib_dir{stdlib_dir}, _flags{flags} {
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

    bool ok = false;
    try {
        // compile
        compiler.compile(out);
        auto compiled = out.str();
        std::cout << "COMPILED (raw):\n" << compiled << "\n";
        AnswerMap answers = parse_answers(compiled);
        std::cout << "COMPILED (parsed):\n" << answers << "\n";

        // check
        if (!(_flags & SkipAnswerCheck))
            compare_answer_maps(_answers, answers);
        ok = true;

    } catch (const ulam::sema::EvalExceptError& e) {
        std::cout << "eval error: " << e.message() << "\n";

    } catch (const std::exception& e) {
        std::cout << "error: " << e.what() << "\n";
    }

    std::cout << "\nANSWER (raw):\n" << _answers_text << "\n";
    std::cout << "ANSWER (parsed:)\n" << _answers << "\n";

    if (!ok)
        throw std::invalid_argument("test case failed");
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
    TestCaseParser p{_text};

    // answer mark
    p.move_to("#!");
    p.skip_line();
    p.skip_comments();

    // "Exit status: <status>\n"
    // p.move_to("Exit status:");
    while (p.at("Exit status:")) {
        p.skip("Exit status:");
        p.skip_spaces();
        _exit_statuses.push_back(p.read_int());
        p.skip('\n');
    }

    // parse answers
    {
        auto start = p.pos();
        p.move_to('#');
        if (!(_flags & SkipAnswerCheck)) {
            _answers_text = p.substr_from(start);
            _answers = parse_answers(_answers_text);
        }
    }
    p.skip_comments();

    while (true) {
        // name
        p.move_to_file_name();
        auto name = p.read_file_name();
        p.skip_line();

        // src
        auto src_start = p.pos();
        p.move_to("\n#"); // # at start of line
        auto src = p.substr_from(src_start);
        add_src(name, src);
        p.skip('\n'); // at #

        p.skip_comments();
        if (p.at("#.")) // end marker
            break;
    }
}

void TestCase::add_src(Path path, const std::string_view text) {
    auto ext = path.extension();
    if (ext == ".ulam") {
        _srcs.emplace_back(std::move(path), text);
    } else if (ext == ".inc") {
        _inc_srcs.emplace_back(std::move(path), text);
    } else {
        throw std::invalid_argument(
            "source file extension must be `.ulam' or `.inc'");
    }
}
