#include "libulam/src_mngr.hpp"
#include "libulam/src_loc.hpp"
#include <cassert>

namespace ulam {

Src* SrcMngr::string(std::string text) {
    auto ptr = std::make_unique<StrSrc>(_srcs.size(), std::move(text));
    _srcs.emplace_back(std::move(ptr));
    return _srcs.back().get();
}

Src* SrcMngr::file(std::filesystem::path path) {
    auto ptr = std::make_unique<FileSrc>(_srcs.size(), std::move(path));
    _srcs.push_back(std::move(ptr));
    return _srcs.back().get();
}

Src* SrcMngr::src(src_id_t src_id) {
    assert(src_id < _srcs.size());
    return _srcs[src_id].get();
}

loc_id_t SrcMngr::loc_id(src_id_t src_id, const char* ptr, linum_t linum, chr_t chr) {
    loc_id_t id = _locs.size();
    _locs.emplace_back(src_id, ptr, linum, chr);
    return id;
}

SrcLoc SrcMngr::loc(loc_id_t loc_id) {
    assert(loc_id != NoLocId);
    assert(loc_id < _locs.size());
    return _locs[loc_id];
}

std::string_view
SrcMngr::str_at(const SrcLoc& loc, std::size_t size, int off) {
    // TODO: remove ptr from loc
    return {loc.ptr() + off, size};
}

std::string_view SrcMngr::str_at(loc_id_t loc_id, std::size_t size, int off) {
    return str_at(loc(loc_id), size, off);
}

std::string_view SrcMngr::line_at(const SrcLoc& loc) {
    auto ref = src(loc.src_id())->line(loc.linum());
    return {ref.start(), ref.size()};
}

std::string_view SrcMngr::line_at(loc_id_t loc_id) {
    return line_at(loc(loc_id));
}

} // namespace ulam
