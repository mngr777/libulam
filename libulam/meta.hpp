#pragma once
#include <algorithm>
#include <iterator>
#include <libulam/assert.hpp>
#include <libulam/detail/variant.hpp>
#include <libulam/meta/id.hpp>
#include <list>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ulam {

class Meta {
public:
    template <typename T> using List = std::list<T>;

    using Value = detail::Variant<std::string, List<std::string>>;

private:
    using Item = std::pair<std::string, Value>;

    using idx_t = std::uint32_t;
    using Map = std::unordered_map<std::string, idx_t>;

public:
    Meta() {}

    Meta(Meta&&) = default;
    Meta& operator=(Meta&&) = default;

    bool empty() const;

    bool has(const std::string& name) const;

    const Value* get(const std::string& name) const;

    template <typename T>
    bool set(std::string&& name, T&& value, bool replace = false) {
        auto it = _map.find(name);
        if (it != _map.end()) {
            if (replace) {
                auto idx = it->second;
                _items[idx].second = Value{std::forward<T>(value)};
            }
            return false;
        }
        auto idx = do_add(std::string{name}, Value{std::forward<T>(value)});
        _map.insert(it, Map::value_type{std::move(name), idx});
        return true;
    }

    template <typename T> bool append(std::string&& name, List<T>&& value) {
        auto it = _map.find(name);
        if (it != _map.end()) {
            auto idx = it->second;
            _items[idx].second.accept(
                [&](List<T>& old) {
                    std::move(
                        value.begin(), value.end(), std::back_inserter(old));
                },
                [&](auto&&) { ulam_assert(false); });
            return false;
        }
        auto idx =
            do_add(std::string{name}, Value{std::forward<List<T>>(value)});
        _map.insert(it, Map::value_type{std::move(name), idx});
        return true;
    }

    bool has_desc() const { return !_desc.empty(); }
    const std::string& desc() const { return _desc; }
    void set_desc(std::string&& desc) { _desc = std::move(desc); }

    bool has_items() const { return !_items.empty(); }

    const auto begin() const { return _items.begin(); }
    const auto end() const { return _items.end(); }

private:
    const Value* do_get(idx_t idx) const;
    idx_t do_add(std::string&& name, Value&& value);

    std::string _desc;
    std::vector<Item> _items;
    Map _map;
};

} // namespace ulam

std::ostream& operator<<(std::ostream& os, const ulam::Meta& meta);
std::ostream& operator<<(std::ostream& os, const ulam::Meta::Value& value);
