#include <cassert>
#include <libulam/semantic/type/class_kind.hpp>

namespace ulam {

const std::string_view class_kind_str(ClassKind kind) {
    switch (kind) {
    case ClassKind::Element:
        return "element";
    case ClassKind::Quark:
        return "quark";
    case ClassKind::Transient:
        return "transient";
    default:
        assert(false);
    }
}

} // namespace ulam
