#include "libulam/context.hpp"
#include "src/meta/parser.hpp"
#include <iostream>

static const char* MetaComment1 = R"END(/**
    Test description

    \sa EventWindow
    \sa WindowServices
    \author Dave Ackley
    \version 2
    \license public-domain
*/)END"; // no \n at the end

int main() {
    ulam::Context ctx;

    // create source just for diagnostics
    auto src = ctx.src_man().string(MetaComment1, "comment.txt");

    ulam::SrcLoc loc{src->id(), 1, 1};
    ulam::meta::Parser parser{ctx, loc, MetaComment1};
    auto meta = parser.parse();

    std::cout << meta << "\n";
}
