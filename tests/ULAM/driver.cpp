#include "./test_case.hpp"
#include <algorithm>
#include <cassert>
#include <exception>
#include <filesystem>
#include <iostream>
#include <set> // TEST
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
    assert(n > 0);
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
        std::set<std::string> skip = {
            "t3205_test_compiler_elementandquark_emptyquark.test", // Empty
            "t3233_test_compiler_elementandquarkarray_ew.test", // aref called by name
            "t3357_test_compiler_elementandquarkswclassargs_lhsconstantcompare.test", // behave()
            "t3358_test_compiler_elementandquarkswclassargs_lhsconstantlogicaland.test", // behave()
            "t3361_test_compiler_elementandquarkswclassargs_memberconstantasfunccallarg.test", // behave()
            "t3400_test_compiler_arraysizeof_lengthzeroelement.test", // Empty
            "t3401_test_compiler_unaryminofmaxofconstant_issue.test", // Empty
            "t3418_test_compiler_namedconstant_unsignedchar.test", // char
            "t3422_test_compiler_namedconstant_specialchars.test", // char
            "t3423_test_compiler_namedconstant_octal.test", // char
            "t3424_test_compiler_namedconstant_hex.test", // char
            "t3449_test_compiler_bitwisefunccallreturns.test", // char
            "t3450_test_compiler_minmaxsizeoffunccallreturns.test", // string
            "t3484_test_compiler_elementandquark_caarray_ambiguousfunc_issue.test", // Empty
            "t3485_test_compiler_voidfuncreturnscastedatom.test", // return (Void) a; ??
            "t3494_test_compiler_divideandmodmixedtypes.test", // carrying consteval flag through props?
        };
        for (unsigned n = 1; n <= test_paths.size(); ++n) {
            if (skip.count(test_paths[n - 1].filename()) > 0)
                continue;
            if (!run(n, test_paths))
                break;
        }
    } else {
        if (case_num <= test_paths.size()) {
            run(case_num, test_paths);
        } else {
            std::cout << "case not found\n";
        }
    }
}
