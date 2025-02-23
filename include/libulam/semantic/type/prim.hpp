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
    PrimType(Builtins& builtins, TypeIdGen* id_gen);

    std::string name() const override;

    bool is(BuiltinTypeId id) const { return builtin_type_id() == id; }

    Ref<PrimType> as_prim() override { return this; }
    Ref<const PrimType> as_prim() const override { return this; }

    RValue load(const BitVectorView data, BitVector::size_t off) const override;

    void store(BitVectorView data, BitVector::size_t off, const RValue& rval)
        const override;

    virtual RValue from_datum(Datum datum) const { assert(false); }
    virtual Datum to_datum(const RValue& rval) const { assert(false); }

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;

    bool is_castable_to(
        BuiltinTypeId builtin_type_id, bool expl = true) const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    RValue cast_to(Ref<Type> type, RValue&& rval) override {
        auto canon = type->canon();
        return canon->is_prim() ? cast_to(canon->as_prim(), std::move(rval))
                                : RValue{};
    }

    virtual TypedValue cast_to(BuiltinTypeId id, RValue&& value) {
        assert(false);
    }
    virtual RValue cast_to(Ref<PrimType> type, RValue&& value) {
        assert(false);
    }

    virtual TypedValue unary_op(Op op, RValue&& rval) { assert(false); }

    virtual TypedValue binary_op(
        Op op,
        RValue&& left_rval,
        Ref<const PrimType> right_type,
        RValue&& right_rval) {
        assert(false);
    };

protected:
    virtual bool
    is_castable_to_prim(Ref<const PrimType> type, bool expl) const {
        return false;
    }

    Builtins& builtins() { return _builtins; }

private:
    Builtins& _builtins;
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

    BuiltinTypeId builtin_type_id() const override { return TypeId; }
    bitsize_t bitsize() const override { return _bitsize; }

protected:
    Ref<PrimTypeTpl> tpl() { return _tpl; }

private:
    Ref<PrimTypeTpl> _tpl;
    bitsize_t _bitsize;
};

class PrimTypeTpl : public TypeTpl {
public:
    PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen);

    Ref<Type> type(
        Diag& diag,
        Ref<ast::ArgList> args_node,
        TypedValueList&& args) override = 0;

    virtual Ref<PrimType>
    type(Diag& diag, Ref<ast::Node> node, bitsize_t bitsize) = 0;

    virtual Ref<PrimType> type(bitsize_t bitsize) = 0;

protected:
    Builtins& _builtins;
};

template <typename T> class _PrimTypeTpl : public PrimTypeTpl {
    static_assert(T::MaxSize > 0);

public:
    _PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen):
        PrimTypeTpl{builtins, id_gen} {}

    // TODO: this should probably be implemented at higher level ??
    Ref<Type> type(
        Diag& diag,
        Ref<ast::ArgList> args_node,
        TypedValueList&& args) override {
        // NOTE: args_node can be null
        bitsize_t size = 0;
        if (args.size() == 0) {
            size = T::DefaultSize;
        } else {
            if (args.size() > 1) {
                diag.emit(
                    Diag::Error, args_node->loc_id(), 1,
                    std::string("too many arguments for ") + type_str());
                // continue
            }
            // get first arg
            auto& arg = args.front();
            auto rval = arg.value().move_rvalue();
            assert(!rval.empty());
            if (rval.is<Integer>()) {
                auto int_val = rval.get<Integer>();
                size = (int_val < 0) ? 0 : static_cast<Unsigned>(int_val);
            } else if (rval.is<Unsigned>()) {
                size = rval.get<Unsigned>();
            } else {
                diag.emit(
                    Diag::Error, args_node->loc_id(), 1,
                    "cannot convert to bit size");
                return {};
            }
        }
        return type(diag, args_node, size);
    }

    Ref<PrimType>
    type(Diag& diag, Ref<ast::Node> node, bitsize_t size) override {
        // check, adjust and continue on error
        if (size < T::MinSize) {
            diag.emit(
                Diag::Error, node->loc_id(), 1,
                std::string("bit size argument must be at least ") +
                    std::to_string(T::MinSize));
            size = T::MinSize;
        } else if (size > T::MaxSize) {
            diag.emit(
                Diag::Error, node->loc_id(), 1,
                std::string("bit size argument must be at most ") +
                    std::to_string(T::MaxSize));
            size = T::MaxSize;
        }
        return get(size);
    }

    Ref<PrimType> type(bitsize_t size) override { return get(size); }

private:
    Ref<PrimType> get(bitsize_t size) {
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
