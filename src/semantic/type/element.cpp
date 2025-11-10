#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/element.hpp>

namespace ulam {

elt_id_t ElementRegistry::add(Ref<Class> cls) {
    assert(cls->is_element());
    assert(
        std::find(_elements.begin(), _elements.end(), cls) == _elements.end());
    _elements.push_back(cls);
    return _elements.size();
}

Ref<Class> ElementRegistry::get(elt_id_t id) const {
    assert(id != NoEltId);
    assert(0 < id && id <= _elements.size());
    return _elements[id - 1];
}

} // namespace ulam
