#pragma once
#include <filesystem>
#include <vector>

class TestCase {
public:
    using Path = std::filesystem::path;

    TestCase(const Path& stdlib_dir, const Path& path);

    void run();

private:
    void load(const Path& path);
    void parse();

    Path _stdlib_dir;
    std::string _text;
    std::string_view _answer;
    int _exit_status{0};
    // {filename, text}
    std::vector<std::pair<Path, std::string_view>> _srcs;
    std::vector<std::pair<Path, std::string_view>> _inc_srcs;
};
