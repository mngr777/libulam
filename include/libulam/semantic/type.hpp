#pragma once
#include <libulam/semantic/ops.hpp>
#include <libulam/memory/ptr.hpp>

namespace ulam::ast {
class ClassDef;
}

namespace ulam {

using array_idx_t = std::uint16_t;
using array_size_t = array_idx_t;

constexpr array_size_t UnkArraySize = -1;

class TypeTpl {};

class ClassTpl : public TypeTpl {};

class Operator {};

class BaseType;

class Type {
public:
    virtual ~Type() {}

    virtual bool is_defined() const = 0;

    virtual Ref<Type> parent() = 0;
    virtual Ref<const Type> parent() const = 0;

    virtual Ref<BaseType> base() = 0;
    virtual Ref<const BaseType> base() const = 0;

    bool is_array() const { return array_size() != 0; }
    virtual array_size_t array_size() const { return 0; }

    virtual bool is_ref() const { return false; }

    virtual Ref<Operator> op(Op op, Ref<Type> rhs_type) = 0;
};

class BaseType : public Type {
public:
    BaseType(bool is_defined = true): _is_defined{is_defined} {}

    bool is_defined() const override { return _is_defined; }

    Ref<Type> parent() override { return this; }
    Ref<const Type> parent() const override { return this; }

    Ref<BaseType> base() override { return this; }
    Ref<const BaseType> base() const override { return this; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; } // TODO

private:
    bool _is_defined;
};

class Class : public BaseType {
public:
    enum Kind { Element, Quark, Transient };

    Class(Ref<ast::ClassDef> node);
    ~Class();

    Ref<ast::ClassDef> node() { return _node; }

private:
    Ref<ast::ClassDef> _node;
};

class PrimType : public BaseType {};

class _TypeDec : public Type {
public:
    _TypeDec(Ref<Type> parent): _parent{parent} {}

    bool is_defined() const override { return base()->is_defined(); }

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

class TypeAlias : public _TypeDec {
public:
    TypeAlias(Ref<Type> parent): _TypeDec{parent} {}
};

class ArrayType : public _TypeDec {
public:
    ArrayType(Ref<Type> parent, array_size_t array_size):
        _TypeDec{parent}, _array_size{array_size} {}

    array_size_t array_size() const override { return _array_size; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; }

private:
    array_size_t _array_size;
};

class RefType : public _TypeDec {
public:
    RefType(Ref<Type> parent): _TypeDec{parent} {}

    bool is_ref() const override { return true; }
};

} // namespace ulam
