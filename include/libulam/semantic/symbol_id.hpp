#pragma once
#include <libulam/semantic/module.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_set>

namespace ulam::sema {
class SymbolId;
}

template <> struct std::hash<ulam::sema::SymbolId>;

namespace ulam::sema {

class SymbolId {
public:
    SymbolId(module_id_t module_id, str_id_t class_name_id, str_id_t name_id):
        _module_id{module_id},
        _class_name_id{class_name_id},
        _name_id{name_id} {}

    bool operator==(SymbolId other) const {
        return _module_id == other._module_id &&
               _class_name_id == other._class_name_id &&
               _name_id == other._name_id;
    }

    bool operator!=(SymbolId other) const {
        return !(*this == other);
    }

    module_id_t module_id() const { return _module_id; }
    str_id_t class_name_id() const { return _class_name_id; }
    str_id_t name_id() const { return _name_id; }

private:
    module_id_t _module_id;
    str_id_t _class_name_id;
    str_id_t _name_id;
};

using SymbolIdSet = std::unordered_set<SymbolId>;

} // namespace ulam::sema

template <> struct std::hash<ulam::sema::SymbolId> {
    std::size_t operator()(const ulam::sema::SymbolId& sym_id) const {
        return (std::size_t)sym_id.module_id() << 2 ^
               (std::size_t)sym_id.name_id() << 1 ^
               (std::size_t)sym_id.class_name_id();
    }
};
