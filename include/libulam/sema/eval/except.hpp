#pragma once
#include <exception>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <string>

namespace ulam::ast {
class Return;
}

namespace ulam {

class EvalExcept : std::exception {};

class EvalExceptReturn : public EvalExcept {
public:
    explicit EvalExceptReturn(Ref<ast::Return> node, ExprRes&& res):
        _node{node}, _res{std::move(res)} {}

    Ref<ast::Return> node() { return _node; }

    const ExprRes& res() const { return _res; }
    ExprRes move_res();

private:
    Ref<ast::Return> _node;
    ExprRes _res;
};

class EvalExceptBreak : public EvalExcept {};
class EvalExceptContinue : public EvalExcept {};

class EvalExceptError : public EvalExcept {
public:
    enum Code { Error };

    explicit EvalExceptError(std::string message):
        _code{Error}, _message{std::move(message)} {}
    explicit EvalExceptError(Code code): _code{code} {}

    Code code() const { return _code; }
    const std::string& message() const { return _message; }

private:
    Code _code;
    std::string _message;
};

} // namespace ulam
