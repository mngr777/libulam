#pragma once
#include <exception>
#include <libulam/semantic/expr_res.hpp>

namespace ulam {

class EvalExcept : std::exception {};

class EvalExceptReturn : public EvalExcept {
public:
    EvalExceptReturn(ExprRes&& res): _res{std::move(res)} {}

    const ExprRes& res() const { return _res; }
    ExprRes move_res();

private:
    ExprRes _res;
};

class EvalExceptError : public EvalExcept {
public:
    enum Code { Error };

    EvalExceptError(Code code): _code{code} {}

    Code code() const { return _code; }

private:
    Code _code;
};

} // namespace ulam
