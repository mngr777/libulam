#pragma once
#include <cstdint>
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {
class Node;
class BinaryOp;
class TypeDef;
} // namespace ulam::ast

namespace ulam {

class Scope;

using type_id_t = std::uint16_t;
constexpr type_id_t NoTypeId = 0;

using bitsize_t = std::uint16_t;
constexpr bitsize_t NoBitsize = 0;

using array_idx_t = std::uint16_t;
using array_size_t = array_idx_t;
constexpr array_size_t UnknownArraySize = -1;

class BasicType;
class PrimType;
class Class;
class AliasType;
class ArrayType;
class RefType;

class Type {
public:
    explicit Type(type_id_t id): _id{id} {}
    virtual ~Type() {}

    type_id_t id() const { return _id; }

    virtual Ref<Type> prev() = 0;
    virtual Ref<const Type> prev() const = 0;

    bool is_basic() const;

    virtual Ref<BasicType> basic() = 0;
    virtual Ref<const BasicType> basic() const = 0;

    virtual Ref<Type> canon() { return this; }
    virtual Ref<const Type> canon() const { return this; }

    bool is_array() const { return array_size() != 0; }
    virtual array_size_t array_size() const { return 0; }

    virtual bool is_reference() const { return false; }

    virtual bool is_convertible(Ref<const Type> type) { return false; }

    // TODO: move out of type?

    virtual Value cast(
        ast::Ref<ast::Node> node,
        Ref<const Type> type,
        const Value& value,
        bool is_impl = true) {
        return {};
    }

    virtual ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& left,
        Ref<const Type> right_type,
        const Value& right) {
        return {ExprError::NoOperator};
    };

    virtual Ref<ArrayType> array(array_size_t size) { return {}; };
    virtual Ref<RefType> reference() { return {}; }

private:
    type_id_t _id{};
};

class BasicType : public Type {
public:
    explicit BasicType(type_id_t id): Type{id} {}

    void add_op();

    Ref<Type> prev() override { return this; }
    Ref<const Type> prev() const override { return this; }

    Ref<BasicType> basic() override { return this; }
    Ref<const BasicType> basic() const override { return this; }

    virtual BuiltinTypeId builtin_type_id() const { return NoBuiltinTypeId; }

    bool is_class() const { return as_class(); }
    bool is_prim() const { return as_prim(); }
    bool is_alias() const { return as_alias(); }

    virtual Ref<Class> as_class() { return {}; }
    virtual Ref<const Class> as_class() const { return {}; }

    virtual Ref<PrimType> as_prim() { return {}; }
    virtual Ref<const PrimType> as_prim() const { return {}; }

    virtual Ref<AliasType> as_alias() { return {}; }
    virtual Ref<const AliasType> as_alias() const { return {}; }
};

class AliasType : public BasicType, public ScopeObject {
public:
    AliasType(type_id_t id, ast::Ref<ast::TypeDef> node, Ref<Class> owner = {}):
        BasicType{id}, _node(node) {}

    Ref<AliasType> as_alias() override { return this; }
    Ref<const AliasType> as_alias() const override { return this; }

    str_id_t name_id() const;

    ast::Ref<ast::TypeDef> node() { return _node; }

    Ref<Type> canon() override {
        return const_cast<Ref<Type>>(std::as_const(*this).canon());
    }

    Ref<const Type> canon() const override {
        if (!_aliased)
            return {};
        return _aliased->canon();
    }

    // ??
    Ref<Class> owner() { return _owner; }
    Ref<const Class> owner() const { return _owner; }

    Ref<Type> aliased() { return _aliased; }
    Ref<const Type> aliased() const { return _aliased; }

    void set_aliased(Ref<Type> type) { _aliased = type; }

private:
    ast::Ref<ast::TypeDef> _node;
    Ref<Class> _owner;
    Ref<Type> _aliased;
};

// TODO: canon() for decorators

class _TypeDec : public Type {
public:
    _TypeDec(type_id_t id, Ref<Type> prev): Type{id}, _prev{prev} {}
    virtual ~_TypeDec() = 0;

    Ref<Type> prev() override { return _prev; }
    Ref<const Type> prev() const override { return _prev; }

    Ref<BasicType> basic() override { return _prev->basic(); }
    Ref<const BasicType> basic() const override { return _prev->basic(); }

    array_size_t array_size() const override { return _prev->array_size(); }

    bool is_reference() const override { return _prev->is_reference(); }

private:
    Ref<Type> _prev;
};

class CompType : public _TypeDec {
public:
    CompType(type_id_t id, Ref<Type> prev): _TypeDec{id, prev} {}
    ~CompType() = 0;
};

class ArrayType : public CompType {
public:
    ArrayType(type_id_t id, Ref<Type> prev, array_size_t array_size):
        CompType{id, prev}, _array_size{array_size} {}

    array_size_t array_size() const override { return _array_size; }

private:
    array_size_t _array_size;
};

class RefType : public CompType {
public:
    RefType(type_id_t id, Ref<Type> prev): CompType{id, prev} {}

    bool is_reference() const override { return true; }
};

} // namespace ulam
