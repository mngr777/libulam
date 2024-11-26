#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

PrimType::PrimType(Ref<Program> program):
    BasicType{program->next_type_id()}, _program{program} {}

Diag& PrimType::diag() {
    return program()->diag();
}

Ref<Type> PrimType::prim_type(
    ast::Ref<ast::Node> node, BuiltinTypeId id, bitsize_t bitsize) {
    return (bitsize == 0) ? program()->prim_type(id)
                          : program()->prim_type_tpl(id)->type(node, bitsize);
}

PrimTypeTpl::PrimTypeTpl(Ref<Program> program):
    TypeTpl{program} {}

} // namespace ulam
