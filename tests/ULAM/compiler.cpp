#include "./compiler.hpp"
#include "./eval/stringifier.hpp"
#include "./out.hpp"
#include "tests/ast/print.hpp"
#include <iostream> // TEST
#include <libulam/sema.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <stdexcept>
#include <utility>

namespace {

std::string class_prefix(ulam::Ref<ulam::Class> cls) {
    switch (cls->kind()) {
    case ulam::ClassKind::Element:
        return "Ue_";
    case ulam::ClassKind::Transient:
        return "Un_";
    default:
        return "Uq_";
    }
}

std::string class_name(ulam::Ref<ulam::Class> cls) {
    return class_prefix(cls) + cls->name();
}

} // namespace

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
        // NOTE: only string modules to be compiled: all files are from stdlib
        _module_name_ids.insert(module->name_id());
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

void Compiler::compile(std::ostream& os) {
    Eval eval{_ctx, ulam::ref(_ast)};
    for (auto module : _ast->program()->modules()) {
        if (_module_name_ids.count(module->name_id()) == 0)
            continue;

        auto sym = module->get(module->name_id());
        if (!sym) {
            throw std::invalid_argument(
                std::string{"no main class in module "} +
                std::string{module->name()});
        }
        assert(sym->is<ulam::Class>() || sym->is<ulam::ClassTpl>());

        for (auto cls : module->classes())
            compile_class(os, eval, cls);
    }
}

void Compiler::compile_class(
    std::ostream& os, Eval& eval, ulam::Ref<ulam::Class> cls) {
    bool has_test = cls->has_fun("test");

    auto text = cls->name() + " foo; ";
    if (has_test)
        text += "foo.test(); ";
    text += "foo;\n";

    try {
        auto obj = eval.eval(text);
        assert(obj);
        assert(obj.type()->is_class());
        assert(obj.value().is_rvalue());
        // NOTE: intentional double space after `{'
        auto test_postfix =
            has_test ? "Int test() {  " + eval.data() + " }" : "<NOMAIN>";
        write_obj(os, std::move(obj), test_postfix, has_test);
        os << "\n";

    } catch (ulam::sema::EvalExceptError& e) {
        std::cerr << "eval error: " << e.message() << "\n";
        throw e;
    }
}

void Compiler::write_obj(
    std::ostream& os,
    ulam::sema::ExprRes&& obj,
    const std::string& test_postfix,
    bool is_main) {
    assert(obj.type()->is_class());
    auto cls = obj.type()->as_class();
    auto val = obj.move_value();

    os << class_name(cls);
    write_class_parents(os, cls);
    os << " { ";
    write_class_type_defs(os, cls);
    write_class_consts(os, cls);
    write_class_props(os, cls, val, is_main);
    os << test_postfix << " }";
}

void Compiler::write_class_parents(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    auto parents = cls->parents();
    if (parents.size() < 2)
        return; // ignoring UrSelf

    os << " : ";
    bool first = true;
    for (auto anc : parents) {
        if (!first)
            os << " + ";
        auto name = anc->cls()->name();
        if (name != "UrSelf") {
            os << name;
            first = false;
        }
    }
}

void Compiler::write_class_type_defs(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    for (auto type_def : cls->type_defs())
        write_class_type_def(os, type_def);
}

void Compiler::write_class_type_def(
    std::ostream& os, ulam::Ref<ulam::AliasType> alias) {
    os << out::type_def_str(alias) << "; ";
}

void Compiler::write_class_consts(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    for (auto var : cls->consts())
        write_class_const(os, var);
}

void Compiler::write_class_const(std::ostream& os, ulam::Ref<ulam::Var> var) {
    assert(_ast->program());
    auto program = _ast->program();
    auto& str_pool = program->str_pool();

    Stringifier stringifier{program};
    stringifier.options.use_unsigned_suffix = true;
    stringifier.options.bits_use_unsigned_suffix = false;

    os << out::var_str(str_pool, stringifier, var) << "; ";
}

void Compiler::write_class_props(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    ulam::Value& obj,
    bool is_main) {
    for (auto prop : cls->props())
        write_class_prop(os, prop, obj, is_main);

    for (auto parent : cls->parents()) {
        auto props = parent->cls()->props();
        if (props.empty())
            continue;
        os << ":" << parent->cls()->name() << "< ";
        for (auto prop : props)
            write_class_prop(os, prop, obj, is_main);
        os << "> ";
    }
}

void Compiler::write_class_prop(
    std::ostream& os,
    ulam::Ref<ulam::Prop> prop,
    ulam::Value& obj,
    bool is_main) {
    assert(_ast->program());
    auto program = _ast->program();
    auto& str_pool = program->str_pool();

    Stringifier stringifier{program};
    stringifier.options.use_unsigned_suffix = is_main;
    stringifier.options.bits_use_unsigned_suffix = false;

    auto rval_copy = obj.copy_rvalue(); // TMP
    os << out::prop_str(str_pool, stringifier, prop, rval_copy) << "; ";
}
