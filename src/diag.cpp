#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libulam/ast/node.hpp>
#include <libulam/diag.hpp>

namespace ulam {
namespace {

constexpr unsigned MaxErrorNum = 10;

constexpr char FatalPrefix[] = "Fatal error ";
constexpr char ErrorPrefix[] = "Error ";
constexpr char WarnPrefix[] = "Warning ";
constexpr char NoticePrefix[] = "Notice ";
constexpr char DebugPrefix[] = "Debug ";

const char* level_prefix(Diag::Level lvl) {
    switch (lvl) {
    case Diag::Fatal:
        return FatalPrefix;
    case Diag::Error:
        return ErrorPrefix;
    case Diag::Warn:
        return WarnPrefix;
    case Diag::Notice:
        return NoticePrefix;
    case Diag::Debug:
        return DebugPrefix;
    default:
        assert(false);
    }
}

} // namespace

void Diag::emit(
    Diag::Level lvl, Ref<const ast::Node> node, const std::string& text) {
    emit(lvl, node->loc_id(), 1, text);
}

void Diag::emit(
    Diag::Level lvl,
    loc_id_t loc_id,
    std::size_t len,
    const std::string& text) {
    emit(lvl, loc_id, 0, len, text);
}

// TMP: output to stderr
void Diag::emit(
    Diag::Level lvl,
    loc_id_t loc_id,
    int off,
    std::size_t len,
    const std::string& text) {
    const auto& loc = _sm.loc(loc_id);
    auto src = _sm.src(loc.src_id());
    std::cerr << level_prefix(lvl) << "in " << src->name() << ":" << loc.linum()
              << ":" << loc.chr() << "\n";
    auto line = _sm.line_at(loc);
    std::cerr << line;
    if (line.size() > 0 && line[line.size() - 1] != '\n')
        std::cerr << "\n";
    std::cerr << std::string(loc.chr() - 1, ' ') << "^\n";
    std::cerr << std::string(loc.chr() - 1, ' ') << text << "\n";
    if (lvl < Diag::Warn)
        ++_err_num;
    if (lvl == Diag::Fatal || _err_num == MaxErrorNum)
        std::exit(-1);
}

} // namespace ulam
