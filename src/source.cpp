#include "src/source.hpp"
#include "libulam/types.hpp"
#include "src/source_manager.hpp"
#include <cassert>

namespace ulam {

const SrcLocId SourceStream::last_loc_id() {
    _line_buf.store();
    _line_buf.mark();
    return _src.loc_id(_line_buf.linum(), _line_buf.chr());
}

const std::string_view SourceStream::substr() const {
    return _line_buf.substr();
}

Source::Source(
    SourceManager& sm,
    const SrcId id,
    std::string name,
    std::pmr::memory_resource* res):
    _sm(sm), _res(res), _lines(_res), _id(id), _name(std::move(name)) {}

SourceStream& Source::stream() {
    if (!_out)
        _out.emplace(*this);
    return _out->stream;
}

const SrcLocId Source::loc_id(const LineNum linum, const CharNum chr) {
    return _sm.loc_id(_id, linum, chr);
}

StrSource::StrSource(
    SourceManager& sm,
    const SrcId id,
    std::string name,
    std::string text,
    std::pmr::memory_resource* res):
    Source(sm, id, std::move(name), res), _is(text) {}

FileSource::FileSource(
    SourceManager& sm,
    const SrcId id,
    std::filesystem::path path,
    std::pmr::memory_resource* res):
    Source(sm, id, path, res), _is(path) {}

} // namespace ulam
