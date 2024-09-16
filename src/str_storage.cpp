#include <cassert>
#include <mutex>
#include "src/str_storage.hpp"

namespace ulam {

const StrId StrStorageBase::put(const std::string_view str, bool copy) {
    std::unique_lock lck(_mtx);
    return do_put(str, copy).first;
}

const std::string_view StrStorageBase::get(const StrId id) const {
    std::shared_lock lck(_mtx);
    assert(id < _index.size() && "Invalid string ID");
    return _index[id];
}

StrStorageBase::Pair StrStorageBase::store(const std::string_view str, bool copy) {
    StrId id = _index.size();
    const std::string_view copied = copy ? _notepad.write(str) : str;
    _index.push_back(copied);
    return {id, copied};
}


StrStorage::Pair StrStorage::do_put(const std::string_view str, bool copy) {
    return store(str, copy);
}


UniqueStrStorage::Pair UniqueStrStorage::do_put(const std::string_view str, bool copy) {
    // already stored?
    auto it = _map.find(str);
    if (it != _map.end())
        return {it->second, it->first};
    // add copy
    auto pair = store(str, copy);
    _map[pair.second] = pair.first;
    return pair;
}

} // namespace ulam

