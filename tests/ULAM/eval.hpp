#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <limits>

class Eval : public ulam::sema::Eval {
public:
    using ulam::sema::Eval::Eval;

    const std::string& code() const { return _code; }
    int status() const { return _status; }

protected:
    ulam::sema::ExprRes do_eval(ulam::Ref<ulam::ast::Block> block) override;

private:
    std::string _code;
    int _status{std::numeric_limits<int>::min()};
};
