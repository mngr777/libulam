#pragma once
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/str_pool.hpp>
#include <vector>

namespace ulam::ast {
class Root;
class ModuleDef;
} // namespace ulam::ast

namespace ulam {

class Program {
public:
    Program(Diag& diag, UniqStrPool& str_pool);
    ~Program();

    Diag& diag() { return _diag; }
    UniqStrPool& str_pool() { return _str_pool; }
    auto& builtins() { return _builtins; }

    auto& modules() { return _modules; }
    Ref<Module> module(module_id_t id);
    Ref<Module> add_module(Ref<ast::ModuleDef> node);

    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    Diag& _diag;
    UniqStrPool& _str_pool;
    Builtins _builtins;
    TypeIdGen _type_id_gen;
    std::vector<Ptr<Module>> _modules;
};

} // namespace ulam
