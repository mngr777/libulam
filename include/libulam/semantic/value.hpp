#pragma once
#include <functional>
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/array.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/value/types.hpp>
#include <list>

namespace ulam {

using scope_lvl_t = std::uint16_t;
constexpr scope_lvl_t NoScopeLvl = -1;
constexpr scope_lvl_t AutoScopeLvl = -2;

class FunSet;
class Prop;
class Type;
class TypedValue;
class Var;

class RValue;
class Value;

class LValue : public detail::NullableVariant<
                   Ref<Var>,
                   ArrayAccess,
                   ObjectView,
                   BoundFunSet,
                   BoundProp> {
public:
    using Variant::Variant;

    RValue rvalue() const;

    Ref<Type> type();
    Ref<Class> obj_cls();
    ObjectView obj_view();

    LValue array_access(Ref<Type> item_type, array_idx_t index);
    LValue bound_prop(Ref<Prop> prop);
    LValue bound_fset(Ref<FunSet> fset);

    LValue bound_self();
    LValue self();

    Value assign(RValue&& rval);

    bool is_consteval() const { return false; } // TODO

    bool is_xvalue() const { return _is_xvalue; }
    void set_is_xvalue(bool is_xvalue) { _is_xvalue = is_xvalue; }

    bool has_scope_lvl() const { return _scope_lvl != NoScopeLvl; }
    bool has_auto_scope_lvl() const { return _scope_lvl == AutoScopeLvl; }
    scope_lvl_t scope_lvl() const { return _scope_lvl; }
    void set_scope_lvl(scope_lvl_t scope_lvl) { _scope_lvl = scope_lvl; }

private:
    template <typename T>
    LValue derived(T&& value) {
        LValue lval{std::forward<T>(value)};
        lval._is_xvalue = _is_xvalue;
        lval._scope_lvl = _scope_lvl;
        return lval;
    }

    bool _is_xvalue{true};
    scope_lvl_t _scope_lvl{NoScopeLvl};
};

class RValue : public detail::NullableVariant<
                   Integer,
                   Unsigned /* Unary, Bool*/,
                   Bits,
                   String,
                   Array,
                   SPtr<Object>> {
public:
    template <typename T>
    explicit RValue(T&& value, bool is_consteval = false):
        Variant{std::forward<T>(value)}, _is_consteval{is_consteval} {}

    RValue(): Variant{}, _is_consteval{false} {}

    RValue(RValue&&) = default;
    RValue& operator=(RValue&&) = default;

    RValue copy() const;

    Ref<Class> obj_cls();
    ObjectView obj_view();

    RValue array_access(Ref<Type> item_type, array_idx_t index);
    LValue bound_prop(Ref<Prop> prop);
    LValue bound_fset(Ref<FunSet> fset);

    LValue self();

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

    using Variant::Variant;

    Value(Value&&) = default;
    Value& operator=(Value&&) = default;

    bool empty() const;

    bool is_lvalue() const { return is<LValue>(); }
    bool is_rvalue() const { return is<RValue>(); }

    LValue& lvalue() { return get<LValue>(); }
    const LValue& lvalue() const { return get<LValue>(); }

    RValue& rvalue() { return get<RValue>(); }
    const RValue& rvalue() const { return get<RValue>(); }

    Ref<Class> obj_cls();
    ObjectView obj_view();

    Value array_access(Ref<Type> item_type, array_idx_t index);
    Value bound_prop(Ref<Prop> prop);
    Value bound_fset(Ref<FunSet> fset);

    LValue self();

    RValue copy_rvalue() const;
    RValue move_rvalue();

    bool is_consteval() const;

    void with_rvalue(std::function<void(const RValue&)> cb) const;
};

using ValueList = std::list<Value>;

} // namespace ulam
