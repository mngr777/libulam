#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/resolve_deps.hpp>
#include <libulam/semantic/program.hpp>

namespace ulam {

Sema::Sema(Diag& diag): _diag{diag} {}

Sema::~Sema() {}

void Sema::analyze(ast::Ref<ast::Root> ast) {
    sema::ResolveDeps resolve_deps{_diag, ast};
    resolve_deps.analyze();
}

} // namespace ulam
