#pragma once
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam::ast {

class TypeIdent : public Stmt, public Named {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_self, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_super, false)
    ULAM_AST_SIMPLE_ATTR(bool, is_local, false)
public:
    explicit TypeIdent(Str name): Named{name} { set_loc_id(name.loc_id()); }
};

class ArgList;

class TypeSpec : public Tuple<Stmt, TypeIdent, ArgList> {
    ULAM_AST_NODE
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

class TypeName : public Tuple<List<Stmt, TypeIdent>, TypeSpec> {
    ULAM_AST_NODE
public:
    explicit TypeName(Ptr<TypeSpec>&& spec): Tuple{std::move(spec)} {}

    unsigned child_num() const override {
        return Tuple::child_num() + List::child_num();
    }

    ULAM_AST_TUPLE_PROP(first, 0)

    Ref<TypeIdent> ident(unsigned n) {
        return (n == 0) ? first()->ident() : List::get(n - 1);
    }

    Ref<const TypeIdent> ident(unsigned n) const {
        return (n == 0) ? first()->ident() : List::get(n - 1);
    }

    Ptr<TypeIdent> replace_ident(unsigned n, Ptr<TypeIdent>&& repl) {
        return (n == 0) ? first()->replace_ident(std::move(repl))
                        : List::replace(n - 1, std::move(repl));
    }

    Ref<Node> child(unsigned n) override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }

    Ref<const Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
};

class TypeNameList : public List<Node, TypeName> {
    ULAM_AST_NODE
};

// TypeName[2][4]&
class FullTypeName : public Tuple<Stmt, TypeName, ExprList> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_ref, false)
public:
    FullTypeName(Ptr<TypeName>&& type_name, Ptr<ExprList>&& array_dims):
        Tuple{std::move(type_name), std::move(array_dims)} {}

    ULAM_AST_TUPLE_PROP(type_name, 0)
    ULAM_AST_TUPLE_PROP(array_dims, 1)

    bool is_array() { return has_array_dims(); }
};

} // namespace ulam::ast
