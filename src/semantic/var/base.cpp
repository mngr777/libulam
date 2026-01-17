#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

bool VarBase::has_name() const { return has_node(); }

str_id_t VarBase::name_id() const {
    ulam_assert(has_name());
    return _node->name().str_id();
}

bitsize_t VarBase::bitsize() const {
    ulam_assert(_type);
    return _type->bitsize();
}

bool VarBase::has_type_node() const { return _type_node; }

Ref<ast::TypeName> VarBase::type_node() {
    ulam_assert(_type_node);
    return _type_node;
}

bool VarBase::has_node() const { return _node; }

Ref<ast::VarDecl> VarBase::node() {
    ulam_assert(_node);
    return _node;
}

Ref<const ast::VarDecl> VarBase::node() const {
    ulam_assert(_node);
    return _node;
}

bool VarBase::has_type() const { return _type; }

Ref<Type> VarBase::type() const {
    ulam_assert(_type);
    return _type;
}

void VarBase::set_type(Ref<Type> type) {
    ulam_assert(type);
    ulam_assert(!_type);
    _type = type;
}

bool VarBase::is_parameter() const {
    return has_node() && node()->is_parameter();
}

} // namespace ulam
