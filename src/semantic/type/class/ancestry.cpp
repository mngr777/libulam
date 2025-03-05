#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>

namespace ulam::cls {

bool Ancestor::has_data_off() const { return _data_off != NoBitsize; }

bitsize_t Ancestor::data_off() const {
    assert(has_data_off());
    return _data_off;
}

void Ancestor::set_data_off(bitsize_t data_off) {
    assert(!has_data_off());
    _data_off = data_off;
}

bool Ancestry::add(Ref<Class> cls, Ref<ast::TypeName> node) {
    // already has this base?
    if (is_base(cls))
        return false;

    // add immediate base
    auto anc = make<Ancestor>(cls, node);
    // _cur_data_off += cls->direct_bitsize();
    _parents.push_back(ref(anc));
    _map[cls->id()] = ref(anc);
    _ancestors.push_back(std::move(anc));

    // add grandparents
    for (auto& parent_anc : cls->_ancestry.ancestors())
        add(parent_anc->cls(), parent_anc->node());

    return true;
}

void Ancestry::init() {
    bitsize_t off = 0;
    for (auto& anc : _ancestors) {
        auto cls = anc->cls();
        assert(cls->is_ready());
        anc->set_data_off(off);
        off += cls->direct_bitsize();
    }
}

bool Ancestry::is_base(Ref<const Class> cls) const {
    assert(cls->id() != NoTypeId);
    return _map.count(cls->id()) == 1;
}

bitsize_t Ancestry::data_off(Ref<const Class> cls) const {
    assert(is_base(cls));
    return _map.at(cls->id())->data_off();
}

} // namespace ulam::cls
