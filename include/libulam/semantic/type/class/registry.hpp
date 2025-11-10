#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value/types.hpp>
#include <vector>

namespace ulam {

class Class;

class ClassRegistry {
public:
    cls_id_t add(Ref<Class> cls);

    Ref<Class> get(cls_id_t id) const;

private:
    std::vector<Ref<Class>> _classes;
};

} // namespace ulam
