#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/ops.hpp>

namespace ulam {

class Scope;
class Value;

using type_id_t = std::uint16_t;
using array_idx_t = std::uint16_t;
using array_size_t = array_idx_t;

constexpr array_size_t UnkArraySize = -1;

class ArrayType;
class BasicType;
class RefType;

class Type {
public:
    explicit Type(type_id_t id): _id{id} {}
    virtual ~Type() {}

    type_id_t id() const { return _id; }

    virtual bool is_placeholder() const { return true; }

    virtual Ref<Type> prev() = 0;
    virtual Ref<const Type> prev() const = 0;

    virtual Ref<BasicType> basic() = 0;
    virtual Ref<const BasicType> basic() const = 0;

    bool is_array() const { return array_size() != 0; }
    virtual array_size_t array_size() const { return 0; }

    virtual bool is_reference() const { return false; }

    // virtual op();

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
};

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

class AliasType : public _TypeDec {
public:
    AliasType(type_id_t id, Ref<Type> prev): _TypeDec{id, prev} {}
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
