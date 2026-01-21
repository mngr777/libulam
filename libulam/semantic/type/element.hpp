#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/class/options.hpp>
#include <libulam/semantic/value/types.hpp>
#include <vector>

namespace ulam {

class Class;

class ElementRegistry {
public:
    ElementRegistry(const ClassOptions& class_options);

    elt_id_t add(Ref<Class> cls);

    Ref<Class> get(elt_id_t id) const;

private:
    const ClassOptions& _class_options;
    std::vector<Ref<Class>> _elements;
};

} // namespace ulam
