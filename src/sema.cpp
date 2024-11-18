#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam {

Sema::Sema(Diag& diag): _diag{diag} {}

Sema::~Sema() {}

void Sema::analyze(ast::Ref<ast::Root> ast) {
    sema::Init init{_diag, ast};
    init.analyze();
}

} // namespace ulam
