#pragma once
#include <libulam/meta/id.hpp>
#include <libulam/meta/type.hpp>
#include <string_view>

namespace ulam::meta {

class Field {
public:
    Field(std::string_view name, Id id, Type type):
        _name{name}, _id{id}, _type{type} {}

    Field() {}

    bool empty() const { return _id == None; }

    std::string_view name() const { return _name; }
    Id id() const { return _id; }
    Type type() const { return _type; }

private:
    std::string_view _name;
    Id _id{None};
    Type _type{Type::String};
};

const Field* find_field(std::string_view name);

} // namespace ulam::meta
