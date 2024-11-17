#pragma once
#include <libulam/memory/ptr.hpp>

namespace ulam {

class Type;

class Object {
public:
    Object(Ref<Type> type): _type{type} {}

    Ref<Type> type() { return _type; }
    Ref<const Type> type() const { return _type; }

    // TODO: data

private:
    Ref<Type> _type;
};

} // namespace ulam
