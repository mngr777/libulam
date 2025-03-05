#include <cassert>
#include <libulam/sema.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Sema] "
#include "src/debug.hpp"

namespace ulam {

Sema::Sema(Diag& diag, SrcMngr& src_mngr): _diag{diag}, _src_mngr{src_mngr} {}

Sema::~Sema() {}

void Sema::analyze(Ref<ast::Root> ast) {
    sema::Init init{_diag, _src_mngr, ast};
    init.analyze();
    assert(ast->program());

    sema::Resolver resolver{ast->program()};
    resolver.resolve();
}

} // namespace ulam
