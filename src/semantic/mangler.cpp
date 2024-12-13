#include "src/semantic/detail/leximited.hpp"
#include <cassert>
#include <libulam/semantic/mangler.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class.hpp>
#include <sstream>

namespace ulam {

// see ULAM SymbolClassNameTemplate::formatAnInstancesArgValuesAsAString
std::string Mangler::mangled(const TypedValueList& values) {
    std::stringstream ss;
    for (const auto& tv : values)
        write_mangled(ss, tv);
    return {ss.str(), true};
}

void Mangler::write_mangled(std::ostream& os, const TypedValue& tv) {
    write_mangled(os, tv.type());
    write_mangled(os, tv.value());
}

// see ULAM {UlamType,UlamTypeClass}::getUlamTypeMangledType()
void Mangler::write_mangled(std::ostream& os, Ref<const Type> type) {
    assert(type);
    assert(type->canon());

    type = type->canon();

    if (type->is_ref()) {
        os << "r";
        type = type->as_ref()->refd()->canon();
        assert(type);
    }

    while (type->is_array()) {
        auto array = type->canon()->as_array();
        assert(array);
        auto size = array->array_size();
        if (size == 0 || size == UnknownArraySize) { // TODO: is this correct?
            detail::write_leximited(os, (Integer)-1);
        } else {
            detail::write_leximited(os, (Unsigned)size);
        }
        type = array->item_type()->canon();
    }

    if (type->bitsize() == 0) {
        os << "10";
    } else {
        detail::write_leximited(os, (Unsigned)type->bitsize());
    }

    if (type->is_class()) {
        auto cls = type->as_class();
        detail::write_leximited(os, _str_pool.get(cls->name_id()));
        // NOTE: param types and values are not included
    } else {
        assert(type->is_prim());
        detail::write_leximited(os, builtin_type_code(type->builtin_type_id()));
    }
}

void Mangler::write_mangled(std::ostream& os, const Value& type) {
    // TODO: implement
    assert(false);
}

} // namespace ulam
