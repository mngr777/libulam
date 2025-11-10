#include <libulam/semantic/type/class/registry.hpp>

namespace ulam {

cls_id_t ClassRegistry::add(Ref<Class> cls) {
    assert(std::find(_classes.begin(), _classes.end(), cls) == _classes.end());
    _classes.push_back(cls);
    return _classes.size();
}

Ref<Class> ClassRegistry::get(cls_id_t id) const {
    assert(id != NoClassId);
    assert(0 < id && id <= _classes.size());
    return _classes[id - 1];
}

} // namespace ulam
