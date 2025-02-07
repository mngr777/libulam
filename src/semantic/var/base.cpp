#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

str_id_t VarBase::name_id() const { return _node->name().str_id(); }

bitsize_t VarBase::bitsize() const {
    assert(_type);
    return _type->bitsize();
}

Ref<Type> VarBase::type() {
    assert(_type);
    return _type;
}

Ref<const Type> VarBase::type() const {
    assert(_type);
    return _type;
}

void VarBase::set_type(Ref<Type> type) {
    assert(type);
    assert(!_type);
    _type = type;
}

} // namespace ulam
