#include "./eval.hpp"
#include "./eval/visitor.hpp"

void Eval::do_eval(ulam::Ref<ulam::ast::Block> block) {
    auto visitor = ulam::make<EvalVisitor>(_ast->program());
    visitor->eval(block);
    _data = visitor->data();
}
