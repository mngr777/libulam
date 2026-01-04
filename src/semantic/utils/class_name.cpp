#include <libulam/semantic/utils/class_name.hpp>
#include <libulam/semantic/utils/strf.hpp>
#include <sstream>

namespace ulam::utils {

namespace {

// NOTE: no cache

std::string class_name_with_params_str(
    Ref<Program> program, Ref<Class> cls, bool include_args) {
    std::stringstream ss;
    ss << cls->name();
    if (cls->params().empty())
        return ss.str();

    Strf strf{program};
    ss << "(";
    for (const auto param : cls->params()) {
        if (param != *cls->params().begin())
            ss << ",";
        ss << param->type()->name();
        if (include_args)
            strf.str(ss << "=", param->type(), param->value());
    }
    ss << ")";
    return ss.str();
}

str_id_t class_name_with_params_str_id(
    Ref<Program> program, Ref<Class> cls, bool include_args) {
    auto str = class_name_with_params_str(program, cls, include_args);
    return program->text_pool().put(str);
}

str_id_t class_name_default_id(Ref<Program> program, Ref<Class> cls) {
    return program->text_pool().put(cls->name());
}

str_id_t class_name_signature_id(Ref<Program> program, Ref<Class> cls) {
    return class_name_with_params_str_id(program, cls, false);
}

str_id_t class_name_pretty_id(Ref<Program> program, Ref<Class> cls) {
    return class_name_with_params_str_id(program, cls, true);
}

str_id_t class_name_simple_id(Ref<Program> program, Ref<Class> cls) {
    return program->text_pool().put(cls->full_name());
}

str_id_t class_name_mangled_id(Ref<Program> program, Ref<Class> cls) {
    return program->text_pool().put(cls->mangled_name());
}

} // namespace

str_id_t
class_name_id(Ref<Program> program, Ref<Class> cls, ClassNameKind kind) {
    switch (kind) {
    case ClassName:
        return class_name_default_id(program, cls);
    case ClassSignature:
        return class_name_signature_id(program, cls);
    case ClassNamePretty:
        return class_name_pretty_id(program, cls);
    case ClassNameSimple:
        return class_name_simple_id(program, cls);
    case ClassNameMangled:
        return class_name_mangled_id(program, cls);
    default:
        assert(false);
    }
}

} // namespace ulam::utils
