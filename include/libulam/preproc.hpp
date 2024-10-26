#pragma once
#include "libulam/lex.hpp"
#include "libulam/src_loc.hpp"
#include "libulam/src_mngr.hpp"
#include "libulam/token.hpp"
#include <stack>

namespace ulam {

using Version = std::uint8_t;

class Preproc {
    friend Lex;

public:
    explicit Preproc(SrcMngr& sm): _sm{sm} {}

    Preproc(const Preproc&) = delete;
    Preproc& operator=(Preproc) = delete;

    void main_file(std::filesystem::path path);
    void main_str(std::string text);

    Preproc& operator>>(Token& token);

private:
    struct {
        Version version;
    } _state;

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

    SrcMngr& _sm;
    std::stack<std::pair<Src*, Lex>> _stack;
};

} // namespace ulam
