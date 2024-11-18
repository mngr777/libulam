#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <list>

namespace ulam::ast {
class Root;
}

namespace ulam {

class TypeIdGen {
public:
    type_id_t next() { return _next++; }

private:
    type_id_t _next{1};
};

class Program {
public:
    Program(ast::Ref<ast::Root> ast);

    void add_module(Ptr<Module>&& module);

    Ref<Scope> scope() { return ref(_scope); }

    type_id_t next_type_id() { return _type_id_gen.next(); }
    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    ast::Ref<ast::Root> _ast;
    Ptr<Scope> _scope;
    TypeIdGen _type_id_gen;
    std::list<Ptr<Module>> _modules;
};

} // namespace ulam
