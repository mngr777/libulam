#include <libulam/semantic/type/class/registry.hpp>

namespace ulam {

cls_id_t ClassRegistry::add(Ref<Class> cls) {
    ulam_assert(std::find(_classes.begin(), _classes.end(), cls) == _classes.end());
    _classes.push_back(cls);
    return _classes.size();
}

Ref<Class> ClassRegistry::get(cls_id_t id) const {
    ulam_assert(id != NoClassId);
    ulam_assert(0 < id && id <= _classes.size());
    return _classes[id - 1];
}

} // namespace ulam
