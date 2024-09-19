#include "src/source_manager.hpp"
#include "src/source.hpp"
#include <cassert>
#include <mutex>

namespace ulam {

std::shared_ptr<Source>
SourceManager::add_string(std::string name, std::string text) {
    std::unique_lock lck(_src_mtx);
    const SrcId id = _srcs.size();
    auto src = std::make_shared<StrSource>(
        *this, id, std::move(name), std::move(text), &_src_pool);
    return src;
}

std::shared_ptr<Source> SourceManager::add_file(std::filesystem::path file) {
    std::unique_lock lck(_src_mtx);
    const SrcId id = _srcs.size();
    auto src =
        std::make_shared<FileSource>(*this, id, std::move(file), &_src_pool);
    return src;
}

std::shared_ptr<Source> SourceManager::get(const SrcId id) {
    assert(id < _srcs.size() && "Invalid source ID");
    return _srcs[id];
}

const SrcLocId
SourceManager::loc_id(const SrcId src_id, const LineNum linum, const CharNum chr) {
    std::unique_lock lck(_loc_mtx);
    SrcLocId id = _locs.size();
    _locs.emplace_back(src_id, linum, chr);
    return id;
}

} // namespace ulam
