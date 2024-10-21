#pragma once
#include "src/src.hpp"
#include <libulam/src_loc.hpp>
#include <filesystem>
#include <vector>

namespace ulam {

class SrcMngr {
public:
    Src* str(std::string text);
    Src* file(std::filesystem::path path);

    loc_id_t loc_id(src_id_t src_id, const char* ptr, linum_t linum, chr_t chr);

private:
    using Ptr = std::unique_ptr<Src>;
    std::vector<Ptr> _srcs;
    std::vector<SrcLoc> _locs;
};

} // namespace ulam
