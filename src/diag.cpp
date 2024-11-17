#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libulam/diag.hpp>

namespace ulam {
namespace {

constexpr unsigned MaxErrorNum = 10;

constexpr char FatalPrefix[] = "Fatal error ";
constexpr char ErrorPrefix[] = "Error ";
constexpr char WarnPrefix[] = "Warning ";
constexpr char NoticePrefix[] = "Notice ";

const char* level_prefix(diag::Level lvl) {
    switch (lvl) {
    case diag::Fatal:
        return FatalPrefix;
    case diag::Error:
        return ErrorPrefix;
    case diag::Warn:
        return WarnPrefix;
    case diag::Notice:
        return NoticePrefix;
    default:
        assert(false);
    }
}

} // namespace

void Diag::emit(
    diag::Level lvl,
    loc_id_t loc_id,
    std::size_t len,
    const std::string& text) {
    emit(lvl, loc_id, 0, len, text);
}

// TMP: output to stderr
void Diag::emit(
    diag::Level lvl,
    loc_id_t loc_id,
    int off,
    std::size_t len,
    const std::string& text) {
    const auto& loc = _sm.loc(loc_id);
    auto src = _sm.src(loc.src_id());
    std::cerr << level_prefix(lvl) << "in " << src->name() << ":" << loc.linum()
              << ":" << loc.chr() << "\n";
    std::cerr << _sm.line_at(loc);
    std::cerr << std::string(loc.chr() - 1, ' ') << "^\n";
    std::cerr << std::string(loc.chr() - 1, ' ') << text << "\n";
    if (lvl < diag::Warn)
        ++_err_num;
    if (lvl == diag::Fatal || _err_num == MaxErrorNum)
        std::exit(-1);
}

} // namespace ulam
