#pragma once
#include "libulam/types.hpp"
#include <filesystem>
#include <memory>
#include <memory_resource>
#include <shared_mutex>
#include <vector>

namespace ulam {

class Source;

struct SourceLoc {
    SourceLoc(SrcId src_id, StrId line_id, LineNum linum, CharNum chr):
        src_id(src_id), line_id(line_id), linum(linum), chr(chr) {}

    const SrcId src_id;
    const StrId line_id; // TODO: we would like to do without one or the other.
    const LineNum linum; //       add linum metadata to notebook?
    const CharNum chr;
};

class SourceManager {
public:
    SourceManager(std::pmr::memory_resource* res):
        _loc_pool(res), _locs(&_loc_pool) {}

    std::shared_ptr<Source> add_string(std::string name, std::string text);
    std::shared_ptr<Source> add_file(std::filesystem::path file);
    std::shared_ptr<Source> get(const SrcId id);

    const SrcLocId loc_id(
        const SrcId src_id,
        const StrId line_id,
        const LineNum linum,
        CharNum chr);

private:
    std::pmr::synchronized_pool_resource _src_pool;
    std::vector<std::shared_ptr<Source>> _srcs;
    std::shared_mutex _src_mtx;

    std::pmr::unsynchronized_pool_resource _loc_pool;
    std::pmr::vector<SourceLoc> _locs;
    std::shared_mutex _loc_mtx;
};

} // namespace ulam
