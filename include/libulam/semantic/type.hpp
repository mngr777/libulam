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

    virtual Ref<Type> canon() { return this; }
    virtual Ref<const Type> canon() const { return this; }

    virtual BuiltinTypeId builtin_type_id() const { return NoBuiltinTypeId; }

    bool is_prim() const { return as_prim(); }
    bool is_class() const { return as_prim(); }
    bool is_alias() const { return as_alias(); }
    bool is_array() const { return as_array(); }
    bool is_ref() const { return as_ref(); }

    virtual Ref<PrimType> as_prim() { return {}; }
    virtual Ref<const PrimType> as_prim() const { return {}; }

    virtual Ref<Class> as_class() { return {}; }
    virtual Ref<const Class> as_class() const { return {}; }

    virtual Ref<AliasType> as_alias() { return {}; }
    virtual Ref<const AliasType> as_alias() const { return {}; }

    virtual Ref<ArrayType> as_array() { return {}; }
    virtual Ref<const ArrayType> as_array() const { return {}; }

    virtual Ref<RefType> as_ref() { return {}; }
    virtual Ref<const RefType> as_ref() const { return {}; }

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

class UserType : public Type, public ScopeObject {
public:
    explicit UserType(type_id_t id): Type{id} {}

    virtual str_id_t name_id() const = 0;
};

class AliasType : public UserType {
public:
    AliasType(type_id_t id, ast::Ref<ast::TypeDef> node, Ref<Class> owner = {}):
        UserType{id}, _node(node) {}

    Ref<AliasType> as_alias() override { return this; }
    Ref<const AliasType> as_alias() const override { return this; }

    str_id_t name_id() const override;

    ast::Ref<ast::TypeDef> node() { return _node; }

    Ref<Type> canon() override {
        return const_cast<Ref<Type>>(std::as_const(*this).canon());
    }

    Ref<const Type> canon() const override {
        if (!_aliased)
            return {};
        return _aliased->canon();
    }

    Ref<Type> aliased() { return _aliased; }
    Ref<const Type> aliased() const { return _aliased; }

    void set_aliased(Ref<Type> type) { _aliased = type; }

private:
    ast::Ref<ast::TypeDef> _node;
    Ref<Type> _aliased;
};

class ArrayType : public Type {
public:
    ArrayType(type_id_t id, Ref<Type> item_type, array_size_t array_size):
        Type{id}, _item_type{item_type}, _array_size{array_size} {}

    Ref<ArrayType> as_array() override { return this; }
    Ref<const ArrayType> as_array() const override { return this; }

    Ref<Type> item_type() { return _item_type; }
    Ref<const Type> item_type() const { return _item_type; }

    array_size_t array_size() const { return _array_size; }

private:
    Ref<Type> _item_type;
    array_size_t _array_size;
};

class RefType : public Type {
public:
    RefType(type_id_t id, Ref<Type> refd): Type{id}, _refd{refd} {}

    Ref<RefType> as_ref() override { return this; }
    Ref<const RefType> as_ref() const override { return this; }

    Ref<Type> refd() { return _refd; }
    Ref<const Type> refd() const { return _refd; }

private:
    Ref<Type> _refd;
};

} // namespace ulam
