#pragma once
#include <filesystem>
#include <vector>

class TestCase {
public:
    TestCase(const std::filesystem::path& path);

    void run();

private:
    void load(const std::filesystem::path& path);
    void parse();

    std::string _text;
    std::string_view _answer;
    int _exit_status{0};
    // {name, text}
    std::vector<std::pair<std::string_view, std::string_view>> _srcs;
};
