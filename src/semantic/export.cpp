#include <libulam/semantic/export.hpp>

namespace ulam {

bool ExportTable::has(str_id_t name_id) const {
    return _table.count(name_id) > 0;
}

const Export* ExportTable::get(str_id_t name_id) const {
    auto it = _table.find(name_id);
    return (it != _table.end()) ? &it->second : nullptr;
}

const Export* ExportTable::add(str_id_t name_id, Export exp) {
    auto [it, added] = _table.emplace(name_id, std::move(exp));
    return added ? nullptr : &it->second;
}

} // namespace ulam
