#pragma once
#include <libulam/semantic/type.hpp>

namespace ulam {

class Var {
public:
    Var(Ref<Type> type): _type{type} {}

    Ref<Type> type() { return _type; }

private:
    Ref<Type> _type;
};

} // namespace ulam
