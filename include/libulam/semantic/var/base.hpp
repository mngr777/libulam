#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {
class TypeName;
class VarDecl;
} // namespace ulam::ast

namespace ulam {

class VarBase : public Decl {
public:
    using Flag = std::uint8_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag Const = 1;
    static constexpr Flag ClassParam = 1 << 1;
    static constexpr Flag FunParam = 1 << 2;
    static constexpr Flag TmpFunParam = 1 << 3;
    static constexpr Flag Tpl = 1 << 4;

    VarBase(
        Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        Ref<Type> type,
        Flag flags = NoFlags):
        _type_node{type_node}, _node{node}, _type{type}, _flags{flags} {}

    bool has_name() const;
    str_id_t name_id() const;

    bitsize_t bitsize() const;

    bool has_type_node() const;
    Ref<ast::TypeName> type_node();

    bool has_node() const;
    Ref<ast::VarDecl> node();
    Ref<const ast::VarDecl> node() const;

    bool has_type() const;
    Ref<Type> type() const;
    void set_type(Ref<Type> type);

    bool is_const() const { return is(Const); }
    bool is_parameter() const;

    bool is(Flag flags) const { return (_flags & flags) == flags; }
    Flag flags() { return _flags; }
    void set_flag(Flag flag) { _flags |= flag; };
    void unset_flag(Flag flag) { _flags &= ~flag; };

private:
    Ref<ast::TypeName> _type_node;
    Ref<ast::VarDecl> _node;
    Ref<Type> _type{};
    Flag _flags;
};

} // namespace ulam
