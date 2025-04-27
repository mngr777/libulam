#pragma once
#include <libulam/diag.hpp>
#include <libulam/lex.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/token.hpp>
#include <libulam/types.hpp>
#include <stack>

namespace ulam {

using Version = std::uint8_t;

class Context;

class Preproc {
    friend Lex;

public:
    using Path = std::filesystem::path;

    explicit Preproc(Context& ctx): _ctx{ctx}, _version{DefaultVersion} {}

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

    Lex& lexer();
    void lex(Token& token);

    template <typename... Ts>
    bool expect(Token& token, tok::Type type, Ts... stop);

    loc_id_t loc_id();
    void diag(loc_id_t, std::string) {}

    void preproc_ulam();
    void preproc_use();
    bool preproc_load();

    const std::string_view tok_str(const Token& token);

    Context& _ctx;
    Version _version;
    std::stack<std::pair<Src*, Lex>> _stack;
};

} // namespace ulam
