#include <cassert>
#include <libulam/semantic/type/class/layout.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam::cls {

// LayoutItem

bitsize_t LayoutItem::bitsize() const {
    return std::visit([](auto&& obj) { return obj->bitsize(); }, _obj);
}

// Layout

bitsize_t Layout::bitsize() const {
    bitsize_t size = 0;
    for (const auto& item : _items)
        size += item.bitsize();
    return size;
}

void Layout::add(Ref<Var> var) { _items.emplace_back(var); }

} // namespace ulam::cls
