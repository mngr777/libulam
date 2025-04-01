#include "./eval.hpp"
#include "./eval/visitor.hpp"

ulam::Ptr<ulam::sema::EvalVisitor> Eval::visitor() {
    return ulam::make<EvalVisitor>(_ast->program());
}
