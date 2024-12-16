#pragma once
#include <libulam/str_pool.hpp>

namespace ulam::ast {

class Context {
public:
    Context(): _str_pool{} {}

    str_id_t str_id(const std::string_view str) { return _str_pool.put(str); }

    const std::string_view str(str_id_t str_id) const {
        return _str_pool.get(str_id);
    }

    UniqStrPool& str_pool() { return _str_pool; }
    const UniqStrPool& str_pool() const { return _str_pool; }

private:
    UniqStrPool _str_pool;
};

} // namespace ulam::ast
