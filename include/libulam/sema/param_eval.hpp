#pragma once
#include <cstdint>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <utility>

namespace ulam {
class Program;
}

namespace ulam::sema {

class ParamEval {
public:
    using Flag = std::uint8_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag ReqValues = 0;

    explicit ParamEval(Ref<Program> program, Flag flags = NoFlags):
        _program{program}, _flags{flags} {}

    std::pair<TypedValueList, bool>
    eval(Ref<ast::ArgList> args, Ref<Scope> scope);

private:
    Ref<Program> _program;
    Flag _flags;
};

} // namespace ulam::sema
