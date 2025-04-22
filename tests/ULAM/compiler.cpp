#include "./compiler.hpp"
#include "./out.hpp"
#include "tests/ast/print.hpp"
#include <iostream> // TEST
#include <libulam/sema.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <stdexcept>
#include <utility>

namespace {

constexpr char NoMain[] = "<NOMAIN>";

bool is_urself(ulam::Ref<const ulam::Class> cls) {
    return cls->name() == "UrSelf";
}

std::string class_prefix(ulam::ClassKind kind) {
    switch (kind) {
    case ulam::ClassKind::Element:
        return "Ue_";
    case ulam::ClassKind::Transient:
        return "Un_";
    default:
        return "Uq_";
    }
}

std::string class_name(ulam::Ref<ulam::Class> cls) {
    return class_prefix(cls->kind()) + cls->name();
}

std::string class_tpl_name(ulam::Ref<ulam::ClassTpl> tpl) {
    return class_prefix(tpl->kind()) + tpl->name();
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

        // TODO
        // for (auto tpl : module->class_tpls())
        //     compile_class_tpl(os, eval, tpl);
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
            has_test ? "Int test() {  " + eval.data() + " }" : NoMain;
        write_obj(os, std::move(obj), test_postfix, has_test);
        os << "\n";

    } catch (ulam::sema::EvalExceptError& e) {
        std::cerr << "eval error: " << e.message() << "\n";
        throw e;
    }
}

void Compiler::compile_class_tpl(
    std::ostream& os, Eval& eval, ulam::Ref<ulam::ClassTpl> tpl) {
    os << class_tpl_name(tpl);
    write_class_tpl_params(os, eval, tpl);
    os << " { ";
    os << " " << NoMain << " }\n";
}

void Compiler::write_class_tpl_params(
    std::ostream& os, Eval& eval, ulam::Ref<ulam::ClassTpl> tpl) {
    assert(!tpl->params().empty());
    os << "(";
    Stringifier stringifier{program()};
    const auto& params = tpl->params();
    for (auto param : params) {
        if (param != *params.begin())
            os << ", ";
        // os << out::var_def_str(program()->str_pool(), stringifier, param);
    }
    os << ")";
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
    write_class_parent_members(os, cls, val, is_main);
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
        auto parent = anc->cls();
        if (is_urself(parent))
            continue;
        if (!first)
            os << " + ";
        os << anc->cls()->name();
        first = false;
    }
}

void Compiler::write_class_parent_members(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    ulam::Value& obj,
    bool is_main) {

    Stringifier stringifier{program()};
    stringifier.options.use_unsigned_suffix = is_main;
    stringifier.options.bits_use_unsigned_suffix = false;

    for (auto anc : cls->parents()) {
        auto parent = anc->cls();
        if (is_urself(parent) ||
            (parent->type_defs().empty() && parent->consts().empty() &&
             parent->props().empty()))
            continue;

        os << ":" << out::type_str(stringifier, parent) << "< ";
        write_class_type_defs(os, parent);
        write_class_consts(os, parent);
        write_class_props(os, parent, obj, is_main);
        os << "> ";
    }
}

void Compiler::write_class_type_defs(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    Stringifier stringifier{program()};
    for (auto type_def : cls->type_defs())
        write_class_type_def(os, stringifier, type_def);
}

void Compiler::write_class_type_def(
    std::ostream& os, Stringifier& stringifier, ulam::Ref<ulam::AliasType> alias) {
    os << out::type_def_str(stringifier, alias) << "; ";
}

void Compiler::write_class_consts(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    Stringifier stringifier{program()};
    stringifier.options.use_unsigned_suffix = true;
    stringifier.options.bits_use_unsigned_suffix = false;

    // params as consts (t3336)
    for (auto var : cls->params())
        write_class_const(os, stringifier, var);

    // consts
    for (auto var : cls->consts())
        write_class_const(os, stringifier, var);
}

void Compiler::write_class_const(
    std::ostream& os, Stringifier& stringifier, ulam::Ref<ulam::Var> var) {
    assert(_ast->program());
    auto& str_pool = program()->str_pool();
    os << out::var_str(str_pool, stringifier, var) << "; ";
}

void Compiler::write_class_props(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    ulam::Value& obj,
    bool is_main) {

    Stringifier stringifier{program()};
    stringifier.options.use_unsigned_suffix = is_main;
    stringifier.options.bits_use_unsigned_suffix = false;

    for (auto prop : cls->props())
        write_class_prop(os, stringifier, prop, obj);
}

void Compiler::write_class_prop(
    std::ostream& os,
    Stringifier& stringifier,
    ulam::Ref<ulam::Prop> prop,
    ulam::Value& obj) {
    auto& str_pool = program()->str_pool();
    auto rval_copy = obj.copy_rvalue(); // TMP
    os << out::prop_str(str_pool, stringifier, prop, rval_copy) << "; ";
}

ulam::Ref<ulam::Program> Compiler::program() {
    assert(_ast->program());
    return _ast->program();
}
