#pragma once
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
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

    Program(Diag& diag, UniqStrPool& str_pool, SrcMngr& src_mngr);
    ~Program();

    Diag& diag() { return _diag; }
    UniqStrPool& str_pool() { return _str_pool; }
    SrcMngr& src_mngr() { return _src_mngr; }
    auto& builtins() { return _builtins; }

    const ModuleList& modules() { return _module_refs; }

    Ref<Module> module(const std::string_view name);
    Ref<Module> module(str_id_t name_id);

    Ref<Module> add_module(Ref<ast::ModuleDef> node);

    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    Diag& _diag;
    UniqStrPool& _str_pool;
    SrcMngr& _src_mngr;
    Builtins _builtins;
    TypeIdGen _type_id_gen;
    std::list<Ptr<Module>> _modules;
    std::list<Ref<Module>> _module_refs;
    std::map<str_id_t, Ref<Module>> _modules_by_name_id;
};

} // namespace ulam
