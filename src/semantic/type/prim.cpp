#include <cassert>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

PrimType::PrimType(TypeIdGen* id_gen): Type{id_gen} { assert(id_gen); }

PrimTypeTpl::PrimTypeTpl(TypeIdGen& id_gen): TypeTpl{id_gen} {}

} // namespace ulam
