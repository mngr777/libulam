#pragma once
#include <libulam/src.hpp>
#include <libulam/src_loc.hpp>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ulam {

// TODO: REPL source

class SrcMngr {
public:
    using Path = std::filesystem::path;

    SrcMngr() {}

    SrcMngr(const SrcMngr&) = delete;
    SrcMngr& operator=(const SrcMngr&) = delete;

    Src* string(std::string text, Path path);
    Src* file(Path path);

    Src* src(src_id_t src_id);
    Src* src(const Path& path);

    loc_id_t loc_id(src_id_t src_id, const char* ptr, linum_t linum, chr_t chr);
    SrcLoc loc(loc_id_t loc_id);

    std::string_view str_at(const SrcLoc& loc, std::size_t size, int off = 0);
    std::string_view str_at(loc_id_t loc_id, std::size_t size, int off = 0);

    std::string_view line_at(const SrcLoc& loc);
    std::string_view line_at(loc_id_t loc_id);

private:
    std::vector<std::unique_ptr<Src>> _srcs;
    std::map<Path, Src*> _src_map;
    std::vector<SrcLoc> _locs;
};

} // namespace ulam
