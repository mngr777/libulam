#include <cassert>
#include <libulam/str_pool.hpp>

namespace ulam {

// StrPoolBase

bool StrPoolBase::has_id(str_id_t id) const {
    assert(id != NoStrId);
    return id < _index.size();
}

const std::string_view StrPoolBase::get(str_id_t id) const {
    assert(id != NoStrId);
    assert(id < _index.size());
    return _index[id];
}

StrPoolBase::PairT StrPoolBase::store(const std::string_view str, bool copy) {
    auto copied = copy ? _notepad.write(str) : str;
    str_id_t id = _index.size();
    _index.push_back(copied);
    return {id, copied};
}

// UniqStrPool

UniqStrPool::UniqStrPool(UniqStrPool* parent): StrPoolBase{}, _parent{parent} {
    if (_parent) {
        _offset = _parent->_offset + _parent->_index.size();
        _parent->_is_locked = true; // lock parent
    }
}

bool UniqStrPool::has(const std::string_view str) const {
    return _map.count(str) == 1 || (_parent && _parent->has(str));
}

str_id_t UniqStrPool::id(const std::string_view str) const {
    if (_parent) {
        auto str_id = _parent->id(str);
        if (str_id != NoStrId)
            return str_id;
    }
    auto it = _map.find(str);
    return (it != _map.end()) ? it->second + _offset : NoStrId;
}

const std::string_view UniqStrPool::get(str_id_t id) const {
    if (id < _offset) {
        assert(_parent);
        return _parent->get(id);
    }
    return StrPoolBase::get(id - _offset);
}

str_id_t UniqStrPool::put(const std::string_view str, bool copy) {
    assert(!_is_locked);
    if (_parent) {
        auto str_id = _parent->id(str);
        if (str_id != NoStrId)
            return str_id;
    }
    auto it = _map.find(str);
    if (it != _map.end())
        return it->second;
    auto stored = store(str, copy);
    _map[stored.second] = stored.first;
    return stored.first + _offset;
}

// StrPool

str_id_t StrPool::put(const std::string_view str, bool copy) {
    return store(str, copy).first;
}

} // namespace ulam
