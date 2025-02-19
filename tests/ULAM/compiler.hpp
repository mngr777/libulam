#pragma once
#include <libulam/context.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/parser.hpp>
#include <libulam/semantic/program.hpp>
#include <string>
#include <string_view>

class Compiler {
public:
    Compiler():
        _ctx{},
        _ast{ulam::make<ulam::ast::Root>()},
        _parser{_ctx, _ast->ctx().str_pool()} {}

    void parse_module_str(const std::string& text, const std::string& name);
    ulam::Ref<ulam::Program> analyze();
    void compile(std::ostream& out, const std::string_view main_name);

private:
    ulam::Context _ctx;
    ulam::Ptr<ulam::ast::Root> _ast;
    ulam::Parser _parser;
};
