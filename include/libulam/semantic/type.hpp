#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/semantic/value/bit_vector.hpp>
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

class Diag;
class Scope;
class RValue;
class TypedValue;

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
public:
    explicit Type(TypeIdGen* id_gen):
        _id_gen{id_gen}, _id{id_gen ? id_gen->next() : NoTypeId} {}
    virtual ~Type();

    bool operator==(const Type& other) const { return this == &other; }
    bool operator!=(const Type& other) const { return !operator==(other); }

    bool has_id() { return _id != NoTypeId; }
    type_id_t id() const { return _id; }

    virtual std::string name() const { assert(false); }

    virtual bitsize_t bitsize() const = 0;

    virtual RValue construct();

    RValue load(const BitVector& data, BitVector::size_t off) const;
    void
    store(BitVector& data, BitVector::size_t off, const RValue& rval) const;

    virtual RValue load(const BitVectorView data, BitVector::size_t off) const;
    virtual void
    store(BitVectorView data, BitVector::size_t off, const RValue& rval) const;

    bool is_canon() const;
    virtual Ref<Type> canon() { return this; }
    virtual Ref<const Type> canon() const { return this; }

    bool is_builtin() const { return builtin_type_id() != NoBuiltinTypeId; }
    virtual BuiltinTypeId builtin_type_id() const { return NoBuiltinTypeId; }

    bool is_prim() const { return as_prim(); }
    bool is_class() const { return as_class(); }
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

    virtual Ref<Type> non_alias() { return this; }
    virtual Ref<const Type> non_alias() const { return this; }

    virtual TypedValue type_op(TypeOp op);

    bool is_same(Ref<const Type> type) const;

    bool is_expl_castable_to(Ref<const Type> type) const;
    bool is_impl_castable_to(Ref<const Type> type) const;
    virtual bool is_castable_to(Ref<const Type> type, bool expl = true) const;

    bool is_impl_castable_to(BuiltinTypeId builtin_type_id) const;
    bool is_expl_castable_to(BuiltinTypeId builtin_type_id) const;
    virtual bool
    is_castable_to(BuiltinTypeId builtin_type_id, bool expl = true) const;

    virtual conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const;

    virtual RValue cast_to(Ref<Type> type, RValue&& rval);

    virtual Ref<Type> deref() { return this; }
    virtual Ref<const Type> deref() const { return this; }

    virtual Ref<ArrayType> array_type(array_size_t size);
    virtual Ref<RefType> ref_type();

protected:
    virtual Ptr<ArrayType> make_array_type(array_size_t size);
    virtual Ptr<RefType> make_ref_type();

    TypeIdGen* id_gen() { return _id_gen; }

private:
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
    explicit UserType(TypeIdGen* id_gen): Type{id_gen} {}

    virtual str_id_t name_id() const = 0;
};

class AliasType : public UserType {
public:
    AliasType(TypeIdGen* id_gen, Ref<ast::TypeDef> node):
        UserType{id_gen}, _node(node) {}

    str_id_t name_id() const override;

    bitsize_t bitsize() const override;

    Ref<AliasType> as_alias() override { return this; }
    Ref<const AliasType> as_alias() const override { return this; }

    Ref<ast::TypeDef> node() { return _node; }
    Ref<ast::TypeName> type_name();
    Ref<ast::TypeExpr> type_expr();

    Ref<Type> canon() override { return _canon; }
    Ref<const Type> canon() const override { return _canon; }

    Ref<Type> non_alias() override { return _non_alias; }
    Ref<const Type> non_alias() const override { return _non_alias; }

    Ref<Type> aliased() { return _aliased; }
    Ref<const Type> aliased() const { return _aliased; }

    void set_aliased(Ref<Type> type);

protected:
    Ptr<ArrayType> make_array_type(array_size_t size) override;
    Ptr<RefType> make_ref_type() override;

private:
    Ref<ast::TypeDef> _node;
    Ref<Type> _aliased{};
    Ref<Type> _canon{};
    Ref<Type> _non_alias{};
};

class ArrayType : public Type {
    friend AliasType;

public:
    ArrayType(
        TypeIdGen* id_gen,
        Ref<Type> item_type,
        array_size_t array_size,
        Ref<ArrayType> canon = {});

    bitsize_t bitsize() const override;

    RValue construct() override;

    RValue load(const BitVectorView data, BitVector::size_t off) const override;
    void store(BitVectorView data, BitVector::size_t off, const RValue& rval)
        const override;

    Ref<ArrayType> as_array() override { return this; }
    Ref<const ArrayType> as_array() const override { return this; }

    Ref<Type> canon() override { return _canon; }
    Ref<const Type> canon() const override { return _canon; }

    Ref<Type> item_type() { return _item_type; }
    Ref<const Type> item_type() const { return _item_type; }

    array_size_t array_size() const { return _array_size; }

private:
    void set_canon(Ref<ArrayType> canon);

    Ref<Type> _item_type;
    array_size_t _array_size;
    Ref<ArrayType> _canon{};
};

class RefType : public Type {
    friend AliasType;

public:
    RefType(TypeIdGen* id_gen, Ref<Type> refd):
        Type{id_gen}, _refd{refd}, _canon{this} {}

    bitsize_t bitsize() const override;

    Ref<RefType> as_ref() override { return this; }
    Ref<const RefType> as_ref() const override { return this; }

    Ref<Type> canon() override { return _canon; }
    Ref<const Type> canon() const override { return _canon; }

    Ref<Type> refd() { return _refd; }
    Ref<const Type> refd() const { return _refd; }

    Ref<Type> deref() override { return refd(); }
    Ref<const Type> deref() const override { return refd(); }

    Ref<RefType> ref_type() override { return this; }

private:
    void set_canon(Ref<RefType> canon);

    Ref<Type> _refd{};
    Ref<RefType> _canon{};
};

} // namespace ulam
