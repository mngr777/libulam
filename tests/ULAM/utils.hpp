#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <string>

std::string class_prefix(ulam::ClassKind kind);

std::string class_name(ulam::Ref<ulam::Class> cls);

std::string class_name_mangled(
    ulam::Ref<ulam::Program> program, ulam::Ref<ulam::Class> cls);
