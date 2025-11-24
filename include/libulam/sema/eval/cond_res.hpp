#pragma once
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/stmts.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <utility>

namespace ulam::sema {

// TODO: add ast::AsCond for codegen, see tests/ULAM/eval/which.cpp
class AsCondContext {
public:
    // (a as A), match
    AsCondContext(Ref<Type> type, Ptr<ast::VarDef>&& var_def, Ptr<Var>&& var):
        _type{type}, _var_def{std::move(var_def)}, _var{std::move(var)} {}

    // (self as A), match
    AsCondContext(Ref<Type> type, LValue self):
        _type{type}, _self{self}, _is_self{true} {}

    // not a match
    AsCondContext(Ref<Type> type): _type{type} {}

    // empty
    AsCondContext() {}

    AsCondContext(AsCondContext&&) = default;
    AsCondContext& operator=(AsCondContext&&) = default;

    bool empty() const { return !has_var() && !is_self(); }

    Ref<Type> type() const { return _type; }

    bool has_var() const { return ref(_var); }
    Ref<Var> var() const {
        assert(has_var());
        return ref(_var);
    }

    bool is_self() const { return _is_self; }
    LValue self() const {
        assert(_is_self);
        return _self;
    }

private:
    Ref<Type> _type{};
    Ptr<ast::VarDef> _var_def;
    Ptr<Var> _var;
    LValue _self;
    bool _is_self{false};
};

using CondRes = std::pair<bool, AsCondContext>;

} // namespace ulam::sema
