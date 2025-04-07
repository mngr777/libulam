#include "./eval.hpp"
#include "./eval/visitor.hpp"

ulam::sema::ExprRes Eval::do_eval(ulam::Ref<ulam::ast::Block> block) {
    auto visitor = ulam::make<EvalVisitor>(_ast->program());
    auto res = visitor->eval(block);
    _data = visitor->data();
    return res;
}
