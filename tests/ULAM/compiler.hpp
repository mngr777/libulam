#pragma once
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

    void compile(std::ostream& out);

private:
    void write_obj(
        std::ostream& out,
        ulam::sema::ExprRes&& obj,
        const std::string& test_postfix);

    void write_class_type_defs(std::ostream& out, ulam::Ref<ulam::Class> cls);
    void write_class_type_def(
        std::ostream& out, ulam::Ref<ulam::AliasType> alias_type);

    void write_class_props(
        std::ostream& out, ulam::Ref<ulam::Class> cls, ulam::Value& obj);
    void write_class_prop(
        std::ostream& out, ulam::Ref<ulam::Prop> prop, ulam::Value& obj);

    ulam::Context _ctx;
    ulam::Ptr<ulam::ast::Root> _ast;
    ulam::Parser _parser;
    std::set<ulam::str_id_t> _module_name_ids;
};
