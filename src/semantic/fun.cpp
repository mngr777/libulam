#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/fun.hpp>

namespace ulam {

ast::Ref<ast::TypeName> FunOverload::ret_type_node() {
    return _node->ret_type_name();
}

ast::Ref<ast::ParamList> FunOverload::params_node() { return _node->params(); }

ast::Ref<ast::FunDefBody> FunOverload::body_node() { return _node->body(); }

} // namespace ulam
