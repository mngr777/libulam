#include <cassert>
#include <libulam/semantic/type/prim.hpp>
#include <sstream>

namespace ulam {

// PrimType

PrimType::PrimType(Builtins& builtins, TypeIdGen* id_gen):
    Type{id_gen}, _builtins{builtins} {
    assert(id_gen);
}

std::string PrimType::name() const {
    std::stringstream ss;
    ss << builtin_type_str(builtin_type_id());
    ss << "(" << (Unsigned)bitsize() << ")";
    return ss.str();
}

// PrimTypeTpl

PrimTypeTpl::PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen):
    TypeTpl{id_gen}, _builtins{builtins} {}

} // namespace ulam
