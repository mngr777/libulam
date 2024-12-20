#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Param {
public:
    Param(Ref<Type> type, Value&& default_value):
        _type{type}, _default_value{std::move(default_value)} {}
    Param(Ref<Type> type): Param{type, {}} {}

    Ref<Type> type() { return _type; }
    Ref<const Type> type() const { return _type; }

    bool has_default_value() const { return !_default_value.is_nil(); }
    Value& default_value() { return _default_value; }
    const Value& default_value() const { return _default_value; }

private:
    Ref<Type> _type;
    Value _default_value;
};

using ParamList = std::list<Param>;

} // namespace ulam
