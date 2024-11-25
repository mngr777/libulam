#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type_tpl.hpp>

namespace ulam {

TypeTpl::TypeTpl(Ref<Program> program):
    _program{program}, _id{program->next_type_id()} {}

Diag& TypeTpl::diag() { return _program->diag(); }

} // namespace ulam
