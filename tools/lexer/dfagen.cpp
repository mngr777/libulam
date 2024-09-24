#include "dfagen/graph.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

static void gen_char_class_table();
static void gen_dfa_data();

static void exit_usage(const char* name) {
    std::cerr << "Usage: " << name << " (char-class-table|dfa-data|all)\n";
    std::exit(-1);
}

int main(int argc, char** argv) {
    if (argc < 2)
        exit_usage(argv[0]);

    std::string action(argv[1]);
    if (action == "all") {
        gen_char_class_table();
        gen_dfa_data();
    } else if (action == "char-class-table") {
        gen_char_class_table();
    } else if (action == "dfa-data") {
        gen_dfa_data();
    } else {
        exit_usage(argv[0]);
    }
}

// <char-class> /* <hex> [ <chr>] */
void gen_char_class_table() {
    std::cout << "ClassFlags CharClassTable[128] = {\n";
    const unsigned ColNum = 4;
    char ch = 0;
    while (true) {
        bool is_last = (ch == 127);
        bool is_last_col = (is_last || (ch + 1) % ColNum == 0);

        std::stringstream ss;
        if (ch == '_') {
            ss << "cls::WordNonAlnum";
        } else if (std::isalpha(ch)) {
            ss << "cls::Alpha";
        } else if (std::isdigit(ch)) {
            ss << "cls::Digit";
        } else if (std::isspace(ch)) {
            ss << "cls::Space";
        } else {
            ss << "cls::Other";
        }
        ss << " /* " << std::hex << std::setw(2) << std::setfill('0')
                  << (int)ch;
        switch (ch) {
        case '\0':
            ss << " \\0";
            break;
        case '\n':
            ss << " \\n";
            break;
        default:
            if (' ' <= ch && ch <= '~')
                ss << " " << ch;
        }
        ss << " */";
        if (!is_last)
            ss << ",";
        std::cout << std::left;
        if (!is_last_col)
            std::cout << std::setw(24);
        std::cout << ss.str();

        if (!is_last) {
            if (is_last_col)
                std::cout << "\n";
            ++ch;
        } else {
            std::cout << "\n";
            break;
        }
    }
    std::cout << "};\n\n";
}

void gen_dfa_data() {
    Graph graph;

#define TOK(str, type)                                                         \
    if (str) {                                                                 \
        graph.add(str, ulam::tok::type);                                       \
    }
#define TOK_SEL_ALL
#include "libulam/token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK

    try {
        graph.gen(std::cout);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        std::exit(-1);
    }
}
