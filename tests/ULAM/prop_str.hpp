#pragma once
#include "./eval/stringifier.hpp"
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/value.hpp>
#include <string>

std::string prop_str(
    ulam::UniqStrPool& str_pool,
    Stringifier& stringifier,
    ulam::Ref<ulam::Prop> prop,
    ulam::RValue& obj);
