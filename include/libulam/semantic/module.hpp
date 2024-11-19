#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/import.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>

namespace ulam::ast {
class ModuleDef;
class TypeSpec;
} // namespace ulam::ast

namespace ulam {

class Scope;

class Module {
public:
    Module(ast::Ref<ast::ModuleDef> node);

    void export_classes(Scope* scope);

    auto& classes() { return _classes; }
    const auto& classes() const { return _classes; }

    bool has_class(str_id_t name_id);
    Ref<Class> get_class(str_id_t name_id);
    void add_class(Ptr<Class>&& cls);

    auto& imports() { return _imports; }
    const auto& imports() const { return _imports; }

    void add_import(ast::Ref<ast::TypeSpec> node);

private:
    ast::Ref<ast::ModuleDef> _node;
    std::unordered_map<str_id_t, Ptr<Class>> _classes;
    std::unordered_map<str_id_t, Import> _imports;
};

} // namespace ulam
