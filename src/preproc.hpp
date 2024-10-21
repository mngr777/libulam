#pragma once
#include "libulam/src_loc.hpp"
#include "libulam/token.hpp"
#include "src/lex.hpp"
#include "src/src_mngr.hpp"
#include <stack>

namespace ulam {

using Version = std::uint8_t;

class Preproc {
    friend Lex;
public:
    Preproc(SrcMngr& sm): _sm{sm} {}

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
    bool expect(Token& token, tok::Type type, Ts... stop) {
        lex(token);
        if (token.is(type))
            return true;
        while (true) {
            lex(token);
            if (token.in(stop...))
                break;
            diag(token.loc_id, "Unexpected token");
        }
        return false;
    }

    loc_id_t loc_id();
    void diag(loc_id_t, std::string) {}

    void preproc_ulam();
    void preproc_use();
    bool preproc_load();

    SrcMngr& _sm;
    std::stack<std::pair<Src*, Lex>> _stack;
};

} // namespace ulam
