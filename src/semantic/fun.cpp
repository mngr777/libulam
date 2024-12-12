#include "libulam/semantic/scope/state.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/fun.hpp>

namespace ulam {

// FunOverload

Ref<ast::TypeName> FunOverload::ret_type_name() {
    assert(_node->has_ret_type_name());
    return _node->ret_type_name();
}

void FunOverload::add_param_type(Ref<Type> type) {
    assert(params_node());
    assert(_param_types.size() < params_node()->child_num());
    _param_types.push_back(type);
}

Ref<ast::ParamList> FunOverload::params_node() { return _node->params(); }

Ref<ast::FunDefBody> FunOverload::body_node() { return _node->body(); }

// Fun

void Fun::merge(Ref<Fun> other) {
    for (auto& item : other->_overloads)
        add_overload(item.ref());
}

Ref<FunOverload> Fun::add_overload(Ref<ast::FunDef> node, PersScopeState scope_state) {
    assert(node);
    assert(scope_state);
    auto overload = ulam::make<FunOverload>(node);
    overload->set_pers_scope_state(scope_state);
    auto overload_ref = ref(overload);
    _overloads.push_back(std::move(overload));
    return overload_ref;
}

Ref<FunOverload> Fun::add_overload(Ref<FunOverload> overload) {
    _overloads.push_back(overload);
    return overload;
}

} // namespace ulam
