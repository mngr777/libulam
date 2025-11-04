#pragma once
#include "libulam/semantic/scope/options.hpp"
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/export.hpp>
#include <libulam/semantic/mangler.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/type/element.hpp>
#include <libulam/src_mngr.hpp>
#include <libulam/str_pool.hpp>
#include <list>
#include <map>

namespace ulam::ast {
class Root;
class ModuleDef;
} // namespace ulam::ast

namespace ulam {

class Program {
public:
    using ModuleList = std::list<Ref<Module>>;

    Program(
        Diag& diag, UniqStrPool& str_pool, UniqStrPool& text_pool, SrcMngr& sm);
    ~Program();

    Diag& diag() { return _diag; }

    UniqStrPool& str_pool() { return _str_pool; }
    UniqStrPool& text_pool() { return _text_pool; }

    SrcMngr& sm() { return _sm; }

    TypeIdGen& type_id_gen() { return _type_id_gen; }

    ElementRegistry& elements() { return _elements; }

    auto& builtins() { return _builtins; }

    Mangler& mangler() { return _mangler; }

    const ScopeOptions& scope_options() const;
    ScopeOptions set_scope_options(ScopeOptions options);

    const ModuleList& modules() { return _modules; }
    Ref<Module> module(const std::string_view name);
    Ref<Module> module(str_id_t name_id);
    Ref<Module> add_module(Ref<ast::ModuleDef> node);

    const ExportTable& exports() { return _exports; }
    const Export* add_export(str_id_t name_id, Export exp);

private:
    Diag& _diag;
    UniqStrPool& _str_pool;
    UniqStrPool& _text_pool;
    SrcMngr& _sm;
    TypeIdGen _type_id_gen;
    ElementRegistry _elements;
    Builtins _builtins;
    Mangler _mangler;

    struct {
        ScopeOptions scope_options{DefaultScopeOptions};
    } _options;

    std::list<Ptr<Module>> _module_ptrs;
    std::list<Ref<Module>> _modules;
    std::map<str_id_t, Ref<Module>> _modules_by_name_id;
    ExportTable _exports;
};

} // namespace ulam
