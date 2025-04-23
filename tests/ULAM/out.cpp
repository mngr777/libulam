#include "./out.hpp"
#include "libulam/semantic/value.hpp"
#include <sstream>

namespace out {

std::string
type_str(Stringifier& stringifier, ulam::Ref<ulam::Type> type, bool with_dims) {
    auto str = type_base_name(type);
    bool is_ref = type->is_ref();
    type = type->deref();
    if (type->is_class())
        str += class_param_str(stringifier, type->as_class());
    if (with_dims)
        str += type_dim_str(type);
    if (is_ref)
        str += "&";
    return str;
}

std::string
class_param_str(Stringifier& stringifier, ulam::Ref<ulam::Class> cls) {
    auto params = cls->params();
    if (params.empty())
        return "";

    std::string str;
    bool use_unsigned_suffix = stringifier.options.use_unsigned_suffix;
    bool unary_no_unsigned_suffix =
        stringifier.options.unary_no_unsigned_suffix;
    stringifier.options.use_unsigned_suffix = true;
    stringifier.options.unary_no_unsigned_suffix = true; // t3429 `Bar(3)`
    for (auto param : params) {
        if (!str.empty())
            str += ",";
        assert(param->has_value());
        assert(param->has_type());
        auto rval = param->value().copy_rvalue(); // TMP
        str += stringifier.stringify(param->type(), rval);
    }
    stringifier.options.use_unsigned_suffix = use_unsigned_suffix;
    stringifier.options.unary_no_unsigned_suffix = unary_no_unsigned_suffix;
    return "(" + str + ")";
}

std::string type_base_name(ulam::Ref<ulam::Type> type) {
    type = type->canon()->deref();
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

std::string
type_def_str(Stringifier& stringifier, ulam::Ref<ulam::AliasType> alias) {
    auto aliased = alias->aliased();
    return "typedef " + type_str(stringifier, aliased, false) + " " +
           alias->name() + type_dim_str(aliased);
}

std::string var_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Var> var) {
    auto str = var_def_str(str_pool, stringifier, var);
    if (var->has_value()) {
        var->value().with_rvalue([&](const ulam::RValue& rval) {
            auto unary_as_unsigned_lit =
                stringifier.options.unary_as_unsigned_lit;
            stringifier.options.unary_as_unsigned_lit = true;
            str += " = " + stringifier.stringify(var->type(), rval);
            stringifier.options.unary_as_unsigned_lit = unary_as_unsigned_lit;
        });
    }
    return str;
}

std::string var_def_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Var> var) {
    std::ostringstream os;
    if (var->is_const())
        os << "constant ";
    std::string name{str_pool.get(var->name_id())};
    os << type_str(stringifier, var->type(), false) << " " << name
       << type_dim_str(var->type());
    return os.str();
}

std::string prop_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Prop> prop,
    ulam::RValue& obj,
    bool inner) {
    std::ostringstream os;
    auto type = prop->type();
    auto rval = obj.prop(prop).rvalue(); // TMP
    os << type_str(stringifier, type, false) << " "
       << str_pool.get(prop->name_id()) << type_dim_str(type);

    auto val_str = stringifier.stringify(type, rval);

    // check if already parenthesized, workaround for object array format
    // `Class a[2](Type prop1(val);), (Type prop1(val);)`
    // (each object value individually wrapped, see `Poo.mbar` in t3230)
    // alternative: do not add `(' for first and `)' for last item in
    // `Stringifier::stringify_array`
    if (!val_str.empty() && val_str[0] == '(') {
        os << val_str;
    } else {
        os << "(" << val_str << ")";
    }

    return os.str();
}

} // namespace out
