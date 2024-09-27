#pragma once
#include "libulam/types.hpp"
#include "str_storage.hpp"
#include <memory_resource>

namespace ulam {

class Context {
public:
    explicit Context(std::pmr::memory_resource* res):
        _str_pool(res), _name_strs(&_str_pool), _value_strs(&_str_pool) {}

    const std::string_view name_str(const StrId id) const;
    StrId store_name_str(const std::string_view str);

    const std::string_view value_str(const StrId id) const;
    StrId store_value_str(const std::string_view str);

private:
    std::pmr::unsynchronized_pool_resource _str_pool;
    UniqueStrStorage _name_strs;
    StrStorage _value_strs;
};

} // namespace ulam
