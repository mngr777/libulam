#include <libulam/semantic/utils/class_name.hpp>
#include <libulam/semantic/utils/strf.hpp>
#include <sstream>

namespace ulam::utils {

namespace {

// NOTE: no cache

std::string class_name_full(
    Ref<Program> program,
    Ref<Class> cls,
    bool include_param_names,
    bool include_args) {

    std::stringstream ss;
    ss << cls->name();
    if (cls->params().empty())
        return ss.str();

    Strf strf{program};
    auto& str_pool = program->str_pool();
    ss << "(";
    for (const auto param : cls->params()) {
        // ,
        if (param != *cls->params().begin())
            ss << ",";
        if (include_param_names) {
            // type name
            ss << param->type()->name();
            // name
            ss << " " << str_pool.get(param->name_id());
        }
        // value
        if (include_args) {
            if (include_param_names)
                ss << "=";
            strf.str(ss, param->type(), param->value());
        }
    }
    ss << ")";
    return ss.str();
}

str_id_t class_name_full_id(
    Ref<Program> program,
    Ref<Class> cls,
    bool include_param_names,
    bool include_args) {

    auto str = class_name_full(program, cls, include_param_names, include_args);
    return program->text_pool().put(str);
}

str_id_t class_name_default_id(Ref<Program> program, Ref<Class> cls) {
    return program->text_pool().put(cls->name());
}

str_id_t class_name_signature_id(Ref<Program> program, Ref<Class> cls) {
    return class_name_full_id(program, cls, true, false);
}

str_id_t class_name_pretty_id(Ref<Program> program, Ref<Class> cls) {
    return class_name_full_id(program, cls, true, true);
}

str_id_t class_name_simple_id(Ref<Program> program, Ref<Class> cls) {
    return class_name_full_id(program, cls, false, true);
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
