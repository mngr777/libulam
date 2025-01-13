#include "libulam/semantic/value/types.hpp"
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
    return ss.str();
}

std::string Mangler::mangled(const ParamList& params) {
    std::stringstream ss;
    for (const auto& param : params)
        write_mangled(ss, param.type());
    return ss.str();
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

    // &
    if (type->is_ref()) {
        os << "r";
        type = type->as_ref()->refd()->canon();
        assert(type);
    }

    // []
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

    // bitsize
    detail::write_leximited(os, (Unsigned)type->bitsize());

    // type name/code
    if (type->is_class()) {
        auto cls = type->as_class();
        detail::write_leximited(os, cls->name());
        for (auto var : cls->param_vars()) {
            write_mangled(os, var->type());
            write_mangled(os, var->value());
        }
    } else {
        assert(type->is_prim());
        detail::write_leximited(os, builtin_type_code(type->builtin_type_id()));
        if (has_bitsize(type->builtin_type_id()))
            detail::write_leximited(os, (Unsigned)type->bitsize());
    }
}

void Mangler::write_mangled(std::ostream& os, const Value& value) {
    assert(value.is_nil());
    auto rval = value.rvalue();
    if (rval->is<Unsigned>()) {
        detail::write_leximited(os, rval->get<Unsigned>());
    } else if (rval->is<Integer>()){
        detail::write_leximited(os, rval->get<Integer>());
    // } else if (rval->is<Bool>()){
    //     detail::write_leximited(os, rval->get<Bool>());
    } else if (rval->is<String>()) {
        detail::write_leximited(os, rval->get<String>());
    } else {
        assert(false); // TODO
    }
}

} // namespace ulam
