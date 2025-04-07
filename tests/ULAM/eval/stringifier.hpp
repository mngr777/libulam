#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>
#include <string>

class Stringifier {
public:
    Stringifier(ulam::UniqStrPool& text_pool): _text_pool{text_pool} {}

    std::string stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval);

private:
    std::string
    stringify_prim(ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval);

    ulam::UniqStrPool& _text_pool;
};
