#pragma once
#include <cstdint>
#include <libulam/src_mngr.hpp>

namespace ulam {

class Diag {
public:
    enum Level : std::uint8_t { Fatal = 0, Error, Warn, Notice, Debug };

    Diag(SrcMngr& sm): _sm{sm} {}

    void emit(
        Diag::Level lvl,
        loc_id_t loc_id,
        std::size_t len,
        const std::string& text);

    void emit(
        Diag::Level lvl,
        loc_id_t loc_id,
        int off,
        std::size_t len,
        const std::string& text);

private:
    SrcMngr& _sm;
    unsigned _err_num{0};
};

} // namespace ulam
