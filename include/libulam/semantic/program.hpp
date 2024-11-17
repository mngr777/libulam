#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <list>

namespace ulam {

class TypeIdGen {
public:
    type_id_t next() { return _next++; }

private:
    type_id_t _next{1};
};

class Program {
public:
    void add_module(Ptr<Module>&& module);

    type_id_t next_type_id() { return _type_id_gen.next(); }
    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    TypeIdGen _type_id_gen;
    std::list<Ptr<Module>> _modules;
};

} // namespace ulam
