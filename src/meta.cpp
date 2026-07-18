#include <libulam/meta.hpp>

namespace ulam {

bool Meta::empty() const { return !has_desc() && !has_items(); }

bool Meta::has(const std::string& name) const { return get(name); }

const Meta::Value* Meta::get(const std::string& name) const {
    auto it = _map.find(name);
    return (it != _map.end()) ? do_get(it->second) : nullptr;
}

const Meta::Value* Meta::do_get(idx_t idx) const {
    ulam_assert(idx < _items.size());
    return &_items[idx].second;
}

Meta::idx_t Meta::do_add(std::string&& name, Value&& value) {
    idx_t idx = _items.size();
    _items.emplace_back(name, std::move(value));
    return idx;
}

} // namespace ulam

std::ostream& operator<<(std::ostream& os, const ulam::Meta& meta) {
    if (meta.has_desc())
        os << '"' << meta.desc() << "\"\n";
    for (const auto& item : meta)
        os << "  \\{" << item.first << "} " << item.second << "\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ulam::Meta::Value& value) {
    value.accept(
        [&](const std::string& str) { os << '"' << str << '"'; },
        [&](const ulam::Meta::List<std::string>& list) {
            for (const auto& str : list)
                os << "\"" <<str << "\" ";
        },
        [&](auto&&) { ulam_assert(false); });
    return os;
}
