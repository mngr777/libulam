#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <list>

namespace ulam {

class Program {
public:
    void add_module(Ptr<Module>&& module);

private:
    std::list<Ptr<Module>> _modules;
};

} // namespace ulam
