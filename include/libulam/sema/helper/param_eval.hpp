#pragma once
#include <cstdint>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <utility>

namespace ulam::sema {

class ParamEval : public Helper {
public:
    using Flag = std::uint8_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag ReqValues = 0;

    explicit ParamEval(
        ast::Ref<ast::Root> ast, Flag flags = NoFlags):
        Helper{ast}, _flags{flags} {}

    std::pair<TypedValueList, bool> eval(ast::Ref<ast::ArgList> args, Ref<Scope> scope);

private:
    Flag _flags;
};

} // namespace ulam::sema
