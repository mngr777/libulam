#include "./type_str.hpp"
#include <sstream>

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
           type_dim_str(aliased) + "; ";
}
