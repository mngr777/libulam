#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>

namespace ulam {

class Scope;

using type_id_t = std::uint16_t;
using array_idx_t = std::uint16_t;
using array_size_t = array_idx_t;

constexpr array_size_t UnkArraySize = -1;

class Operator {};

class ArrayType;
class BaseType;
class RefType;

class Type {
public:
    explicit Type(type_id_t id): _id{id} {}
    virtual ~Type() {}

    type_id_t id() const { return _id; }

    virtual bool is_placeholder() const { return true; }

    virtual Ref<Type> prev() = 0;
    virtual Ref<const Type> prev() const = 0;

    virtual Ref<BaseType> base() = 0;
    virtual Ref<const BaseType> base() const = 0;

    bool is_array() const { return array_size() != 0; }
    virtual array_size_t array_size() const { return 0; }

    virtual bool is_ref() const { return false; }

    virtual Ref<Operator> op(Op op, Ref<Type> rhs_type) = 0;
    // virtual void add_op(Op op, Ref<Type> rhs_type, Ptr<Operator>&& oper);

    virtual Ref<ArrayType> array(array_size_t size) { return {}; };
    virtual Ref<RefType> reference() { return {}; }

private:
    type_id_t _id{};
};

class BaseType : public Type {
public:
    explicit BaseType(type_id_t id): Type{id} {}

    void add_op();

    Ref<Type> prev() override { return this; }
    Ref<const Type> prev() const override { return this; }

    Ref<BaseType> base() override { return this; }
    Ref<const BaseType> base() const override { return this; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; } // TODO
};

class PrimType : public BaseType {};

class CompType : public Type {
public:
    CompType(type_id_t id, Ref<Type> prev): Type{id}, _prev{prev} {}

    Ref<Type> prev() override { return _prev; }
    Ref<const Type> prev() const override { return _prev; }

    Ref<BaseType> base() override { return _prev->base(); }
    Ref<const BaseType> base() const override { return _prev->base(); }

    array_size_t array_size() const override { return _prev->array_size(); }

    bool is_ref() const override { return _prev->is_ref(); }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override {
        return _prev->op(op, rhs_type);
    }

private:
    Ref<Type> _prev;
};

class AliasType : public CompType {
public:
    AliasType(type_id_t id, Ref<Type> prev): CompType{id, prev} {}
};

class ArrayType : public CompType {
public:
    ArrayType(type_id_t id, Ref<Type> prev, array_size_t array_size):
        CompType{id, prev}, _array_size{array_size} {}

    array_size_t array_size() const override { return _array_size; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; }

private:
    array_size_t _array_size;
};

class RefType : public CompType {
public:
    RefType(type_id_t id, Ref<Type> prev): CompType{id, prev} {}

    bool is_ref() const override { return true; }
};

} // namespace ulam
