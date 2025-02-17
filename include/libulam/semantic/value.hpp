#pragma once
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/array.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/bound.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/value/types.hpp>
#include <list>

namespace ulam {

class FunSet;
class Prop;
class Type;
class TypedValue;
class Var;

class RValue;
class Value;

class LValue
    : public detail::
          Variant<Ref<Var>, ArrayAccess, ObjectView, BoundFunSet, BoundProp> {
public:
    using Variant::Variant;

    RValue rvalue() const;

    Ref<Type> type();
    Ref<Class> obj_cls();

    LValue array_access(Ref<Type> item_type, array_idx_t index);
    LValue bound_prop(Ref<Prop> prop);
    LValue bound_fset(Ref<FunSet> fset);

    Value assign(RValue&& rval);
};

class RValue : public detail::Variant<
                   Integer,
                   Unsigned /* Unary, Bool*/,
                   Bits,
                   String,
                   Ref<FunSet>, /* ?? */
                   Array,
                   SPtr<Object>> {
public:
    using Variant::Variant;

    RValue(RValue&&) = default;
    RValue& operator=(RValue&&) = default;

    RValue copy() const;

    Ref<Class> obj_cls();

    RValue array_access(Ref<Type> item_type, array_idx_t index);
    LValue bound_prop(Ref<Prop> prop);
    LValue bound_fset(Ref<FunSet> fset);
};

class Value : public detail::Variant<LValue, RValue> {
public:
    using RValueCb = std::function<void(RValue&)>;
    using RValueConstCb = std::function<void(const RValue&)>;

    using Variant::Variant;

    Value(Value&&) = default;
    Value& operator=(Value&&) = default;

    bool is_lvalue() const { return is<LValue>(); }
    bool is_rvalue() const { return is<RValue>(); }

    LValue& lvalue() { return get<LValue>(); }
    const LValue& lvalue() const { return get<LValue>(); }

    RValue& rvalue() { return get<RValue>(); }
    const RValue& rvalue() const { return get<RValue>(); }

    Ref<Class> obj_cls();

    Value array_access(Ref<Type> item_type, array_idx_t index);
    Value bound_prop(Ref<Prop> prop);
    Value bound_fset(Ref<FunSet> fset);

    RValue copy_rvalue() const;
    RValue move_rvalue();
};

using ValueList = std::list<Value>;

} // namespace ulam
