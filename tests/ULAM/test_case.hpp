#pragma once
#include "./answer.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

class TestCase {
public:
    using Path = std::filesystem::path;

    using flags_t = std::uint8_t;
    static constexpr flags_t NoFlags = 0;
    static constexpr flags_t SkipAnswerCheck = 1;

    TestCase(const Path& stdlib_dir, const Path& path, flags_t flags = NoFlags);

    bool run();

private:
    void load(const Path& path);
    void parse();

    void add_src(Path path, const std::string_view text);

    Path _stdlib_dir;
    std::string _text;
    std::string_view _answers_text;
    AnswerMap _answers;
    std::vector<int> _exit_statuses;
    // {filename, text}
    std::vector<std::pair<Path, std::string_view>> _srcs;
    std::vector<std::pair<Path, std::string_view>> _inc_srcs;
    flags_t _flags;
};
