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
    explicit Stringifier(ulam::Ref<ulam::Program> program):
        _builtins{program->builtins()},
        _str_pool{program->str_pool()},
        _text_pool{program->text_pool()} {}

    std::string stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval);

    struct {
        bool unary_as_unsigned_lit = false;
        bool bool_as_unsigned_lit = false;
        bool use_unsigned_suffix = true;
        bool unary_no_unsigned_suffix = false;
        bool bits_use_unsigned_suffix = true;
        bool class_params_as_consts = true;
    } options;

private:
    std::string
    stringify_prim(ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval);

    std::string
    stringify_class(ulam::Ref<ulam::Class> cls, const ulam::RValue& rval);

    std::string stringify_array(
        ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval);

    std::string int_to_str(ulam::Integer val, ulam::bitsize_t size) const;
    std::string unsigned_to_str(ulam::Unsigned val) const;
    std::string unary_to_str(ulam::Unsigned val) const;
    std::string bits_to_str(const ulam::Bits& bits) const;

    ulam::Builtins& _builtins;
    ulam::UniqStrPool& _str_pool;
    ulam::UniqStrPool& _text_pool;
};
