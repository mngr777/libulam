#include "./prop_str.hpp"
#include "./type_str.hpp"
#include <sstream>

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
            os << type_def_str(type_def);
    }
    os << stringifier.stringify(type, rval)
       << ")";
    return os.str();
}
