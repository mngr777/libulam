#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_name_kind.hpp>

namespace ulam::utils {

str_id_t class_name_id(Ref<Program> program, Ref<Class> cls, ClassNameKind kind);

}
