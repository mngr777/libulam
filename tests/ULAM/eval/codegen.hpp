#pragma once
#include "./codegen/context_stack.hpp"
#include "./stringifier.hpp"
#include <libulam/ast/nodes/exprs.hpp>
#include <libulam/ast/nodes/stmt.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/program.hpp>
#include <string>

class Codegen {
public:
    explicit Codegen(ulam::Ref<ulam::Program> program): _program{program} {}

    Stringifier make_strf();

    void block_open(bool nospace = false);
    void block_close(bool nospace = false);

    void append(std::string data, bool nospace = false);

    void add_as_cond(
        ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type);
    std::string as_cond_str(
        ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type);
    std::string as_cond_prefix_str(
        ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type);

    bool has_next_prefix() const;
    void set_next_prefix(std::string prefix);
    std::string move_next_prefix();

    gen::ContextStack& ctx_stack() { return _ctx_stack; }

    unsigned next_tmp_idx() { return ++_tmp_idx; }
    std::string next_tmp_idx_str() { return std::to_string(next_tmp_idx()); }

    const std::string& code() const { return _code; }

private:
    ulam::Ref<ulam::Program> _program;
    gen::ContextStack _ctx_stack;
    std::string _code;
    std::string _next_prefix;
    unsigned _tmp_idx{0};
};
