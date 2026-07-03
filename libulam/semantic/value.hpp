#pragma once
#include <functional>
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/bound_fun_set.hpp>
#include <libulam/semantic/value/data.hpp>
#include <libulam/semantic/value/flags.hpp>
#include <libulam/semantic/value/types.hpp>
#include <list>
#include <variant>

namespace ulam {

// TODO: constness is an attribute of lvalue !!

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
    using Base = detail::NullableVariant<Ref<Var>, DataView, BoundFunSet>;

public:
    LValue();

private:
    LValue(std::monostate);
    LValue(Ref<Var> var);
    LValue(DataView data);
    LValue(BoundFunSet bfset);

public:
    static constexpr value::flags_t DefaultFlags = value::IsXvalue;

    template <typename T>
    static LValue make(T&& value, value::flags_t flags = DefaultFlags) {
        LValue lval{std::forward<T>(value)};
        lval._flags |= flags; // ??
        return lval;
    }

    static LValue make_ph(value::flags_t flags = DefaultFlags) {
        return make(std::monostate{}, flags);
    }

    template <typename T> LValue derived(T&& value) const {
        LValue lval{std::forward<T>(value)};
        lval._flags = _flags;
        lval._scope_lvl = _scope_lvl;
        return lval;
    }

    LValue derived() const { return derived(std::monostate{}); }

    bool has_rvalue() const;
    RValue rvalue(bool real = false) const;
    void
    with_rvalue(std::function<void(const RValue&)> cb, bool real = false) const;

    Ref<Type> type() const;

    DataView data_view();
    const DataView data_view() const;

    Ref<Class> dyn_cls(bool real = false) const;
    Ref<Type> dyn_obj_type(bool real = false) const;

    LValue self();
    LValue as(Ref<Type> type);
    LValue atom_of();

    LValue array_access(array_idx_t idx, bool is_consteval_idx = true);
    const LValue
    array_access(array_idx_t idx, bool is_consteval_idx = true) const;

    LValue prop(Ref<Prop> prop);
    const LValue prop(Ref<Prop> prop) const;

    LValue bound_fset(Ref<FunSet> fset, Ref<Class> base = {});

    Value assign(RValue&& rval);

    bool is_consteval() const;
    void set_is_consteval(bool is_consteval);

    bool is_xvalue() const;
    void set_is_xvalue(bool is_xvalue);

    bool has_scope_lvl() const { return _scope_lvl != NoScopeLvl; }
    bool has_auto_scope_lvl() const { return _scope_lvl == AutoScopeLvl; }
    scope_lvl_t scope_lvl() const { return _scope_lvl; }
    void set_scope_lvl(scope_lvl_t scope_lvl) { _scope_lvl = scope_lvl; }

private:
    value::flags_t _flags{value::NoFlags};
    scope_lvl_t _scope_lvl{NoScopeLvl};
};

class RValue
    : public detail::NullableVariant<Integer, Unsigned, Bits, String, DataPtr> {
    using Base =
        detail::NullableVariant<Integer, Unsigned, Bits, String, DataPtr>;

public:
    RValue(): Base{} {}

private:
    template <typename T>
    explicit RValue(T&& value, value::flags_t flags):
        Base{std::forward<T>(value)}, _flags{flags} {
        ulam_assert((_flags | value::RValueFlags) == value::RValueFlags);
    }

public:
    // TODO: DefaultFlags

    template <typename T>
    static RValue make(T&& value, value::flags_t flags = value::NoFlags) {
        return RValue{std::forward<T>(value), flags};
    }

    static RValue make_ph(value::flags_t flags = value::NoFlags) {
        return make(std::monostate{}, flags);
    }

    RValue(RValue&&) = default;
    RValue& operator=(RValue&&) = default;

    bool has_rvalue() const;

    RValue copy() const;

    DataView data_view();
    const DataView data_view() const;

    Ref<Class> dyn_cls(bool self = false) const;
    Ref<Type> dyn_obj_type(bool self = false) const;

    LValue self();
    LValue as(Ref<Type> type);
    LValue atom_of();

    LValue array_access(array_idx_t idx, bool is_consteval_idx);
    const LValue array_access(array_idx_t idx, bool is_consteval_idx) const;

    LValue prop(Ref<Prop> prop);
    const LValue prop(Ref<Prop> prop) const;

    LValue bound_fset(Ref<FunSet> fset, Ref<Class> base = {});

    bool is_consteval() const;
    void set_is_consteval(bool is_consteval);

private:
    value::flags_t _flags{value::NoFlags};
};

class Value : public detail::Variant<LValue, RValue> {
public:
    explicit Value(LValue lval): Variant{std::move(lval)} {}
    explicit Value(RValue&& rval): Variant{std::move(rval)} {}
    Value(): Value{RValue{}} {}

    // ?? remove and use Value{type->construct()}
    template <typename T>
    static Value make_l(T&& value, value::flags_t flags = value::NoFlags) {
        return Value{LValue::make(std::forward<T>(value), flags)};
    }

    template <typename T>
    static Value make_r(T&& value, value::flags_t flags = value::NoFlags) {
        return Value{RValue::make(std::forward<T>(value), flags)};
    }

    static Value make_l_ph(value::flags_t flags = value::NoFlags) {
        return Value{LValue::make_ph(flags)};
    }

    static Value make_r_ph(value::flags_t flags = value::NoFlags) {
        return Value{RValue::make_ph(flags)};
    }

    Value(Value&&) = default;
    Value& operator=(Value&&) = default;

    bool empty() const;
    bool has_rvalue() const;

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

    Ref<Class> dyn_cls(bool real = false) const;
    Ref<Type> dyn_obj_type(bool real = false) const;

    LValue self();
    LValue as(Ref<Type> type);
    LValue atom_of();

    bitsize_t position_of();

    Value array_access(array_idx_t idx, bool is_consteval_idx = true);
    const Value
    array_access(array_idx_t idx, bool is_consteval_idx = true) const;

    Value prop(Ref<Prop> prop);
    const Value prop(Ref<Prop> prop) const;

    Value bound_fset(Ref<FunSet> fset, Ref<Class> base = {});

    Value copy() const;

    RValue copy_rvalue(bool real = false) const;
    RValue move_rvalue(bool real = false);

    Value deref(bool real = false);

    bool is_consteval() const;
    void set_is_consteval(bool is_consteval);

    void
    with_rvalue(std::function<void(const RValue&)> cb, bool real = false) const;
};

using ValueList = std::list<Value>;

} // namespace ulam
