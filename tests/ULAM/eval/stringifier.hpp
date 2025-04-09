#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>
#include <string>

class Stringifier {
public:
    Stringifier(ulam::Builtins& builtins, ulam::UniqStrPool& text_pool):
        _builtins{builtins}, _text_pool{text_pool} {}

    std::string stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval);

private:
    std::string
    stringify_prim(ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval);

    std::string
    stringify_class(ulam::Ref<ulam::Class> cls, const ulam::RValue& rval);

    std::string stringify_array(
        ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval);

    ulam::Builtins& _builtins;
    ulam::UniqStrPool& _text_pool;
};
