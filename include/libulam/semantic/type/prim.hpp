#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <string>
#include <unordered_map>

namespace ulam {

class Builtins;

// TODO: make virtual methods pure
class PrimType : public Type {
public:
    using Type::is_castable_to;

    PrimType(Builtins& builtins, TypeIdGen* id_gen);

    const std::string_view name() const override;

    bool is_constructible() const override { return true; }

    RValue load(const BitsView data, bitsize_t off) override;

    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    virtual RValue from_datum(Datum datum) { assert(false); }
    virtual Datum to_datum(const RValue& rval) { assert(false); }

    Ref<Type> common(Ref<Type> type) override;
    Ref<Type>
    common(const Value& val1, Ref<Type> type, const Value& val2) override;

    bool is_castable_to(
        Ref<const Type> type,
        const Value& val,
        bool expl = true) const override;

    bool is_castable_to(
        BuiltinTypeId bi_type_id,
        const Value& val,
        bool expl = true) const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    conv_cost_t conv_cost(
        Ref<const Type> type,
        const Value& val,
        bool allow_cast = false) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;
    TypedValue cast_to(BuiltinTypeId bi_type_id, Value&& val);

    virtual TypedValue unary_op(Op op, RValue&& rval) { assert(false); }

    virtual TypedValue binary_op(
        Op op,
        RValue&& left_rval,
        Ref<const PrimType> right_type,
        RValue&& right_rval) {
        assert(false);
    };

protected:
    Ref<Type> common_prim(Ref<PrimType> type);

    Ref<Type>
    common_prim(const Value& val1, Ref<PrimType> type, const Value& val2);

    virtual bool is_castable_to_prim(Ref<const PrimType> type, bool expl) const;

    virtual bool is_castable_to_prim(BuiltinTypeId bi_type_id, bool expl) const;

    // NOTE: these allow e.g. auto-casting Int to Unsigned if value is
    // consteval and  >= 0: `Unsigned a = 1;`
    virtual bool
    is_impl_castable_to_prim(Ref<const PrimType> type, const Value& val) const;
    virtual bool
    is_impl_castable_to_prim(BuiltinTypeId bi_type_id, const Value& val) const;

    // TODO: make pure virtual
    virtual TypedValue cast_to_prim(BuiltinTypeId id, RValue&& rval) {
        assert(false);
    }
    virtual RValue cast_to_prim(Ref<PrimType> type, RValue&& rval) {
        assert(false);
    }

    Ref<PrimType> _as_prim() override { return this; }
    Ref<const PrimType> _as_prim() const override { return this; }
};

class PrimTypeTpl;

template <
    BuiltinTypeId _TypeId,
    bitsize_t Min,
    bitsize_t Max,
    bitsize_t Default>
class _PrimType : public PrimType {
    static_assert(ulam::is_prim(_TypeId));
    static_assert(has_bitsize(_TypeId));
    static_assert(0 < Min);
    static_assert(Min <= Max);
    static_assert(Min <= Default && Default <= Max);

public:
    static constexpr BuiltinTypeId TypeId = _TypeId;
    static constexpr bitsize_t MinSize = Min;
    static constexpr bitsize_t MaxSize = Max;
    static constexpr bitsize_t DefaultSize = Default;

    _PrimType(
        Builtins& builtins,
        TypeIdGen& id_gen,
        Ref<PrimTypeTpl> tpl,
        bitsize_t bitsize):
        PrimType{builtins, &id_gen}, _tpl{tpl}, _bitsize{bitsize} {}

    const std::string_view name() const override {
        if (_name.empty()) {
            _name = PrimType::name();
            if (bitsize() != DefaultSize)
                _name += std::string{"("} + std::to_string(bitsize()) + ")";
        }
        return _name;
    }

    BuiltinTypeId bi_type_id() const override { return TypeId; }
    bitsize_t bitsize() const override { return _bitsize; }

protected:
    Ref<PrimTypeTpl> tpl() { return _tpl; }

private:
    Ref<PrimTypeTpl> _tpl;
    bitsize_t _bitsize;
    mutable std::string _name;
};

class PrimTypeTpl : public TypeTpl {
public:
    PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen);

    virtual Ref<PrimType> type(bitsize_t bitsize) = 0;

    virtual bitsize_t min_bitsize() const = 0;
    virtual bitsize_t max_bitsize() const = 0;

protected:
    Builtins& _builtins;
};

template <typename T> class _PrimTypeTpl : public PrimTypeTpl {
    static_assert(T::MaxSize > 0);

public:
    _PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen):
        PrimTypeTpl{builtins, id_gen} {}

    bitsize_t min_bitsize() const override { return T::MinSize; }
    bitsize_t max_bitsize() const override { return T::MaxSize; }

    Ref<PrimType> type(bitsize_t size) override { return get(size); }

    Ref<T> exact_type(bitsize_t size) { return get(size); }

private:
    Ref<T> get(bitsize_t size) {
        assert(T::MinSize <= size && size <= T::MaxSize);
        {
            auto it = _types.find(size);
            if (it != _types.end())
                return ref(it->second);
        }
        auto type = make<T>(_builtins, id_gen(), this, size);
        auto type_ref = ref(type);
        _types.emplace(size, std::move(type));
        return type_ref;
    }

    std::string type_str() const {
        return std::string{builtin_type_str(T::TypeId)};
    }

    std::unordered_map<bitsize_t, Ptr<T>> _types;
};

} // namespace ulam
