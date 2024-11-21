#pragma once
#include <libulam/semantic/type.hpp>

namespace ulam {

class TypeIdGen {
public:
    type_id_t next() { return _next++; }

private:
    type_id_t _next{1};
};

} // namespace ulam
