#pragma once
#include <memory_resource>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include "libulam/types.hpp"
#include "memory/notepad.hpp"

namespace ulam {

class StrStorageBase {
public:
    StrStorageBase(std::pmr::memory_resource* res):
        _notepad(res), _index(res) {}
    virtual ~StrStorageBase() {}

    StrStorageBase(const StrStorageBase&) = delete;
    StrStorageBase& operator=(const StrStorageBase&) = delete;

    const StrId put(const std::string_view str, bool copy = true);
    const std::string_view get(const StrId id) const;

protected:
    using Pair = std::pair<const StrId, const std::string_view>;

    virtual Pair do_put(const std::string_view str, bool copy) = 0;
    Pair store(const std::string_view str, bool copy);

private:
    mem::Notepad _notepad;
    std::pmr::vector<std::string_view> _index;
    mutable std::shared_mutex _mtx;
};


class StrStorage : public StrStorageBase {
public:
    StrStorage(std::pmr::memory_resource* res):
        StrStorageBase(res) {}

protected:
    Pair do_put(const std::string_view str, bool copy) override;
};


class UniqueStrStorage : public StrStorageBase {
public:
    UniqueStrStorage(std::pmr::memory_resource* res):
        StrStorageBase(res), _map(res) {}

protected:
    Pair do_put(const std::string_view str, bool copy) override;

private:
    std::pmr::unordered_map<std::string_view, StrId> _map;
};

} // namespace ulam

