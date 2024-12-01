#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/fun.hpp>

namespace ulam {

// FunOverload

ast::Ref<ast::TypeName> FunOverload::ret_type_node() {
    return _node->ret_type_name();
}

ast::Ref<ast::ParamList> FunOverload::params_node() { return _node->params(); }

ast::Ref<ast::FunDefBody> FunOverload::body_node() { return _node->body(); }

// Fun
Ref<FunOverload> Fun::add_overload(ast::Ref<ast::FunDef> node) {
    auto overload = ulam::make<FunOverload>(node);
    auto overload_ref = ref(overload);
    _overloads.push_back(std::move(overload));
    return overload_ref;
}


} // namespace ulam
