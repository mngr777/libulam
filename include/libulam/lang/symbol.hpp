#pragma once
#include <libulam/lang/ops.hpp>
#include <libulam/memory/ptr.hpp>
#include <string>

namespace ulam {

using array_idx_t = std::uint16_t;
using array_size_t = array_idx_t;

constexpr array_size_t UnkArraySize = -1;

class Type;

class TypeTpl {};

class PrimTypeTpl : public TypeTpl {};

class ClassTpl : public TypeTpl {};

class Operator {};

class BaseType;

class Type {
public:
    virtual ~Type();

    virtual Ref<Type> parent() = 0;
    virtual Ref<BaseType> base() = 0;
    virtual Ref<Operator> op(Op op, Ref<Type> rhs_type) = 0;
};

class BaseType : public Type {
public:
    Ref<Type> parent() override { return this; }
    Ref<BaseType> base() override { return this; }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override { return {}; } // TODO
};

class Class : public Type {};

class PrimType : public Type {
};

class TypeAlias : public Type {
public:
    TypeAlias(Ref<Type> parent): _parent{parent} {}

    Ref<Type> parent() override { return _parent; }
    Ref<BaseType> base() override { return _parent->base(); }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override {
        return _parent->op(op, rhs_type);
    }

private:
    Ref<Type> _parent;
};

class Var {
public:
    Var(Ref<Type> type, Ref<Var> refd, bool is_const, array_size_t array_size = 0):
        _type{type}, _refd{refd} {}
    Var(Ref<Type> type, bool is_const, array_size_t array_size = 0):
        Var{type, {}, is_const, array_size} {}

    bool is_ref() const { return !_refd; }

    Ref<Type> type() { return _type; }

    Ref<Var> refd() { return _refd; }
    Ref<Var> deref();

    bool is_const() const { return _is_const; }

    bool is_array() const { return array_size() != 0; }
    array_size_t array_size() const { return _array_size; }

private:
    Ref<Type> _type;
    Ref<Var> _refd{};
    bool _is_const;
    array_size_t _array_size;
};

class Fun {
public:
    Fun(Ref<Type> ret_type): _ret_type(ret_type) {}

    Ref<Type> ret_type() { return _ret_type; }

private:
    Ref<Type> _ret_type;
};

class Symbol {
public:
    enum Kind { Tpl, Type, Fun, Var };

    Symbol(std::string name) {}

    const std::string& name() const { return _name; }
    Kind kind() const { return _kind; }

private:
    std::string _name;
    Kind _kind;
};

} // namespace ulam
