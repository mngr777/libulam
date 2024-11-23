#pragma once
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {
class Type;
class TypeTpl;
} // namespace ulam

namespace ulam::ast {

class TypeIdent : public Stmt, public Named {
    ULAM_AST_NODE
public:
    explicit TypeIdent(Str name): Named{name} {}
};

class ArgList;

class TypeSpec : public Tuple<Stmt, TypeIdent, ArgList> {
    ULAM_AST_NODE
    ULAM_AST_REF_ATTR(TypeTpl, type_tpl)
    ULAM_AST_REF_ATTR(Type, type)
public:
    TypeSpec(Ptr<TypeIdent>&& ident, Ptr<ArgList>&& args):
        Tuple{std::move(ident), std::move(args)},
        _builtin_type_id{NoBuiltinTypeId} {}
    TypeSpec(BuiltinTypeId builtin_type_id, Ptr<ArgList>&& args):
        Tuple{{}, std::move(args)}, _builtin_type_id(builtin_type_id) {}

    bool is_builtin() const { return _builtin_type_id != NoBuiltinTypeId; }
    BuiltinTypeId builtin_type_id() const { return _builtin_type_id; }

    ULAM_AST_TUPLE_PROP(ident, 0);
    ULAM_AST_TUPLE_PROP(args, 1);

private:
    BuiltinTypeId _builtin_type_id;
};

class TypeName : public Tuple<ListOf<Stmt, TypeIdent>, TypeSpec> {
    ULAM_AST_NODE
public:
    explicit TypeName(Ptr<TypeSpec>&& spec): Tuple{std::move(spec)} {}

    unsigned child_num() const override {
        return Tuple::child_num() + ListOf::child_num();
    }

    ULAM_AST_TUPLE_PROP(first, 0)

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : ListOf::child(n - 1);
    }

    const Ref<Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : ListOf::child(n - 1);
    }
};

} // namespace ulam::ast
