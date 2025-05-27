#include "./compiler.hpp"
#include "./out.hpp"
#include "libulam/ast/nodes/init.hpp"
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

    auto obj = eval.eval(text);
    assert(obj);
    assert(obj.type()->is_class());
    assert(obj.value().is_rvalue());
    auto test_postfix =
        has_test ? "Int test() {  " + eval.data() + " }" : NoMain;
    write_obj(os, std::move(obj), test_postfix, has_test);
    os << "\n";
}

void Compiler::write_obj(
    std::ostream& os,
    ulam::sema::ExprRes&& obj,
    const std::string& test_postfix,
    bool in_main) {

    assert(obj.type()->is_class());
    auto cls = obj.type()->as_class();
    auto rval = obj.move_value().move_rvalue();

    os << class_name(cls);
    write_class_parents(os, cls);
    os << " { ";
    write_obj_members(os, cls, rval, in_main, true, false, cls);
    os << test_postfix;
    os << " }";
}

void Compiler::write_class_parents(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    auto parents = cls->parents();
    if (parents.size() == 0)
        return;

    Stringifier stringifier{program()};
    os << " : ";
    for (auto anc : parents) {
        if (anc != *parents.begin())
            os << " + ";
        os << out::type_str(stringifier, anc->cls());
    }
}

void Compiler::write_obj_members(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    const ulam::RValue& rval,
    bool in_main,
    bool is_outer,
    bool is_base,
    ulam::Ref<ulam::Class> outer) {
    write_class_type_defs(os, cls);
    write_class_consts(os, cls, in_main, is_outer, is_base, outer);
    write_obj_props(os, cls, rval, in_main, is_outer, is_base, outer);
    if (in_main || !is_base)
        write_obj_parent_members(os, cls, rval, in_main, is_outer, outer);
}

void Compiler::write_obj_parent_members(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    const ulam::RValue& obj,
    bool in_main,
    bool is_outer,
    ulam::Ref<ulam::Class> outer) {
    Stringifier stringifier{program()};
    stringifier.options.use_unsigned_suffix = in_main;
    stringifier.options.bits_use_unsigned_suffix = false;

    for (const auto anc : cls->ancestors()) {
        if (anc->is_implicit())
            continue;
        auto parent = anc->cls();
        std::stringstream buf;
        write_obj_members(buf, parent, obj, in_main, is_outer, true, outer);
        std::string mem_str{std::move(*(buf.rdbuf())).str()};
        if (!mem_str.empty()) {
            auto prefix = anc->is_parent() ? ':' : '^';
            os << prefix << out::type_str(stringifier, parent) << "< "
               << mem_str << "> ";
        }
    }
}

void Compiler::write_class_type_defs(
    std::ostream& os, ulam::Ref<ulam::Class> cls) {
    Stringifier stringifier{program()};
    for (auto type_def : cls->type_defs())
        write_class_type_def(os, stringifier, type_def);
}

void Compiler::write_class_type_def(
    std::ostream& os,
    Stringifier& stringifier,
    ulam::Ref<ulam::AliasType> alias) {
    os << out::type_def_str(stringifier, alias) << "; ";
}

void Compiler::write_class_consts(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    bool in_main,
    bool is_outer,
    bool is_base,
    ulam::Ref<ulam::Class> outer) {
    Stringifier stringifier{program()};
    stringifier.options.use_unsigned_suffix = true;
    stringifier.options.use_unsigned_suffix_31bit = !is_base;
    stringifier.options.bits_use_unsigned_suffix = true;
    stringifier.options.bits_32_as_signed_int = in_main;
    stringifier.options.array_fmt = in_main ? Stringifier::ArrayFmt::Chunks
                                            : Stringifier::ArrayFmt::Default;
    stringifier.options.object_fmt =
        in_main ? Stringifier::ObjectFmt::Chunks : Stringifier::ObjectFmt::Map;

    bool tpl_only =
        !(in_main || is_outer || cls->is_element() ||
          /* (!is_outer && outer->is_element() && !is_base) || */ // t41490
          (is_base && !cls->has_cls_tpl()));

    // params as consts (t3336)
    if (!tpl_only) {
        for (auto var : cls->params())
            write_class_const(os, stringifier, var);
    }

    // consts
    for (auto var : cls->consts()) {
        if (!tpl_only || var->node()->is_in_tpl())
            write_class_const(os, stringifier, var);
    }
}

void Compiler::write_class_const(
    std::ostream& os, Stringifier& stringifier, ulam::Ref<ulam::Var> var) {
    assert(_ast->program());
    auto& str_pool = program()->str_pool();
    os << out::var_str(str_pool, stringifier, var) << "; ";
}

void Compiler::write_obj_props(
    std::ostream& os,
    ulam::Ref<ulam::Class> cls,
    const ulam::RValue& obj,
    bool in_main,
    bool is_outer,
    bool is_base,
    ulam::Ref<ulam::Class> outer) {

    Stringifier stringifier{program()};
    stringifier.options.use_unsigned_suffix = true;
    stringifier.options.use_unsigned_suffix_zero =
        in_main || cls->is_transient() || cls->is_quark();
    stringifier.options.bits_use_unsigned_suffix = cls->is_transient();
    stringifier.options.use_unsigned_suffix_31bit = !is_base;
    stringifier.options.hex_u64_zero_as_int = cls->is_transient();
    stringifier.options.bits_32_as_signed_int = in_main;
    stringifier.options.class_params_as_consts = in_main;

    for (auto prop : cls->props()) {
        // t41298, t41355 hacks
        auto type = prop->type();
        if (type->is_array())
            type = type->as_array()->item_type();
        bool bool_as_unsigned_lit = stringifier.options.bool_as_unsigned_lit;
        bool unary_as_unsigned_lit = stringifier.options.unary_as_unsigned_lit;
        bool use_unsigned_suffix_zero =
            stringifier.options.use_unsigned_suffix_zero;
        bool bits_use_unsigned_suffix =
            stringifier.options.bits_use_unsigned_suffix;

        stringifier.options.bool_as_unsigned_lit = false;
        stringifier.options.unary_as_unsigned_lit =
            !in_main && !prop->type()->is_array();
        stringifier.options.use_unsigned_suffix_zero =
            use_unsigned_suffix_zero &&
            (in_main ||
             (type->is(ulam::UnsignedId) && prop->node()->has_init() &&
              (!type->is_alias() || type->bitsize() <= 8)));
        stringifier.options.bits_use_unsigned_suffix =
            bits_use_unsigned_suffix &&
            (!type->is(ulam::BitsId) || type->bitsize() != 64);

        write_obj_prop(os, stringifier, prop, obj, in_main, outer);

        stringifier.options.bool_as_unsigned_lit = bool_as_unsigned_lit;
        stringifier.options.unary_as_unsigned_lit = unary_as_unsigned_lit;
        stringifier.options.use_unsigned_suffix_zero = use_unsigned_suffix_zero;
        stringifier.options.bits_use_unsigned_suffix = bits_use_unsigned_suffix;
    }
}

void Compiler::write_obj_prop(
    std::ostream& os,
    Stringifier& stringifier,
    ulam::Ref<ulam::Prop> prop,
    const ulam::RValue& obj,
    bool in_main,
    ulam::Ref<ulam::Class> outer) {
    auto& str_pool = program()->str_pool();
    auto type = prop->type();
    auto lval = obj.prop(prop);
    os << out::type_str(stringifier, type, false) << " "
       << str_pool.get(prop->name_id()) << out::type_dim_str(type) << "(";

    lval.with_rvalue([&](const auto& rval) {
        bool empty_string_as_empty = stringifier.options.empty_string_as_empty;
        stringifier.options.empty_string_as_empty = true;

        if (type->is_array()) {
            auto array_type = type->as_array();
            auto item_type = array_type->item_type();
            for (ulam::array_idx_t idx = 0; idx < array_type->array_size();
                 ++idx) {
                const auto item_lval = lval.array_access(idx, true);
                item_lval.with_rvalue([&](const auto& item_rval) {
                    if (item_type->is_class()) {
                        if (idx > 0)
                            os << "), (";
                        write_obj_members(
                            os, item_type->as_class(), item_rval, in_main,
                            false, false, outer);

                    } else {
                        assert(!item_type->is_array());
                        if (idx > 0)
                            os << ", ";
                        os << stringifier.stringify(item_type, item_rval);
                    }
                });
            }
        } else if (type->is_class()) {
            auto node = prop->node();
            if (!in_main && node->has_init() &&
                node->init()->is<ulam::ast::InitMap>()) {
                // output as map if prop has init map
                // NOTE: this doesn't work in general, ULAM tests show only
                // values that are in init map (which we don't have at this
                // point)
                os << stringifier.stringify(prop->type(), rval);
            } else {
                write_obj_members(
                    os, type->as_class(), rval, in_main, false, false, outer);
            }
        } else {
            os << stringifier.stringify(type, rval);
        }

        stringifier.options.empty_string_as_empty = empty_string_as_empty;
    });
    os << "); ";
}

ulam::Ref<ulam::Program> Compiler::program() {
    assert(_ast->program());
    return _ast->program();
}
