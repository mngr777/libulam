#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/eval.hpp>

class Eval : public ulam::sema::Eval {
public:
    using ulam::sema::Eval::Eval;

protected:
    ulam::Ptr<ulam::sema::EvalVisitor> visitor() override;
};
