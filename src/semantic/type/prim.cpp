#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

PrimType::PrimType(Ref<Program> program):
    BasicType{program->next_type_id()}, _program{program} {}

PrimTypeTpl::PrimTypeTpl(Ref<Program> program):
    TypeTpl{program}, _diag{program->diag()} {}

} // namespace ulam
