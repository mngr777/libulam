#pragma once
#include "./answer.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

class TestCase {
public:
    using Path = std::filesystem::path;

    using run_flags_t = std::uint8_t;
    static constexpr run_flags_t NoRunFlags = 0;
    static constexpr run_flags_t SkipAnswerCheck = 1;

    TestCase(const Path& stdlib_dir, const Path& path);

    void run(run_flags_t flags = NoRunFlags);

private:
    void load(const Path& path);
    void parse();

    Path _stdlib_dir;
    std::string _text;
    AnswerMap _answers;
    int _exit_status{0};
    // {filename, text}
    std::vector<std::pair<Path, std::string_view>> _srcs;
    std::vector<std::pair<Path, std::string_view>> _inc_srcs;
};
