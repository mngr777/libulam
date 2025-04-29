#include "./out.hpp"
#include <sstream>

namespace {

ulam::Ref<ulam::Type> type_base(ulam::Ref<ulam::Type> type) {
    type = type->canon()->deref();
    while (type->is_array())
        type = type->as_array()->item_type();
    return type;
}

} // namespace

namespace out {

std::string
type_str(Stringifier& stringifier, ulam::Ref<ulam::Type> type, bool with_dims) {
    auto base = type_base(type);
    auto str = base->name();
    bool is_ref = type->is_ref();
    type = type->deref();
    if (base->is_class())
        str += class_param_str(stringifier, base->as_class());
    if (with_dims)
        str += type_dim_str(type);
    if (is_ref)
        str += "&";
    return str;
}

std::string
class_param_str(Stringifier& stringifier, ulam::Ref<ulam::Class> cls) {
    std::string str;
    auto params = cls->params();
    if (params.empty())
        return str;

    bool use_unsigned_suffix = stringifier.options.use_unsigned_suffix;
    bool unary_no_unsigned_suffix =
        stringifier.options.unary_no_unsigned_suffix;
    bool short_bits_as_str = stringifier.options.short_bits_as_str;
    auto array_fmt = stringifier.options.array_fmt;
    stringifier.options.use_unsigned_suffix = true;
    stringifier.options.unary_no_unsigned_suffix = true; // t3429 `Bar(3)`
    stringifier.options.short_bits_as_str = true;        // t3640
    stringifier.options.array_fmt = Stringifier::ArrayFmt::Leximited;

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
    stringifier.options.short_bits_as_str = short_bits_as_str;
    stringifier.options.array_fmt = array_fmt;

    return "(" + str + ")";
}

std::string type_dim_str(ulam::Ref<ulam::Type> type) {
    type = type->actual(); // NOTE: adding dimensions for array reference types,
                           // t3634
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
        os << (var->is_parameter() ? "parameter" : "constant") << " ";
    std::string name{str_pool.get(var->name_id())};
    os << type_str(stringifier, var->type(), false) << " " << name
       << type_dim_str(var->type());
    return os.str();
}

} // namespace out
