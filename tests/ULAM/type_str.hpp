#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <string>

std::string type_str(ulam::Ref<ulam::Type> type);

std::string type_base_name(ulam::Ref<ulam::Type> type);

std::string type_dim_str(ulam::Ref<ulam::Type> type);

std::string type_def_str(ulam::Ref<ulam::AliasType> alias);
