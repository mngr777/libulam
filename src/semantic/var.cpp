#include <cassert>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/semantic/var.hpp>
#include <utility>

namespace ulam {

str_id_t Var::name_id() const { return _node->name().str_id(); }

bitsize_t Var::bitsize() const {
    assert(_type);
    return _type->bitsize();
}

Ref<Type> Var::type() {
    assert(_type);
    return _type;
}

Ref<const Type> Var::type() const {
    assert(_type);
    return _type;
}

void Var::set_type(Ref<Type> type) {
    assert(type);
    assert(!_type);
    _type = type;
}

void Var::set_value(Value&& value) { std::swap(_value, value); }

} // namespace ulam
