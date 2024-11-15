#include <libulam/semantic/program.hpp>
#include <utility>

namespace ulam {

void Program::add_module(Ptr<Module>&& module) {
    _modules.push_back(std::move(module));
}

} // namespace ulam
