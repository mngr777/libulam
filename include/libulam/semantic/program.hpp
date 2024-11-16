#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <list>

namespace ulam {

class Program {
public:
    void add_module(Ptr<Module>&& module);

    type_id_t next_type_id() { return _next_type_id++; }

private:
    type_id_t _next_type_id{1};
    std::list<Ptr<Module>> _modules;
};

} // namespace ulam
