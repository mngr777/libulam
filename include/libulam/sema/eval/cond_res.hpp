#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/memory/ptr.hpp>
#include <utility>

namespace ulam::sema {

class AsCondContext {
public:
    AsCondContext(Ptr<ast::VarDef>&& var_def, Ref<Var> var):
        _var_def{std::move(var_def)}, _var{var} {
        assert(_var_def && _var);
    }
    AsCondContext(): _var_def{}, _var{} {}

    AsCondContext(AsCondContext&&) = default;
    AsCondContext& operator=(AsCondContext&&) = default;

    bool has_var() const { return _var; }
    Ref<Var> var() const { return _var; }

private:
    Ptr<ast::VarDef> _var_def;
    Ref<Var> _var;
};

using CondRes = std::pair<bool, AsCondContext>;

} // namespace ulam::sema
