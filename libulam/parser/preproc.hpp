#pragma once
#include <libulam/diag.hpp>
#include <libulam/parser/lex.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/parser/token.hpp>
#include <libulam/types.hpp>
#include <libulam/utils/file.hpp>
#include <stack>

namespace ulam {

using Version = std::uint8_t;

class Context;

class Preproc {
    friend Lex;

public:
    explicit Preproc(Context& ctx);

    Preproc(const Preproc&) = delete;
    Preproc& operator=(Preproc) = delete;

    void main_file(Path path);
    void main_string(std::string text, Path path);

    void add_string(std::string text, Path path);

    Preproc& operator>>(Token& token);

    const Path& current_path() const;

    Version version() const { return _version; }

private:
    void push(Src* src);
    void pop();

    Src& src();
    Lex& lexer();

    void lex(Token& token);

    template <typename... Ts>
    bool expect(Token& token, tok::Type type, Ts... stop);

    loc_id_t loc_id();

    void preproc_ulam();
    void preproc_use();
    bool preproc_load();
    void preproc_meta(Token& token);

    const std::string_view tok_str(const Token& token);

    Context& _ctx;
    utils::PathResolver _path_resolver;
    Version _version;
    std::stack<std::pair<Src*, Lex>> _stack;
};

} // namespace ulam
