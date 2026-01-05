#include "./utils.hpp"
#include <cerrno>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/utils/leximited.hpp>
#include <sstream>

namespace {

void write_mangled_rval(
    std::ostream& os,
    ulam::Ref<ulam::Program> program,
    const ulam::RValue& rval) {
    using ulam::detail::write_leximited;

    rval.accept(
        [&](const auto& val) { write_leximited(os, val); },
        [&](const ulam::String& str) {
            write_leximited(os, program->text_pool().get(str.id));
        },
        [&](const ulam::Bits& val) { val.write_hex(os); },
        [&](const ulam::DataPtr& val) { val->bits().write_hex(os); },
        [&](const std::monostate&) { assert(false); });
}

void write_mangled_type_name(
    std::ostream& os,
    ulam::Ref<ulam::Program> program,
    ulam::Ref<ulam::Type> type) {
    using ulam::detail::write_leximited;

    // use canonical type
    type = type->canon();

    // ref?
    if (type->is_ref())
        os << 'r';
    type = type->deref();

    // array size or 0
    if (type->is_array()) {
        auto array_type = type->as_array();
        write_leximited(os, (ulam::Unsigned)array_type->array_size());
        type = array_type->non_array();
    } else {
        write_leximited(os, (ulam::Unsigned)0);
    }

    if (type->is_class()) {
        auto cls = type->as_class();
        // bitsize (properties)
        write_leximited(os, (ulam::Unsigned)cls->props_bitsize());
        // name
        write_leximited(os, cls->name());
        // params
        write_leximited(os, (ulam::Unsigned)cls->params().size());
        for (auto param : cls->params()) {
            write_mangled_type_name(os, program, param->type());
            param->value().with_rvalue([&](const ulam::RValue& rval) {
                write_mangled_rval(os, program, rval);
            });
        }
    } else {
        assert(type->is_builtin());
        // bitsize
        write_leximited(os, (ulam::Unsigned)type->bitsize());
        // type code
        write_leximited(os, ulam::builtin_type_code(type->bi_type_id()));
    }
}

} // namespace

std::string class_prefix(ulam::ClassKind kind) {
    switch (kind) {
    case ulam::ClassKind::Element:
        return "Ue_";
    case ulam::ClassKind::Transient:
        return "Un_";
    default:
        return "Uq_";
    }
}

std::string class_name(ulam::Ref<ulam::Class> cls) {
    return class_prefix(cls->kind()) + std::string{cls->name()};
}

std::string class_name_mangled(
    ulam::Ref<ulam::Program> program, ulam::Ref<ulam::Class> cls) {
    std::stringstream ss;
    ss << class_prefix(cls->kind());
    write_mangled_type_name(ss, program, cls);
    return ss.str();
}
