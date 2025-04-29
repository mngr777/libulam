#include "./stringifier.hpp"
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
    assert(!rval.empty());

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
    default:
        assert(false); // TODO
    }
}

std::string Stringifier::stringify_class(
    ulam::Ref<ulam::Class> cls, const ulam::RValue& rval) {
    std::string str;

    // TODO
    // // typedefs
    // bool use_unsigned_suffix = options.use_unsigned_suffix;
    // options.use_unsigned_suffix = false;
    // for (auto type_def : cls->type_defs())
    //     str += " " + out::type_def_str(*this, type_def) + "; ";
    // options.use_unsigned_suffix = use_unsigned_suffix;

    // // params
    // // NOTE: see t3364, t3396 for property value with/without params
    // if (options.class_params_as_consts) {
    //     for (auto var : cls->params())
    //         str += " " + out::var_str(_str_pool, *this, var) + "; ";
    // }

    // // props
    // auto rval_copy = rval.copy(); // TMP
    // for (auto prop : cls->props())
    //     str += " " + out::prop_str(_str_pool, *this, prop, rval_copy) + "; ";

    return str;
}

std::string Stringifier::stringify_array(
    ulam::Ref<ulam::ArrayType> array_type, const ulam::RValue& rval) {
    if (array_type->array_size() == 0)
        return " ";

    std::stringstream ss;
    if (options.array_as_32_bit_chunks) { // t3881
        const auto& bits = rval.data_view().bits();
        ss << "{ ";

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
        ss << ((bits.len() > 0) ? " " : "") << "}";

    } else {
        auto item_type = array_type->item_type();
        auto data = rval.data_view();
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
    }
    return ss.str();
}

std::string
Stringifier::int_to_str(ulam::Integer val, ulam::bitsize_t size) const {
    std::ostringstream ss;
    if (size > 32) {
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
        std::uint32_t hi = val >> 32;
        std::uint32_t lo = (val << 32) >> 32;
        ss << "HexU64(" << std::hex << "0x" << hi << ", 0x" << lo << ")";
    } else {
        ss << val;
        if (options.use_unsigned_suffix &&
            (val != 0 || options.use_unsigned_suffix_zero))
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
        return bits.hex();

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
