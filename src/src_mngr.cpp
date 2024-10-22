#include "src/src_mngr.hpp"
#include <cassert>
#include <memory>

namespace ulam {

Src* SrcMngr::str(std::string text) {
    auto ptr = std::make_unique<StrSrc>(_srcs.size(), std::move(text));
    _srcs.emplace_back(std::move(ptr));
    return _srcs.back().get();
}

Src* SrcMngr::file(std::filesystem::path path) {
    auto ptr = std::make_unique<FileSrc>(_srcs.size(), std::move(path));
    _srcs.push_back(std::move(ptr));
    return _srcs.back().get();
}

loc_id_t SrcMngr::loc_id(src_id_t src_id, const char* ptr, linum_t linum, chr_t chr) {
    loc_id_t id = _locs.size();
    _locs.emplace_back(src_id, ptr, linum, chr);
    return id;
}

SrcLoc SrcMngr::loc(loc_id_t loc_id) {
    assert(loc_id < _locs.size());
    return _locs[loc_id];
}

std::string_view SrcMngr::str_at(loc_id_t loc_id, std::size_t size) {
    return {loc(loc_id).ptr(), size};
}

} // namespace ulam
