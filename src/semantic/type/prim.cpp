#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

PrimType::PrimType(TypeIdGen& id_gen):
    Type{id_gen} {}

// Ref<Type> PrimType::prim_type(
//     Ref<ast::Node> node, BuiltinTypeId id, bitsize_t bitsize) {
//     return (bitsize == 0) ? program()->prim_type(id)
//                           : program()->prim_type_tpl(id)->type(node, bitsize);
// }

PrimTypeTpl::PrimTypeTpl(TypeIdGen& id_gen):
    TypeTpl{id_gen} {}

} // namespace ulam
