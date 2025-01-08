#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>

namespace ulam::cls {

bool Ancestry::add(Ref<Class> cls, Ref<ast::TypeName> node) {
    if (has_base(cls))
        return false;
    auto anc = make<Ancestor>(cls, node);
    _parents.push_back(ref(anc));
    _ancestors.push_back(std::move(anc));
    // TODO: map
    return true;
}

bool Ancestry::has_base(Ref<Class> cls) const {
    assert(cls->id() != NoTypeId);
    return _map.count(cls->id()) == 1;
}

} // namespace ulam::cls
