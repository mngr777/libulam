#include "libulam/semantic/value/types.hpp"
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/element.hpp>

namespace ulam {

static_assert(NoEltId == 0);

ElementRegistry::ElementRegistry(const ClassOptions& class_options):
    _class_options{class_options} {
    _elements.push_back({}); // empty
}

elt_id_t ElementRegistry::add(Ref<Class> cls) {
    assert(cls->is_element());
    assert(
        std::find(_elements.begin(), _elements.end(), cls) == _elements.end());

    elt_id_t id = NoEltId;
    if (!_elements[NoEltId] &&
        cls->name() == _class_options.empty_element_name) {
        // Empty
        _elements[NoEltId] = cls;
    } else {
        // non-Empty element
        assert(cls->name() != _class_options.empty_element_name);
        id = _elements.size();
        _elements.push_back(cls);
    }
    return id;
}

Ref<Class> ElementRegistry::get(elt_id_t id) const {
    assert(id < _elements.size());
    auto elt = _elements[id];
    assert(elt || (id == NoEltId && _class_options.empty_element_name.empty()));
    return _elements[id];
}

} // namespace ulam
