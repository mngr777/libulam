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

    const std::uint16_t size() const { return _classes.size(); }

    auto begin() const { return _classes.begin(); }
    auto end() const { return _classes.end(); }

private:
    std::vector<Ref<Class>> _classes;
};

} // namespace ulam
