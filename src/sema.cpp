#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/resolve_deps.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Sema] "
#include "src/debug.hpp"

namespace ulam {

Sema::Sema(Diag& diag): _diag{diag} {}

Sema::~Sema() {}

void Sema::analyze(Ref<ast::Root> ast) {
    sema::ResolveDeps resolve_deps{_diag, ast};
    resolve_deps.analyze();
    assert(ast->program());
    sema::Resolver resolver{ast->program()};
    resolver.resolve();
}

} // namespace ulam
