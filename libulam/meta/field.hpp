#pragma once
#include <libulam/meta/id.hpp>
#include <libulam/meta/type.hpp>
#include <string_view>

namespace ulam::meta {

class Field {
public:
    using flags_t = std::uint8_t;
    static constexpr flags_t NoFlags = 0;
    static constexpr flags_t IsMulti = 1;

    Field(std::string_view name, Id id, Type type, flags_t flags):
        _name{name}, _id{id}, _type{type}, _flags(flags) {}

    Field() {}

    bool empty() const { return _id == None; }

    std::string_view name() const { return _name; }
    Id id() const { return _id; }
    Type type() const { return _type; }

    bool has_flag(flags_t flag) const { return _flags & flag; }

private:
    std::string_view _name;
    Id _id{None};
    Type _type{Type::String};
    flags_t _flags{NoFlags};
};

const Field* find_field(std::string_view name);

} // namespace ulam::meta
