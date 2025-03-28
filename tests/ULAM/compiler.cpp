#include "tests/ULAM/compiler.hpp"
#include "tests/ast/print.hpp"
#include <iostream> // TEST
#include <libulam/sema.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <stdexcept>
#include <utility>

void Compiler::parse_module_file(const Path& path) {
    auto module = _parser.parse_module_file(path);
    std::cerr << "parsing " << path.filename() << "\n";
    if (module) {
        assert(!_ast->has_module(module->name_id()));
        _ast->add_module(std::move(module));
    }
}

void Compiler::parse_module_str(const std::string& text, const Path& path) {
    auto module = _parser.parse_module_str(text, path);
    auto name = path.stem().string();
    std::cerr << "parsing module " << name << "\n";
    if (module) {
        if (_ast->has_module(module->name_id()))
            throw std::invalid_argument{
                std::string{"duplicate module name "} + name};
        _ast->add_module(std::move(module));
    }
}

void Compiler::add_str_src(const std::string& text, const Path& path) {
    _parser.add_str_src(text, path);
}

ulam::Ref<ulam::Program> Compiler::analyze() {
    // TEST {
    test::ast::Printer printer{std::cout, ulam::ref(_ast)};
    printer.print();
    // }
    ulam::Sema sema{_ctx.diag(), _ctx.sm()};
    sema.analyze(ulam::ref(_ast));
    return _ast->program();
}

void Compiler::compile(std::ostream& out) {
    ulam::sema::Eval eval{_ctx, ulam::ref(_ast)};
    for (auto module : _ast->program()->modules()) {
        auto sym = module->get(module->name_id());
        if (!sym) {
            throw std::invalid_argument(
                std::string{"no main class in module "} +
                std::string{module->name()});
        }
        if (sym->is<ulam::Class>()) {
            auto cls = sym->get<ulam::Class>();
            if (cls->has_fun("test")) {
                auto text = std::string{module->name()} + " foo; foo.test();\n";
                try {
                    eval.eval(text);
                } catch (ulam::EvalExceptError& e) {
                    std::cerr << "eval error: " << e.message() << "\n";
                    throw e;
                }
            }
        }
    }
}
