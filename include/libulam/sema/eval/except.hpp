#pragma once
#include <exception>
#include <libulam/semantic/expr_res.hpp>

namespace ulam {

class EvalExcept : std::exception {
public:
    enum Code { Error, Return };

    EvalExcept(Code code): _code{code} {}
    EvalExcept(Code code, ExprRes&& res): _code{code}, _res{std::move(res)} {}

    Code code() const { return _code; }

    const ExprRes& res() const { return _res; }
    ExprRes move_res();

private:
    Code _code;
    ExprRes _res;
};

} // namespace ulam
