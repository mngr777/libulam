#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/options.hpp>

namespace ulam {

class Program;

class ProgramScope : public BasicScope {
public:
    explicit ProgramScope(
        Ref<Program> program, scope_flags_t flags = scp::NoFlags):
        BasicScope{nullptr, (scope_flags_t)(flags | scp::Program)},
        _program{program} {}

    Ref<Program> program() const override { return _program; }

private:
    Ref<Program> _program;
};

} // namespace ulam
