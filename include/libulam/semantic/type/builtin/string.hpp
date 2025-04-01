#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/str_pool.hpp>
#include <string>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;
class Value;

class StringType : public PrimType {
public:
    StringType(Builtins& builtins, TypeIdGen& id_gen, UniqStrPool& text_pool):
        PrimType{builtins, &id_gen}, _text_pool{text_pool} {}

    std::string name() const override { return "String"; }

    bitsize_t bitsize() const override;

    TypedValue type_op(TypeOp op) override;
    TypedValue type_op(TypeOp op, Value& val) override;

    RValue construct() override;

    RValue from_datum(Datum datum) override;
    Datum to_datum(const RValue& rval) override;

    str_len_t len(const Value& val) const;
    std::uint8_t chr(const Value& val, array_idx_t idx) const;

    std::string_view text(const Value& val) const;

    TypedValue binary_op(
        Op op,
        RValue&& l_rval,
        Ref<const PrimType> r_type,
        RValue&& r_rval) override;

    BuiltinTypeId bi_type_id() const override { return StringId; }

protected:
    bool is_castable_to_prim(
        Ref<const PrimType> type, bool expl = true) const override;
    bool is_castable_to_prim(BuiltinTypeId id, bool expl = true) const override;

    TypedValue cast_to_prim(BuiltinTypeId id, RValue&& rval) override;
    RValue cast_to_prim(Ref<PrimType> type, RValue&& rval) override;

private:
    UniqStrPool& _text_pool;
};

} // namespace ulam
