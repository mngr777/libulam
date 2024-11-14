#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

class Fun {
public:
    Fun(Ref<Type> ret_type) {}

    Ref<Type> ret_type() { return _ret_type; }

    void add_param();

private:
    Ref<Type> _ret_type;
};

}
