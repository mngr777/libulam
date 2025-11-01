#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>

namespace ulam {

class Fun;
class LValue;

class FunScope : public BasicScope {
public:
    FunScope(Ref<Fun> fun, LValue self, scope_flags_t flags = scp::NoFlags):
        BasicScope{fun->scope(), (scope_flags_t)(flags | scp::Fun | scp::Self)},
        _fun{fun},
        _self{self} {}

    Ref<Fun> fun() const override { return _fun; }

    bool has_self() const override { return true; }
    LValue self() const override { return _self; }

private:
    Ref<Fun> _fun{};
    LValue _self{};
};

} // namespace ulam
