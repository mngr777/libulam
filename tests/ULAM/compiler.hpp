#pragma once
#include "./eval.hpp"
#include <filesystem>
#include <libulam/context.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/parser.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type.hpp>
#include <set>
#include <string>

class Compiler {
public:
    using Path = std::filesystem::path;

    Compiler():
        _ctx{},
        _ast{ulam::make<ulam::ast::Root>()},
        _parser{_ctx, _ast->ctx().str_pool(), _ast->ctx().text_pool()} {}

    void parse_module_file(const Path& path);
    void parse_module_str(const std::string& text, const Path& path);

    void add_str_src(const std::string& text, const Path& path);

    ulam::Ref<ulam::Program> analyze();

    void compile(std::ostream& os);

private:
    void
    compile_class(std::ostream& os, Eval& eval, ulam::Ref<ulam::Class> cls);

    void write_obj(
        std::ostream& os,
        ulam::sema::ExprRes&& obj,
        const std::string& test_postfix,
        bool is_main);

    void write_class_parents(std::ostream& os, ulam::Ref<ulam::Class> cls);

    void write_class_type_defs(std::ostream& os, ulam::Ref<ulam::Class> cls);
    void
    write_class_type_def(std::ostream& os, ulam::Ref<ulam::AliasType> alias);

    void write_class_consts(std::ostream& os, ulam::Ref<ulam::Class> cls);
    void write_class_const(std::ostream& os, ulam::Ref<ulam::Var> var);

    void write_class_props(
        std::ostream& os,
        ulam::Ref<ulam::Class> cls,
        ulam::Value& obj,
        bool is_main);

    void write_class_prop(
        std::ostream& os,
        ulam::Ref<ulam::Prop> prop,
        ulam::Value& obj,
        bool is_main);

    ulam::Context _ctx;
    ulam::Ptr<ulam::ast::Root> _ast;
    ulam::Parser _parser;
    std::set<ulam::str_id_t> _module_name_ids;
};
