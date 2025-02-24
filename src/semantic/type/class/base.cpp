#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class/base.hpp>

namespace ulam {

ClassBase::ClassBase(
    Ref<ast::ClassDef> node, Ref<Scope> module_scope, ScopeFlags scope_flags):
    _node{node},
    _param_scope{make<PersScope>(module_scope)},
    _inh_scope{make<PersScope>(ref(_param_scope))},
    _scope{make<PersScope>(ref(_inh_scope), scope_flags)} {
    assert(module_scope);
    assert(module_scope->is(scp::Module));
}

ClassKind ClassBase::kind() const { return node()->kind(); }

} // namespace ulam
