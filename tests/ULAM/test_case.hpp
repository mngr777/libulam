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
    // {name, text}
    std::vector<std::pair<std::string_view, std::string_view>> _srcs;
};
