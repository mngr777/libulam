#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>

namespace ulam::cls {

bool Ancestry::add(Ref<Class> cls, Ref<ast::TypeName> node) {
    // already has this base?
    if (is_base(cls))
        return false;

    // add immediate base
    auto anc = make<Ancestor>(cls, node, _cur_data_off);
    _cur_data_off += cls->direct_bitsize();
    _parents.push_back(ref(anc));
    _map[cls->id()] = ref(anc);
    _ancestors.push_back(std::move(anc));

    // add grandparents
    for (auto& parent_anc : cls->_ancestry.ancestors())
        add(parent_anc->cls(), parent_anc->node());

    return true;
}

bool Ancestry::is_base(Ref<Class> cls) const {
    assert(cls->id() != NoTypeId);
    return _map.count(cls->id()) == 1;
}

} // namespace ulam::cls
