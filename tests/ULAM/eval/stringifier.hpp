#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>
#include <string>

class Stringifier {
public:
    explicit Stringifier(ulam::Ref<ulam::Program> program, bool is_main = true):
        _builtins{program->builtins()},
        _str_pool{program->str_pool()},
        _text_pool{program->text_pool()},
        _is_main{is_main} {}

    std::string stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval);

private:
    std::string
    stringify_prim(ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval);

    std::string
    stringify_class(ulam::Ref<ulam::Class> cls, const ulam::RValue& rval);

    std::string stringify_array(
        ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval);

    ulam::Builtins& _builtins;
    ulam::UniqStrPool& _str_pool;
    ulam::UniqStrPool& _text_pool;
    bool _is_main;
};
