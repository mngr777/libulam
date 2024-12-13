#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
#include <ostream>
#include <string>

namespace ulam {

class Mangler {
public:
    Mangler(UniqStrPool& str_pool): _str_pool{str_pool} {}

    std::string mangled(const TypedValueList& values);

    void write_mangled(std::ostream& os, const TypedValue& tv);
    void write_mangled(std::ostream& os, Ref<const Type> type);
    void write_mangled(std::ostream& os, const Value& type);

private:
    UniqStrPool& _str_pool;
};

} // namespace ulam
