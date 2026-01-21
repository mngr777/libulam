#pragma once
#include <libulam/str_pool.hpp>

namespace ulam::ast {

class Context {
public:
    Context() { _text_pool.put(""); }

    str_id_t str_id(const std::string_view str) { return _str_pool.put(str); }

    const std::string_view str(str_id_t str_id) const {
        return _str_pool.get(str_id);
    }

    str_id_t text_id(const std::string_view str) { return _text_pool.put(str); }

    const std::string_view text(str_id_t str_id) const {
        return _text_pool.get(str_id);
    }

    UniqStrPool& str_pool() { return _str_pool; }
    const UniqStrPool& str_pool() const { return _str_pool; }

    UniqStrPool& text_pool() { return _text_pool; }
    const UniqStrPool& text_pool() const { return _text_pool; }

private:
    UniqStrPool _str_pool;
    UniqStrPool _text_pool;
};

} // namespace ulam::ast
