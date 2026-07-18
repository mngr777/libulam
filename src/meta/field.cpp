#include <libulam/meta/field.hpp>
#include <unordered_map>

namespace ulam::meta {

namespace {

const std::unordered_map<std::string_view, Field> Fields = {
#define FIELD(name, id, type, flags) {name, Field{name, id, Type::type, flags}},
#include <libulam/meta/fields.inc.hpp>
};

} // namespace

const Field* find_field(std::string_view name) {
    auto it = Fields.find(name);
    return (it != Fields.end()) ? &it->second : nullptr;
}

} // namespace ulam::meta
