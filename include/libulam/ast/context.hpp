#pragma once
#include <libulam/diag.hpp>
#include <libulam/lang/scope.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {

class Context {
public:
    Context(Diag& diag): _diag{diag}, _global_scope{nullptr}, _str_pool{} {}

    Diag& diag() { return _diag; }

    Scope* global_scope() { return &_global_scope; }

    str_id_t str_id(const std::string_view str) { return _str_pool.put(str); }
    const std::string_view str(str_id_t str_id) const {
        return _str_pool.get(str_id);
    }

private:
    Diag& _diag;
    Scope _global_scope;
    StrPool _str_pool;
};

} // namespace ulam::ast
