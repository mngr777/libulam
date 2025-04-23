#pragma once
#include "./eval/stringifier.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>
#include <string>

namespace out {

std::string type_str(
    Stringifier& stringifier,
    ulam::Ref<ulam::Type> type,
    bool with_dims = true);

std::string
class_param_str(Stringifier& stringifier, ulam::Ref<ulam::Class> cls);

std::string type_base_name(ulam::Ref<ulam::Type> type);

std::string type_dim_str(ulam::Ref<ulam::Type> type);

std::string
type_def_str(Stringifier& stringifier, ulam::Ref<ulam::AliasType> alias);

std::string var_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Var> var);

std::string var_def_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Var> var);

std::string prop_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Prop> prop,
    ulam::RValue& obj,
    bool inner = false);

} // namespace out
