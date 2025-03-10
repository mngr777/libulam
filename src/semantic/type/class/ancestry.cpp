#include "libulam/semantic/type.hpp"
#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>

namespace ulam::cls {

// Ancestor

bool Ancestor::has_data_off() const { return _data_off != NoBitsize; }

bitsize_t Ancestor::data_off() const {
    assert(has_data_off());
    return _data_off;
}

bool Ancestor::has_size_added() const { return _size_added != NoBitsize; }

bitsize_t Ancestor::size_added() const {
    assert(has_size_added());
    return _size_added;
}

void Ancestor::set_data_off(bitsize_t data_off) {
    assert(!has_data_off());
    _data_off = data_off;
}

void Ancestor::set_size_added(bitsize_t size_added) {
    assert(!has_size_added());
    _size_added = size_added;
}

void Ancestor::add_dep_added(Ref<Ancestor> anc) {
    // assert(anc->cls()->is_base_of(cls()));
    _deps_added.push_back(anc);
}

// Ancestry

bool Ancestry::add(Ref<Class> cls, Ref<ast::TypeName> node) {
    return do_add(cls, node).second;
}

std::pair<Ref<Ancestor>, bool>
Ancestry::do_add(Ref<Class> cls, Ref<ast::TypeName> node) {
    // already added?
    {
        auto it = _map.find(cls->id());
        if (it != _map.end())
            return {it->second, false};
    }

    // add immediate base
    auto anc = make<Ancestor>(cls, node);
    auto ref = ulam::ref(anc);
    _parents.push_back(ref);
    _map[cls->id()] = ref;
    _ancestors.push_back(std::move(anc));

    // add grandparents
    for (auto& parent : cls->_ancestry.ancestors()) {
        auto [grandpa, added] = do_add(parent->cls(), parent->node());
        if (added)
            ref->add_dep_added(grandpa);
    }
    return {ref, true};
}

void Ancestry::init() {
    bitsize_t off = 0;
    std::set<type_id_t> added_ids{};
    for (auto& anc : _ancestors) {
        auto cls = anc->cls();
        anc->set_data_off(off);
        auto direct = cls->direct_bitsize();
        off += direct;

        bitsize_t added = 0;
        if (added_ids.insert(cls->id()).second) {
            added += direct;
            for (auto dep : anc->_deps_added) {
                assert(added_ids.count(dep->cls()->id()) == 0);
                added_ids.insert(dep->cls()->id());
                added += dep->cls()->direct_bitsize();
            }
        }
        anc->set_size_added(added);
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
