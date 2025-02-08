#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
#include <ostream>
#include <string>

namespace ulam {

class Mangler {
public:
    std::string mangled(const TypedValueList& values);
    std::string mangled(const TypeList& types);

    void write_mangled(std::ostream& os, const TypedValue& tv);
    void write_mangled(std::ostream& os, Ref<const Type> type);
    void write_mangled(std::ostream& os, const RValue* rvalue);
};

} // namespace ulam
