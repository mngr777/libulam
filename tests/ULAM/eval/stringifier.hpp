#pragma once
#include <cstdint>
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
    using flags_t = std::uint16_t;
    static constexpr flags_t NoFlags = 0;
    static constexpr flags_t BoolAsUnsignedLit = 1;
    static constexpr flags_t UnaryAsUnsignedLit = 1 << 1;

    explicit Stringifier(ulam::Ref<ulam::Program> program):
        _builtins{program->builtins()},
        _str_pool{program->str_pool()},
        _text_pool{program->text_pool()} {}

    std::string stringify(
        ulam::Ref<ulam::Type> type,
        const ulam::RValue& rval,
        flags_t flags = NoFlags);

    struct {
        bool use_unsigned_suffix = true;
        bool bits_use_unsigned_suffix = true;
    } options;

private:
    std::string stringify_prim(
        ulam::Ref<ulam::PrimType> type,
        const ulam::RValue& rval,
        flags_t flags);

    std::string stringify_class(
        ulam::Ref<ulam::Class> cls, const ulam::RValue& rval, flags_t flags);

    std::string stringify_array(
        ulam::Ref<ulam::ArrayType> array_type,
        const ulam::RValue& rval,
        flags_t flags);

    std::string unsigned_to_str(ulam::Unsigned val);
    std::string bits_to_str(const ulam::Bits& bits);

    ulam::Builtins& _builtins;
    ulam::UniqStrPool& _str_pool;
    ulam::UniqStrPool& _text_pool;
};
