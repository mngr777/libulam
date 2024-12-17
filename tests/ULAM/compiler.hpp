#pragma once
#include <libulam/context.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/parser.hpp>
#include <libulam/semantic/program.hpp>
#include <string>

class Compiler {
public:
    Compiler(): _ctx{}, _parser{_ctx} {}

    void parse_string(const std::string& text, const std::string& name);
    ulam::Ref<ulam::Program> analyze();
    void compile(std::ostream& out);

private:
    ulam::Context _ctx;
    ulam::Parser _parser;
};
