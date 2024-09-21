#include "dfagen/graph.hpp"
#include "libulam/token.hpp"
#include <iostream>

int main() {
    Graph graph;

#define TOK(str, type) if (str) { graph.add(str, ulam::tok::type); }
#define TOK_SEL_ALL
#include "libulam/token.inc.hpp"
#undef TOK_SEL_ALL
#undef TOK

    try {
        graph.gen(std::cout);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return -1;
    }
}
