#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <utility>

namespace ulam::sema {

class AsCondContext {
public:
    AsCondContext(Ref<Type> type, Ptr<ast::VarDef>&& var_def, Ref<Var> var):
        _type{type}, _var_def{std::move(var_def)}, _var{var} {
        assert(_type);
    }
    AsCondContext(Ref<Type> type): _type{type}, _var_def{}, _var{} {
        assert(_type);
    }
    AsCondContext(): _type{}, _var_def{}, _var{} {}

    AsCondContext(AsCondContext&&) = default;
    AsCondContext& operator=(AsCondContext&&) = default;

    bool empty() const { return !_type; }

    Ref<Type> type() const { return _type; }

    bool has_var() const { return _var; }
    Ref<Var> var() const { return _var; }

private:
    Ref<Type> _type;
    Ptr<ast::VarDef> _var_def;
    Ref<Var> _var;
};

using CondRes = std::pair<bool, AsCondContext>;

} // namespace ulam::sema
