#include "src/source/line_storage.hpp"
#include <cassert>
#include <mutex>
#include <shared_mutex>

namespace ulam::src {

namespace {
std::size_t linum_to_idx(const LineNum linum) {
    assert(0 < linum);
    return linum - 1;
}

} // namespace

const LineStorage::Rec LineStorage::get(const LineNum linum) const {
    std::shared_lock lck(_mtx);
    assert(linum <= _lines.size());
    return _lines[linum_to_idx(linum)];
}

const LineStorage::Rec& LineStorage::put(
    const LineNum linum,
    const std::size_t offset,
    const std::string_view line) {
    std::unique_lock lck(_mtx);
    reserve(linum);
    auto& rec = _lines[linum_to_idx(linum)];
    assert(rec.empty() && "Cannot store a line twice");
    rec = {offset, _notepad.write(line)};
    return rec;
}

bool LineStorage::has(const LineNum linum) const {
    std::shared_lock lck(_mtx);
    return linum < _lines.size() + 1 && !_lines[linum_to_idx(linum)].empty();
}

const LineNum LineStorage::size() const {
    std::shared_lock lck(_mtx);
    return _lines.size();
}

void LineStorage::reserve(const LineNum linum) {
    _lines.resize(linum_to_idx(linum) + 1);
}

} // namespace ulam::src
