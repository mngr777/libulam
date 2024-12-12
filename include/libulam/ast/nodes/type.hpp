#pragma once
#include <libulam/ast/nodes/stmt.hpp>
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
    ULAM_AST_REF_ATTR(ClassTpl, cls_tpl)
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

    const Ref<Node> child(unsigned n) const override {
        return (n == 0) ? Tuple::child(0) : List::child(n - 1);
    }
};

class TypeNameList : public List<Node, TypeName> {
    ULAM_AST_NODE
};

} // namespace ulam::ast
