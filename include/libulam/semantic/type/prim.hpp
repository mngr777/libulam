#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim/typed_value.hpp>
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
    using Type::is_castable;

    PrimType(Builtins& builtins, TypeIdGen* id_gen);

    bool is(BuiltinTypeId id) const { return builtin_type_id() == id; }

    Ref<PrimType> as_prim() override { return this; }
    Ref<const PrimType> as_prim() const override { return this; }

    bool is_impl_castable_to(BuiltinTypeId id) const {
        return is_castable_to(id, false);
    }
    bool is_expl_castable_to(BuiltinTypeId id) const {
        return is_castable_to(id, true);
    }

    bool is_impl_castable_to(Ref<PrimType> type) const {
        return is_castable_to(type, false);
    }
    bool is_expl_castable_to(Ref<PrimType> type) const {
        return is_castable_to(type, true);
    }

    virtual bool is_castable_to(BuiltinTypeId id, bool expl = true) const {
        return false;
    }
    virtual bool is_castable_to(Ref<PrimType> type, bool expl = true) const {
        return false;
    }

    virtual PrimTypedValue cast_to(BuiltinTypeId id, Value&& value) {
        return {};
    }
    virtual Value cast_to(Ref<PrimType> type, Value&& value) {
        return {};
    }

    // TODO: make this pure virtual, implement for builtins
    virtual TypedValue binary_op(
        Op op,
        const Value& left_val,
        Ref<const PrimType> right_type,
        const Value& right_val) {
        assert(false);
    };

protected:
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
            auto rval = arg.value().rvalue();
            assert(rval);
            if (rval->is<Integer>()) {
                auto int_val = rval->get<Integer>();
                size = (int_val < 0) ? 0 : static_cast<Unsigned>(int_val);
            } else if (rval->is<Unsigned>()) {
                size = rval->get<Unsigned>();
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
