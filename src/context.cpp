#include "src/context.hpp"

namespace ulam {

const std::string_view Context::str(const StrId id) const {
    return _str_storage.get(id);
}

StrId Context::put_str(const std::string_view str) {
    return _str_storage.put(str);
}

}
