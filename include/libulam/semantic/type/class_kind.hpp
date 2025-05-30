#pragma once
#include <string_view>

namespace ulam {

enum class ClassKind { Element, Quark, Transient, Union };

const std::string_view class_kind_str(ClassKind kind);

} // namespace ulam
