#include "./test_case.hpp"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>

static void exit_usage(std::string name) {
    std::cout << name << " <path-to-ulam-src-root>\n";
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

int main(int argc, char** argv) {
    if (argc < 2)
        exit_usage(argv[0]);
    const std::filesystem::path ulam_src_root{argv[1]};
    const std::filesystem::path test_src_dir{
        ulam_src_root / "src" / "test" / "generic" / "safe"};

    std::vector<std::filesystem::path> test_paths;
    {
        auto it = std::filesystem::directory_iterator{test_src_dir};
        for (const auto& item : it)
            test_paths.push_back(item.path());
    }
    std::sort(test_paths.begin(), test_paths.end());

    for (unsigned n = 1; n <= test_paths.size(); ++n) {
        if (n != 36)
            continue;
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
        if (!ok)
            break;
        // break;
    }
}
