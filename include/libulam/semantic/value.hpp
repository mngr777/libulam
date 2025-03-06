#pragma once
#include <functional>
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>
#include <libulam/semantic/value/data.hpp>
#include <libulam/semantic/value/types.hpp>
#include <list>

namespace ulam {

using scope_lvl_t = std::uint16_t;
constexpr scope_lvl_t NoScopeLvl = -1;
constexpr scope_lvl_t AutoScopeLvl = -2;

class Class;
class FunSet;
class Prop;
class Type;
class TypedValue;
class Var;

class RValue;
class Value;

class LValue : public detail::NullableVariant<Ref<Var>, DataView, BoundFunSet> {
public:
    using Variant::Variant;

    RValue rvalue() const;

    Ref<Type> type() const;

    DataView data_view();
    const DataView data_view() const;

    Ref<Class> dyn_cls() const;
    Ref<Type> dyn_obj_type() const;

    LValue self();
    LValue as(Ref<Type> type);
    LValue atom_of();

    LValue array_access(array_idx_t idx);
    LValue prop(Ref<Prop> prop);
    LValue bound_fset(Ref<FunSet> fset);

    Value assign(RValue&& rval);

    bool is_consteval() const;

    bool is_xvalue() const { return _is_xvalue; }
    void set_is_xvalue(bool is_xvalue) { _is_xvalue = is_xvalue; }

    bool has_scope_lvl() const { return _scope_lvl != NoScopeLvl; }
    bool has_auto_scope_lvl() const { return _scope_lvl == AutoScopeLvl; }
    scope_lvl_t scope_lvl() const { return _scope_lvl; }
    void set_scope_lvl(scope_lvl_t scope_lvl) { _scope_lvl = scope_lvl; }

private:
    template <typename T> LValue derived(T&& value) {
        LValue lval{std::forward<T>(value)};
        lval._is_xvalue = _is_xvalue;
        lval._scope_lvl = _scope_lvl;
        return lval;
    }

    bool _is_xvalue{true};
    scope_lvl_t _scope_lvl{NoScopeLvl};
};

class RValue
    : public detail::NullableVariant<Integer, Unsigned, Bits, String, DataPtr> {
public:
    template <typename T>
    explicit RValue(T&& value, bool is_consteval = false):
        Variant{std::forward<T>(value)}, _is_consteval{is_consteval} {}

    RValue(): Variant{}, _is_consteval{false} {}

    RValue(RValue&&) = default;
    RValue& operator=(RValue&&) = default;

    RValue copy() const;

    DataView data_view();
    const DataView data_view() const;

    Ref<Class> dyn_cls() const;
    Ref<Type> dyn_obj_type() const;

    LValue self();

    LValue array_access(array_idx_t idx);
    LValue prop(Ref<Prop> prop);
    LValue bound_fset(Ref<FunSet> fset);

    bool is_consteval() const { return _is_consteval; }
    void set_is_consteval(bool is_consteval) {
        _is_consteval = is_consteval;
    } // TMP??

private:
    bool _is_consteval;
};

class Value : public detail::Variant<LValue, RValue> {
public:
    using RValueCb = std::function<void(RValue&)>;
    using RValueConstCb = std::function<void(const RValue&)>;

    explicit Value(LValue lval): Variant{std::move(lval)} {}
    explicit Value(RValue&& rval): Variant{std::move(rval)} {}
    Value(): Value{RValue{}} {}

    Value(Value&&) = default;
    Value& operator=(Value&&) = default;

    bool empty() const;

    bool is_lvalue() const { return is<LValue>(); }
    bool is_rvalue() const { return is<RValue>(); }

    bool is_tmp() const {
        return is_rvalue() || (is_lvalue() && lvalue().is_xvalue());
    }

    LValue& lvalue() { return get<LValue>(); }
    const LValue& lvalue() const { return get<LValue>(); }

    RValue& rvalue() { return get<RValue>(); }
    const RValue& rvalue() const { return get<RValue>(); }

    DataView data_view();
    const DataView data_view() const;

    Ref<Class> dyn_cls() const;
    Ref<Type> dyn_obj_type() const;

    LValue self();

    Value array_access(array_idx_t index);
    Value prop(Ref<Prop> prop);
    Value bound_fset(Ref<FunSet> fset);

    RValue copy_rvalue() const;
    RValue move_rvalue();

    bool is_consteval() const;

    void with_rvalue(std::function<void(const RValue&)> cb) const;
};

using ValueList = std::list<Value>;

} // namespace ulam
