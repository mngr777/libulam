#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/types.hpp>
#include <vector>

namespace ulam {

class Class;

class ElementRegistry {
public:
    elt_id_t add(Ref<Class> cls);

    Ref<Class> get(elt_id_t id);

private:
    std::vector<Ref<Class>> _elements;
};

} // namespace ulam
