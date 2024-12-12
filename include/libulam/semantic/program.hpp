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
    Program(Diag& diag, Ref<ast::Root> ast);
    ~Program();

    Diag& diag() { return _diag; }

    Ref<ast::Root> ast() { return _ast; }
    Ref<const ast::Root> ast() const { return _ast; }

    auto& modules() { return _modules; }

    auto& builtins() { return _builtins; }

    // TODO: refactoring?
    str_id_t self_str_id();
    str_id_t self_inst_str_id();

    Ref<Module> module(module_id_t id);
    Ref<Module> add_module(Ref<ast::ModuleDef> node);

    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    Diag& _diag;
    Ref<ast::Root> _ast;
    std::vector<Ptr<Module>> _modules;
    TypeIdGen _type_id_gen;
    Builtins _builtins;
};

} // namespace ulam
