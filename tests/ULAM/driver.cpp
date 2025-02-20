#include "./test_case.hpp"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

static void exit_usage(std::string name) {
    std::cout << name << " <path-to-ulam-src-root>[ <case-number>]\n";
    std::exit(-1);
}

static bool run(const std::filesystem::path& path) {
    try {
        TestCase test_case{path};
        test_case.run();
        return true;
    } catch (std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
}

static bool run(unsigned n, std::vector<std::filesystem::path> test_paths) {
    auto& path = test_paths[n - 1];
    std::cout << "# " << n << " " << path.filename() << "\n";
    bool ok = true;
    try {
        ok = run(path);
    } catch (std::exception& exc) {
        std::cout << "exception thrown\n";
        ok = false;
    }
    std::cout << "# " << n << " " << path.filename() << " "
              << (ok ? "OK" : "FAIL") << "\n";
    return ok;
}

int main(int argc, char** argv) {
    if (argc < 2)
        exit_usage(argv[0]);
    // ULAM test dir path
    const std::filesystem::path ulam_src_root{argv[1]};
    const std::filesystem::path test_src_dir{
        ulam_src_root / "src" / "test" / "generic" / "safe"};
    // test case number
    unsigned case_num = 0;
    if (argc > 2)
        case_num = std::stoul(argv[2]);

    std::vector<std::filesystem::path> test_paths;
    {
        auto it = std::filesystem::directory_iterator{test_src_dir};
        for (const auto& item : it)
            test_paths.push_back(item.path());
    }
    std::sort(test_paths.begin(), test_paths.end());

    if (case_num == 0) {
        for (unsigned n = 1; n <= test_paths.size(); ++n)
            run(n, test_paths);
    } else {
        if (case_num <= test_paths.size()) {
            run(case_num, test_paths);
        } else {
            std::cout << "case not found\n";
        }
    }
}
