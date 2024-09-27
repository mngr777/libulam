#include "src/context.hpp"

namespace ulam {

const std::string_view Context::name_str(const StrId id) const {
    return _name_strs.get(id);
}

StrId Context::store_name_str(const std::string_view str) {
    return _name_strs.put(str);
}

const std::string_view Context::value_str(const StrId id) const {
    return _value_strs.get(id);
}

StrId Context::store_value_str(const std::string_view str) {
    return _value_strs.put(str);
}

} // namespace ulam
