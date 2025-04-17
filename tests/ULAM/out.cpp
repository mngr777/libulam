#include "./out.hpp"
#include <sstream>

namespace out {

std::string type_str(ulam::Ref<ulam::Type> type) {
    auto str = type_base_name(type) + type_dim_str(type);
    if (type->is_ref())
        str += "&";
    return str;
}

std::string type_base_name(ulam::Ref<ulam::Type> type) {
    type = type->canon();
    while (type->is_array())
        type = type->as_array()->item_type();
    return type->name();
}

std::string type_dim_str(ulam::Ref<ulam::Type> type) {
    type = type->canon();
    std::list<ulam::array_size_t> dims;
    while (type->is_array()) {
        auto array_type = type->as_array();
        dims.push_front(array_type->array_size());
        type = array_type->item_type();
    }
    std::ostringstream out;
    for (auto it = dims.begin(); it != dims.end(); ++it)
        out << "[" << *it << "]";
    return out.str();
}

std::string type_def_str(ulam::Ref<ulam::AliasType> alias) {
    auto aliased = alias->aliased();
    return "typedef " + type_base_name(aliased) + " " + alias->name() +
        type_dim_str(aliased);
}

std::string var_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Var> var) {
    auto str = var_def_str(str_pool, var);
    if (var->has_value()) {
        var->value().with_rvalue([&](const ulam::RValue& rval) {
            str += " = " + stringifier.stringify(var->type(), rval);
        });
    }
    return str;
}

std::string var_def_str(ulam::UniqStrPool& str_pool, ulam::Ref<ulam::Var> var) {
    std::ostringstream os;
    if (var->is_const())
        os << "constant ";
    std::string name{str_pool.get(var->name_id())};
    os << type_base_name(var->type()) << " " << name
       << type_dim_str(var->type());
    return os.str();
}

std::string prop_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Prop> prop,
    ulam::RValue& obj) {
    std::ostringstream os;
    assert(str_pool.has_id(prop->name_id()));
    auto type = prop->type();
    auto rval = obj.prop(prop).rvalue(); // TMP
    os << type_base_name(type) << " " << str_pool.get(prop->name_id())
       << type_dim_str(type) << "(";
    if (type->is_class()) {
        for (auto type_def : type->as_class()->type_defs())
            os << type_def_str(type_def) << "; ";
    }
    os << stringifier.stringify(type, rval) << ")";
    return os.str();
}

} // namespace out
