#include "src/source.hpp"
#include "libulam/types.hpp"
#include "src/source_manager.hpp"
#include <cassert>
#include <memory_resource>

namespace ulam {

Source::Source(
    SourceManager& sm,
    const SrcId id,
    std::string name,
    std::pmr::memory_resource* res):
    id(id), name(std::move(name)), _sm(sm), _lines(res), _buf(_lines, nullptr), _out(&_buf) {}

const SrcLocId Source::loc_id() {
    return _sm.loc_id(id, _buf.line_id(), _buf.linum(), _buf.chr());
}

void Source::init(std::istream* is) { _buf.set_input(is); }

StrSource::StrSource(
    SourceManager& sm,
    const SrcId id,
    std::string name,
    std::string text,
    std::pmr::memory_resource* res)
    : Source(sm, id, std::move(name), res), _is(text) {
    init(&_is);
}


FileSource::FileSource(
    SourceManager& sm,
    const SrcId id,
    std::filesystem::path path,
    std::pmr::memory_resource* res):
    Source(sm, id, path, res), _is(path) {
    init(&_is);
}

} // namespace ulam
