#pragma once
#include <libulam/detail/variant.hpp>
#include <libulam/meta/id.hpp>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ulam {

class Meta {
public:
    using Value = detail::Variant<std::string>;
    using Item = std::pair<std::string, Value>;

public:
    Meta() {}

    Meta(Meta&&) = default;
    Meta& operator=(Meta&&) = default;

    bool empty() const;

    bool has(const std::string& name) const;

    const Value* get(const std::string& name) const;

    template <typename T>
    bool add(std::string&& name, T&& value, bool replace = false) {
        return add(name, std::forward<T>(value), replace);
    }

    bool add(std::string&& name, Value&& value, bool replace = false);

    bool has_desc() const { return !_desc.empty(); }
    const std::string& desc() const { return _desc; }
    void set_desc(std::string&& desc) { _desc = std::move(desc); }

    bool has_items() const { return !_items.empty(); }

    const auto begin() const { return _items.begin(); }
    const auto end() const { return _items.end(); }

private:
    using index_t = std::uint32_t;

    const Value* do_get(index_t idx) const;
    index_t do_add(std::string_view name, Value&& value);

    std::string _desc;
    std::vector<Item> _items;
    std::unordered_map<std::string, index_t> _map;
};

} // namespace ulam

std::ostream& operator<<(std::ostream& os, const ulam::Meta& meta);
std::ostream& operator<<(std::ostream& os, const ulam::Meta::Value& value);
