#pragma once
#include <memory_resource>
#include "libulam/types.hpp"
#include "str_storage.hpp"

namespace ulam {

class Context {
public:
    Context(): _str_pool(), _str_storage(&_str_pool) {}

    const std::string_view str(const StrId id) const;
    StrId put_str(const std::string_view str);

private:
    std::pmr::unsynchronized_pool_resource _str_pool;
    StrStorage _str_storage;
};

}
