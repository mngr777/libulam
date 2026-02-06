#pragma once
#include <libulam/types.hpp>
#include <libulam/parser/options.hpp>
#include <libulam/semantic/scope/options.hpp>
#include <libulam/semantic/type/class/options.hpp>
#include <libulam/sema/eval/options.hpp>

namespace ulam {

struct Options {
    PathList include_paths;
    ParserOptions parser_options{DefaultParserOptions};
    ClassOptions class_options{DefaultClassOptions};
    ScopeOptions scope_options{DefaultScopeOptions};
    sema::EvalOptions eval_options{sema::DefaultEvalOptions};
};

const Options DefaultOptions{};

} // namespace ulam
