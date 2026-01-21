#pragma once
#include <string>

namespace ulam {

struct ClassOptions {
    bool lazy_class_id{true};
    std::string empty_element_name{"Empty"};
};

const ClassOptions DefaultClassOptions{};

} // namespace ulam
