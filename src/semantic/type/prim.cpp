#include <cassert>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

PrimType::PrimType(Builtins& builtins, TypeIdGen* id_gen):
    Type{id_gen}, _builtins{builtins} {
    assert(id_gen);
}

PrimTypeTpl::PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen):
    TypeTpl{id_gen}, _builtins{builtins} {}

} // namespace ulam
