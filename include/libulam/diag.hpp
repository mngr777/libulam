#pragma once
#include <cstdint>
#include <libulam/src_mngr.hpp>

namespace ulam {

namespace diag {
enum Level : std::uint8_t { Fatal = 0, Error, Warn, Notice, Debug };
}

class Diag {
public:
    Diag(SrcMngr& sm): _sm{sm} {}

    // TMP, for when we don't have location yet
    void emit(
        diag::Level lvl,
        const std::string& text);

    void emit(
        diag::Level lvl,
        loc_id_t loc_id,
        std::size_t len,
        const std::string& text);

    void emit(
        diag::Level lvl,
        loc_id_t loc_id,
        int off,
        std::size_t len,
        const std::string& text);

private:
    SrcMngr& _sm;
    unsigned _err_num{0};
};

} // namespace ulam
