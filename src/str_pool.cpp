#include "libulam/str_pool.hpp"
#include <cassert>

namespace ulam {

const std::string_view StrPoolBase::get(str_id_t id) const {
    assert(id < _index.size());
    return _index[id];
}

StrPoolBase::PairT StrPoolBase::store(const std::string_view str, bool copy) {
    auto copied = copy ? _notepad.write(str) : str;
    str_id_t id = _index.size();
    _index.push_back(copied);
    return {id, copied};
}

str_id_t UniqStrPool::put(const std::string_view str, bool copy) {
    auto it = _map.find(str);
    if (it != _map.end())
        return it->second;
    auto stored = store(str, copy);
    _map[stored.second] = stored.first;
    return stored.first;
}

str_id_t StrPool::put(const std::string_view str, bool copy) {
    return store(str, copy).first;
}

} // namespace ulam
