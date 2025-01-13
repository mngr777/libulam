#pragma once
#include <libulam/ast/nodes/root.hpp>
#include <libulam/context.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::sema {

class Eval {
public:
    Eval(Context& ctx, Ref<ast::Root> ast);

    void eval(const std::string& text);

private:
    Context& _ctx;
    Ref<ast::Root> _ast;
    EvalVisitor _eval;
};

} // namespace ulam::sema
