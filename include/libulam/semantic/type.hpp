#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/str_pool.hpp>
#include <list>
#include <set>

namespace ulam::ast {
class TypeDef;
class TypeName;
class TypeExpr;
} // namespace ulam::ast

namespace ulam {

class Builtins;
class RValue;
class TypedValue;
class Value;

using type_id_t = std::uint16_t;
constexpr type_id_t NoTypeId = 0;

class TypeIdGen {
public:
    TypeIdGen(): _next{1} {}

    TypeIdGen(TypeIdGen&&) = default;
    TypeIdGen& operator=(TypeIdGen&&) = default;

    type_id_t next() { return _next++; }

private:
    type_id_t _next;
};

class PrimType;
class Class;
class AliasType;
class ArrayType;
class RefType;

class Type {
    friend AliasType;

public:
    explicit Type(Builtins& builtins, TypeIdGen* id_gen):
        _builtins{builtins},
        _id_gen{id_gen},
        _id{id_gen ? id_gen->next() : NoTypeId} {}
    virtual ~Type();

    bool operator==(const Type& other) const { return this == &other; }
    bool operator!=(const Type& other) const { return !operator==(other); }

    bool has_id() { return _id != NoTypeId; }
    type_id_t id() const { return _id; }

    virtual std::string name() const { assert(false); }

    virtual bitsize_t bitsize() const = 0;

    virtual bool is_constructible() const { return false; }
    virtual RValue construct();

    RValue load(const Bits& data, bitsize_t off);
    void store(Bits& data, bitsize_t off, const RValue& rval);

    virtual RValue load(const BitsView data, bitsize_t off);
    virtual void store(BitsView data, bitsize_t off, const RValue& rval);

    // canonical non-reference type, type of value
    bool is_actual() const;
    Ref<Type> actual();
    Ref<const Type> actual() const;

    bool is_canon() const;
    virtual Ref<Type> canon() { return this; }
    virtual Ref<const Type> canon() const { return this; }

    bool is(BuiltinTypeId id) const;
    bool is_builtin() const { return bi_type_id() != NoBuiltinTypeId; }
    virtual BuiltinTypeId bi_type_id() const { return NoBuiltinTypeId; }

    bool is_object() const;
    bool is_atom() const;

    bool is_prim() const { return _as_prim(); }
    bool is_class() const { return _as_class(); }
    bool is_alias() const { return _as_alias(); }
    bool is_array() const { return _as_array(); }
    bool is_ref() const { return _as_ref(); }

    Ref<PrimType> as_prim();
    Ref<const PrimType> as_prim() const;

    Ref<Class> as_class();
    Ref<const Class> as_class() const;

    Ref<AliasType> as_alias();
    Ref<const AliasType> as_alias() const;

    Ref<ArrayType> as_array();
    Ref<const ArrayType> as_array() const;

    Ref<RefType> as_ref();
    Ref<const RefType> as_ref() const;

    virtual Ref<Type> non_alias() { return this; }
    virtual Ref<const Type> non_alias() const { return this; }

    virtual TypedValue type_op(TypeOp op);
    virtual TypedValue type_op(TypeOp op, Value& val);

    bool is_same(Ref<const Type> type) const;
    bool is_same_actual(Ref<const Type> type) const;

    virtual Ref<Type> common(Ref<Type> type);
    virtual Ref<Type>
    common(const Value& val1, Ref<Type> type, const Value& val2);

    bool is_expl_castable_to(Ref<const Type> type, const Value& val) const;
    bool is_impl_castable_to(Ref<const Type> type, const Value& val) const;
    virtual bool is_castable_to(
        Ref<const Type> type, const Value& val, bool expl = true) const;

    bool is_expl_castable_to(BuiltinTypeId bi_type_id, const Value& val) const;
    bool is_impl_castable_to(BuiltinTypeId bi_type_id, const Value& val) const;
    virtual bool is_castable_to(
        BuiltinTypeId bi_type_id, const Value& val, bool expl = true) const;

    bool is_expl_castable_to(Ref<const Type> type) const;
    bool is_impl_castable_to(Ref<const Type> type) const;
    virtual bool is_castable_to(Ref<const Type> type, bool expl = true) const;

    bool is_expl_castable_to(BuiltinTypeId bi_type_id) const;
    bool is_impl_castable_to(BuiltinTypeId bi_type_id) const;
    virtual bool
    is_castable_to(BuiltinTypeId bi_type_id, bool expl = true) const;

    bool is_expl_refable_as(Ref<const Type> type, const Value& val) const;
    bool is_impl_refable_as(Ref<const Type> type, const Value& val) const;
    virtual bool is_refable_as(
        Ref<const Type> type, const Value& val, bool expl = true) const;

    virtual conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const;
    virtual conv_cost_t conv_cost(
        Ref<const Type> type, const Value& val, bool allow_cast = false) const;

    virtual Value cast_to(Ref<Type> type, Value&& val);

    virtual Ref<Type> deref() { return this; }
    virtual Ref<const Type> deref() const { return this; }

    virtual Ref<ArrayType> array_type(array_size_t size);
    virtual Ref<RefType> ref_type();

    Builtins& builtins() { return _builtins; } // hack for Data

protected:
    virtual Ref<PrimType> _as_prim() { return {}; }
    virtual Ref<const PrimType> _as_prim() const { return {}; }

    virtual Ref<Class> _as_class() { return {}; }
    virtual Ref<const Class> _as_class() const { return {}; }

    virtual Ref<AliasType> _as_alias() { return {}; }
    virtual Ref<const AliasType> _as_alias() const { return {}; }

    virtual Ref<ArrayType> _as_array() { return {}; }
    virtual Ref<const ArrayType> _as_array() const { return {}; }

    virtual Ref<RefType> _as_ref() { return {}; }
    virtual Ref<const RefType> _as_ref() const { return {}; }

    virtual Ptr<ArrayType> make_array_type(array_size_t size);
    virtual Ptr<RefType> make_ref_type();

    TypeIdGen* id_gen() { return _id_gen; }

private:
    Builtins& _builtins;
    TypeIdGen* _id_gen;
    type_id_t _id;
    std::unordered_map<array_size_t, Ptr<ArrayType>> _array_types;
    Ptr<RefType> _ref_type;
};

using TypeList = std::list<Ref<Type>>;
using TypeIdSet = std::set<type_id_t>;
using TypeSet = std::set<Ref<Type>>;

class UserType : public Type, public Decl {
public:
    UserType(Builtins& builtins, TypeIdGen* id_gen):
        Type{builtins, id_gen}, Decl{} {}

    virtual str_id_t name_id() const = 0;
};

class AliasType : public UserType {
public:
    using UserType::is_castable_to;

    AliasType(
        UniqStrPool& str_pool,
        Builtins& builtins,
        TypeIdGen* id_gen,
        Ref<ast::TypeDef> node):
        UserType{builtins, id_gen}, _str_pool{str_pool}, _node{node} {}

    std::string name() const override;

    str_id_t name_id() const override;

    bitsize_t bitsize() const override;

    bool is_constructible() const override;
    RValue construct() override;

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    Ref<Type> canon() override { return _canon; }
    Ref<const Type> canon() const override { return _canon; }

    BuiltinTypeId bi_type_id() const override;

    bool is_castable_to(
        Ref<const Type> type,
        const Value& val,
        bool expl = true) const override;

    bool is_castable_to(
        BuiltinTypeId bi_type_id,
        const Value& val,
        bool expl = true) const override;

    bool is_refable_as(Ref<const Type> type, const Value& val, bool expl = true)
        const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    conv_cost_t conv_cost(
        Ref<const Type> type,
        const Value& val,
        bool allow_cast = false) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;

    Ref<Type> non_alias() override { return _non_alias; }
    Ref<const Type> non_alias() const override { return _non_alias; }

    Ref<Type> aliased() { return _aliased; }
    Ref<const Type> aliased() const { return _aliased; }

    void set_aliased(Ref<Type> type);

    Ref<Type> deref() override;
    Ref<const Type> deref() const override;

    Ref<ast::TypeDef> node() { return _node; }
    Ref<ast::TypeName> type_name();
    Ref<ast::TypeExpr> type_expr();

protected:
    Ref<AliasType> _as_alias() override { return this; }
    Ref<const AliasType> _as_alias() const override { return this; }

    Ref<PrimType> _as_prim() override;
    Ref<const PrimType> _as_prim() const override;

    Ref<Class> _as_class() override;
    Ref<const Class> _as_class() const override;

    Ref<ArrayType> _as_array() override;
    Ref<const ArrayType> _as_array() const override;

    Ref<RefType> _as_ref() override;
    Ref<const RefType> _as_ref() const override;

    Ptr<ArrayType> make_array_type(array_size_t size) override;
    Ptr<RefType> make_ref_type() override;

private:
    UniqStrPool& _str_pool;
    Ref<ast::TypeDef> _node;
    Ref<Type> _aliased{};
    Ref<Type> _canon{};
    Ref<Type> _non_alias{};
};

class ArrayType : public Type {
    friend AliasType;

public:
    using Type::is_castable_to;

    ArrayType(
        Builtins& builtins,
        TypeIdGen* id_gen,
        Ref<Type> item_type,
        array_size_t size);

    std::string name() const override;

    bitsize_t bitsize() const override;

    bool is_constructible() const override { return true; }
    RValue construct() override;

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    TypedValue type_op(TypeOp op) override;

    Ref<Type> canon() override { return _canon; }
    Ref<const Type> canon() const override { return _canon; }

    bool is_castable_to(Ref<const Type> type, const Value& val, bool expl = true) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;

    Ref<Type> item_type() { return _item_type; }
    Ref<const Type> item_type() const { return _item_type; }

    array_size_t array_size() const { return _array_size; }

    bitsize_t item_off(array_idx_t idx) const;

protected:
    Ref<ArrayType> _as_array() override { return this; }
    Ref<const ArrayType> _as_array() const override { return this; }

private:
    void set_canon(Ref<ArrayType> canon);

    Ref<Type> _item_type;
    array_size_t _array_size;
    Ref<ArrayType> _canon{};
};

class RefType : public Type {
    friend AliasType;

public:
    using Type::is_castable_to;

    RefType(Builtins& builtins, TypeIdGen* id_gen, Ref<Type> refd):
        Type{builtins, id_gen},
        _refd{refd},
        _canon{refd->is_canon() ? this : refd->canon()->ref_type()} {}

    std::string name() const override;

    bitsize_t bitsize() const override;

    Ref<Type> canon() override { return _canon; }
    Ref<const Type> canon() const override { return _canon; }

    Ref<Type> refd() { return _refd; }
    Ref<const Type> refd() const { return _refd; }

    Ref<Type> deref() override { return refd(); }
    Ref<const Type> deref() const override { return refd(); }

    Ref<RefType> ref_type() override { return this; }

    bool is_castable_to(
        Ref<const Type> type,
        const Value& val,
        bool expl = true) const override;

    bool is_castable_to(
        BuiltinTypeId bi_type_id,
        const Value& val,
        bool expl = true) const override;

    bool
    is_castable_to(BuiltinTypeId bi_type_id, bool expl = true) const override;

    bool is_refable_as(Ref<const Type> type, const Value& val, bool expl = true)
        const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    conv_cost_t conv_cost(
        Ref<const Type> type,
        const Value& val,
        bool allow_cast = false) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;

    Ptr<ArrayType> make_array_type(array_size_t size) override;
    Ptr<RefType> make_ref_type() override;

protected:
    Ref<RefType> _as_ref() override { return this; }
    Ref<const RefType> _as_ref() const override { return this; }

private:
    void set_canon(Ref<RefType> canon);

    Ref<Type> _refd{};
    Ref<RefType> _canon{};
};

} // namespace ulam
