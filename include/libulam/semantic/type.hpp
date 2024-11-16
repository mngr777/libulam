#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>

namespace ulam::ast {
class ClassDef;
}

namespace ulam {

class Scope;

using type_id_t = std::uint16_t;
using array_idx_t = std::uint16_t;
using array_size_t = array_idx_t;

constexpr array_size_t UnkArraySize = -1;

class TypeTpl {};

class ClassTpl : public TypeTpl {};

class Operator {};

class BaseType;

class Type {
public:
    explicit Type(type_id_t id): _id{id} {}
    virtual ~Type() {}

    type_id_t id() const { return _id; }

    virtual Ref<Type> parent() = 0;
    virtual Ref<const Type> parent() const = 0;

    virtual Ref<BaseType> base() = 0;
    virtual Ref<const BaseType> base() const = 0;

    bool is_array() const { return array_size() != 0; }
    virtual array_size_t array_size() const { return 0; }

    virtual bool is_ref() const { return false; }

    virtual Ref<Operator> op(Op op, Ref<Type> rhs_type) = 0;

private:
    type_id_t _id{};
};

class BaseType : public Type {
public:
    explicit BaseType(type_id_t id): Type{id} {}

    Ref<Type> parent() override { return this; }
    Ref<const Type> parent() const override { return this; }

    Ref<BaseType> base() override { return this; }
    Ref<const BaseType> base() const override { return this; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; } // TODO
};

class Class : public BaseType {
public:
    enum Kind { Element, Quark, Transient };

    explicit Class(type_id_t id, Ref<ast::ClassDef> node);
    ~Class();

    Ref<ast::ClassDef> node() { return _node; }

    Ref<Scope> scope() { return ref(_scope); }

private:
    Ref<ast::ClassDef> _node;
    Ptr<Scope> _scope;
};

class PrimType : public BaseType {};

class CompType : public Type {
public:
    CompType(type_id_t id, Ref<Type> parent): Type{id}, _parent{parent} {}

    Ref<Type> parent() override { return _parent; }
    Ref<const Type> parent() const override { return _parent; }

    Ref<BaseType> base() override { return _parent->base(); }
    Ref<const BaseType> base() const override { return _parent->base(); }

    array_size_t array_size() const override { return _parent->array_size(); }

    bool is_ref() const override { return _parent->is_ref(); }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override {
        return _parent->op(op, rhs_type);
    }

private:
    Ref<Type> _parent;
};

class TypeAlias : public CompType {
public:
    TypeAlias(type_id_t id, Ref<Type> parent): CompType{id, parent} {}
};

class ArrayType : public CompType {
public:
    ArrayType(type_id_t id, Ref<Type> parent, array_size_t array_size):
        CompType{id, parent}, _array_size{array_size} {}

    array_size_t array_size() const override { return _array_size; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; }

private:
    array_size_t _array_size;
};

class RefType : public CompType {
public:
    RefType(type_id_t id, Ref<Type> parent): CompType{id, parent} {}

    bool is_ref() const override { return true; }
};

} // namespace ulam
