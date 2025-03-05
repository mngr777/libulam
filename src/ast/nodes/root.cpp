#include <cassert>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam::ast {

// Root

Root::Root() {}
Root::~Root() {} // for Program destructor

bool Root::has_module(str_id_t name_id) const { return _name_id_map.has(name_id); }

void Root::add_module(Ptr<ModuleDef>&& mod) {
    assert(!_name_id_map.has(mod->name_id()));
    _name_id_map.add(mod->name_id(), ref(mod));
    add(std::move(mod));
}

} // namespace ulam::ast
