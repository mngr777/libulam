#pragma once
#include <cassert>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/deps/symbol_id.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/var.hpp>
#include <unordered_map>
#include <vector>

namespace ulam::sema {

class DepTrack {
public:
    void add(SymbolId sym_id, SymbolIdSet deps) {
        assert(deps.size() > 0);
        auto id = _items.size();
        _items.emplace_back(Item{sym_id, deps});
        for (auto dep : deps)
            add_dep(dep, id);
    }

    SymbolIdSet resolved(SymbolId sym_id) {
        SymbolIdSet set;
        // anything dependent on this symbol?
        auto it = _map.find(sym_id);
        if (it == _map.end())
            return set;
        // remove from unresolved dep lists
        for (auto idx : it->second) {
            assert(idx < _items.size());
            auto& item = _items[idx];
            item.unresolved.erase(sym_id);
            // all deps resolved?
            if (item.unresolved.size() == 0)
                set.insert(item.sym_id);
            // NOTE: item is not removed
        }
        return set;
    }

private:
    struct Item {
        SymbolId sym_id;
        SymbolIdSet unresolved;
    };
    using ItemList = std::vector<Item>;
    using ItemIdxSet = std::unordered_set<std::size_t>;

    void add_dep(SymbolId sym_id, unsigned idx) {
        auto [it, _] = _map.emplace(sym_id, ItemIdxSet{});
        it->second.insert(idx);
    }

    ItemList _items;
    std::unordered_map<SymbolId, ItemIdxSet> _map;
};

} // namespace ulam::sema
