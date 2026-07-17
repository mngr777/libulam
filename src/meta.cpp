#include <libulam/assert.hpp>
#include <libulam/meta.hpp>

namespace ulam {

bool Meta::empty() const { return !has_desc() && !has_items(); }

bool Meta::has(const std::string& name) const { return get(name); }

const Meta::Value* Meta::get(const std::string& name) const {
    auto it = _map.find(name);
    return (it != _map.end()) ? do_get(it->second) : nullptr;
}

bool Meta::add(std::string&& name, Value&& value, bool replace) {
    auto it = _map.find(name);
    if (it != _map.end()) {
        if (replace) {
            auto idx = it->second;
            _items[idx].second = std::move(value);
        }
        return false;
    }
    auto idx = do_add(name, std::move(value));
    _map[std::move(name)] = idx;
    return true;
}

const Meta::Value* Meta::do_get(index_t idx) const {
    ulam_assert(idx < _items.size());
    return &_items[idx].second;
}

Meta::index_t Meta::do_add(std::string_view name, Value&& value) {
    index_t idx = _items.size();
    _items.emplace_back(name, std::move(value));
    return idx;
}

} // namespace ulam

std::ostream& operator<<(std::ostream& os, const ulam::Meta& meta) {
    if (meta.has_desc())
        os << '"' << meta.desc() << "\"\n";
    for (const auto& item : meta)
        os << "  \\{" << item.first << "} \"" << item.second << "\"\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ulam::Meta::Value& value) {
    value.accept(
        [&](const std::string& str) { os << str; },
        [&](auto&&) { ulam_assert(false); });
    return os;
}
