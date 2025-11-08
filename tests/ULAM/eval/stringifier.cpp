#include "./stringifier.hpp"
#include "libulam/semantic/value/data.hpp"
#include "src/semantic/detail/leximited.hpp"
#include <cassert>
#include <cstdint>
#include <libulam/semantic/detail/integer.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/value/types.hpp>
#include <sstream>
#include <string>

std::string
Stringifier::stringify(ulam::Ref<ulam::Type> type, const ulam::RValue& rval) {
    // TEST
    // assert(!rval.empty());
    if (rval.empty())
        return "<empty>";

    type = type->deref();

    if (type->is_prim())
        return stringify_prim(type->as_prim(), rval);

    if (type->is_class())
        return stringify_class(type->as_class(), rval);

    if (type->is_array())
        return stringify_array(type->as_array(), rval);

    if (type->is_atom())
        return "Atom";

    assert(false);
}

std::string Stringifier::stringify_prim(
    ulam::Ref<ulam::PrimType> type, const ulam::RValue& rval) {
    switch (type->bi_type_id()) {
    case ulam::IntId: {
        auto int_val = rval.get<ulam::Integer>();
        return int_to_str(int_val, type->bitsize());
    }
    case ulam::UnsignedId: {
        return unsigned_to_str(rval.get<ulam::Unsigned>(), type->bitsize());
    }
    case ulam::UnaryId: {
        if (options.unary_as_unsigned_lit)
            return unsigned_to_str(rval.get<ulam::Unsigned>(), type->bitsize());
        auto unary_type = _builtins.unary_type(type->bitsize());
        auto uns_val = unary_type->unsigned_value(rval);
        return unary_to_str(uns_val);
    }
    case ulam::BoolId: {
        if (options.bool_as_unsigned_lit)
            return unsigned_to_str(rval.get<ulam::Unsigned>(), type->bitsize());
        auto bool_type = _builtins.bool_type(type->bitsize());
        return bool_type->is_true(rval) ? "true" : "false";
    }
    case ulam::BitsId: {
        return bits_to_str(rval.get<ulam::Bits>());
    }
    case ulam::StringId: {
        auto str_id = rval.get<ulam::String>().id;

        // NOTE: union String props can have "invalid" IDs
        if (!_text_pool.has_id(str_id))
            return options.invalid_string_id_as_empty ? ""
                                                      : "UNINITIALIZED_STRING";

        auto str = _text_pool.get(str_id);
        if (options.empty_string_as_empty && str.empty())
            return {};
        return str_lit(str);
    }
    default:
        assert(false); // TODO
    }
}

std::string Stringifier::stringify_class(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    switch (options.object_fmt) {
    case ObjectFmt::Chunks:
        return stringify_class_chunks(cls, rval);
    case ObjectFmt::HexStr:
        return stringify_class_hex_str(cls, rval);
    case ObjectFmt::Map:
        return stringify_class_map(cls, rval);
    default:
        assert(false);
    }
}

// t41277
std::string Stringifier::stringify_class_chunks(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    auto bits =
        rval.data_view().bits().view(cls->data_off(), cls->data_bitsize());
    return "{ " + data_as_chunks(bits) + (bits.len() > 0 ? " " : "") + "}";
}

std::string Stringifier::stringify_class_hex_str(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    assert(rval.is<ulam::DataPtr>());
    auto data = rval.get<ulam::DataPtr>();
    auto data_view = data->bits().view(cls->data_off(), cls->data_bitsize());
    return data_view.hex();
}

std::string Stringifier::stringify_class_map(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    std::string str;
    for (auto prop : cls->all_props()) {
        auto lval = rval.prop(prop);
        lval.with_rvalue([&](const auto& prop_rval) {
            if (!str.empty())
                str += ", ";
            auto label = "." + std::string{_str_pool.get(prop->name_id())};
            auto val = stringify(prop->type(), prop_rval);
            str += label + " = " + val;
        });
    }
    return "{ " + str + " }";
}

std::string Stringifier::stringify_array(
    ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval) {
    if (array_type->array_size() == 0)
        return " ";

    // always used default format for string arrays, t3953
    auto fmt = options.array_fmt;
    if (array_type->item_type()->canon()->is(ulam::StringId))
        fmt = ArrayFmt::Default;

    switch (fmt) {
    case ArrayFmt::Chunks:
        return stringify_array_chunks(array_type, rval);
    case ArrayFmt::Leximited:
        return stringify_array_leximited(array_type, rval);
    default:
        return stringify_array_default(array_type, rval);
    }
}

// t3881
std::string Stringifier::stringify_array_chunks(
    ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval) {
    auto bits = rval.data_view().bits().view();
    return "{ " + data_as_chunks(bits) + (bits.len() > 0 ? " " : "") + "}";
}

std::string Stringifier::stringify_array_leximited(
    ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval) {
    std::stringstream ss;
    auto data = rval.data_view();

    // t3894
    const auto item_size = array_type->item_type()->bitsize();
    assert(item_size < sizeof(ulam::Unsigned) * 8);
    for (ulam::array_idx_t idx = 0; idx < array_type->array_size(); ++idx) {
        auto item_rval = data.array_item(idx).load();
        item_rval.accept(
            [&](ulam::Unsigned val) { ulam::detail::write_leximited(ss, val); },
            [&](ulam::Integer val) { ulam::detail::write_leximited(ss, val); },
            [&](auto&&) { assert(false); });
    }
    return ss.str();
}

std::string Stringifier::stringify_array_default(
    ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval) {
    std::stringstream ss;
    auto data = rval.data_view();
    auto item_type = array_type->item_type();
    if (!item_type->is_class())
        ss << "{ ";
    for (ulam::array_idx_t idx = 0; idx < array_type->array_size(); ++idx) {
        if (idx > 0)
            ss << ", ";
        if (item_type->is_class())
            ss << "(";
        auto item_rval = data.array_item(idx).load();
        ss << stringify(item_type, item_rval);
        if (item_type->is_class())
            ss << ")";
    }
    if (!item_type->is_class())
        ss << ((array_type->array_size() > 0) ? " " : "") << "}";
    return ss.str();
}

std::string
Stringifier::int_to_str(ulam::Integer val, ulam::bitsize_t size) const {
    std::ostringstream ss;
    if (size > 32) {
        if (val == 0 && options.hex_u64_zero_as_int)
            return "0";
        std::uint32_t hi = val >> 32;
        std::uint32_t lo = (val << 32) >> 32;
        ss << "HexU64(" << std::hex << "0x" << hi << ", 0x" << lo << ")";
    } else {
        ss << val;
    }
    return ss.str();
}

std::string
Stringifier::unsigned_to_str(ulam::Unsigned val, ulam::bitsize_t size) const {
    std::ostringstream ss;
    if (size > 32) {
        if (val == 0 && options.hex_u64_zero_as_int)
            return "0";
        std::uint32_t hi = val >> 32;
        std::uint32_t lo = (val << 32) >> 32;
        ss << "HexU64(" << std::hex << "0x" << hi << ", 0x" << lo << ")";
    } else {
        ss << val;
        bool is_zero = val == 0;
        bool add_suffix = false;
        if (options.use_unsigned_suffix)
            add_suffix = (!is_zero || options.use_unsigned_suffix_zero) &&
                         (options.use_unsigned_suffix_31bit ||
                          val < ulam::detail::unsigned_max(30));
        if (!add_suffix)
            add_suffix = (is_zero && options.use_unsigned_suffix_zero_force);
        if (add_suffix)
            ss << "u";
    }
    return ss.str();
}

std::string Stringifier::unary_to_str(ulam::Unsigned val) const {
    auto str = std::to_string(val);
    if (options.use_unsigned_suffix && !options.unary_no_unsigned_suffix &&
        (val != 0 || options.use_unsigned_suffix_zero))
        str += "u";
    return str;
}

std::string Stringifier::bits_to_str(const ulam::Bits& bits) const {
    if (options.short_bits_as_str || bits.len() > sizeof(ulam::Datum) * 8)
        return bits.empty() ? "0" : bits.hex();

    std::string str;
    auto datum = bits.read(0, bits.len());
    if (options.bits_32_as_signed_int && bits.len() == 32) { // t3806
        auto int_val = ulam::detail::integer_from_datum(datum, bits.len());
        str = std::to_string(int_val);
    } else {
        str = std::to_string((ulam::Unsigned)datum);
    }
    if (options.bits_use_unsigned_suffix)
        str += "u";
    return str;
}

std::string Stringifier::str_lit(const std::string_view str) {
    std::ostringstream ss;
    ss << '"';
    for (auto ch : str) {
        switch (ch) {
        case '"':
            ss << "\\\"";
            break;
        case '\0':
            ss << "\\0";
            break;
        case '\a':
            ss << "\\a";
            break;
        case '\b':
            ss << "\\b";
            break;
        case '\f':
            ss << "\\f";
            break;
        case '\n':
            ss << "\\n";
            break;
        case '\r':
            ss << "\\r";
            break;
        case '\t':
            ss << "\\t";
            break;
        case '\v':
            ss << "\\v";
            break;
        default:
            if (' ' <= ch && ch <= '~') {
                ss << ch;
            } else {
                ss << '\\' << std::oct << ch;
            }
        }
    }
    ss << '"';
    return ss.str();
}

std::string Stringifier::data_as_chunks(const ulam::BitsView bits) const {
    std::ostringstream ss;
    using size_t = ulam::Bits::size_t;
    const size_t ChunkSize = 32;
    const size_t Num = (bits.len() + ChunkSize - 1) / ChunkSize;
    const size_t LastChunkSize =
        (bits.len() % ChunkSize > 0) ? bits.len() % ChunkSize : ChunkSize;
    for (size_t i = 0; i < Num; ++i) {
        if (i > 0)
            ss << ", ";
        const ulam::Bits::size_t size =
            (i + 1 < Num) ? ChunkSize : LastChunkSize;
        auto chunk = bits.view(ChunkSize * i, size);
        chunk.write_hex(ss);
    }
    return ss.str();
}
