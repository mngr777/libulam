#include "libulam/src.hpp"
#include <cstdio>
#include <cstring>

namespace ulam {

namespace {

// TODO: report errors
mem::Buf file_contents(const std::filesystem::path& path) {
    auto fd = std::fopen(path.c_str(), "r");
    if (!fd)
        return {};
    std::fseek(fd, 0, SEEK_END);
    std::size_t size = std::ftell(fd);
    std::rewind(fd);
    mem::Buf buf{size + 1};
    std::size_t num_read = std::fread(buf.start(), 1, size, fd);
    *(buf.start() + num_read) = '\0';
    return buf;
}

} // namespace

const mem::BufRef Src::line(linum_t linum) {
    assert(linum > 0);
    const auto buf = content();
    const char* start = buf.start();
    if (linum + 1 > _line_off.size()) {
        std::size_t off = _line_off.empty() ? 0 : _line_off.back();
        const char* cur = start + off;
        bool is_eof = false;
        do {
            while (cur[0] != '\n' && cur[0] != '\0')
                ++cur;
            is_eof = (cur[0] == '\0');
            if (!is_eof)
                ++cur;
            _line_off.push_back(cur - start);
        } while (linum + 1 > _line_off.size() && !is_eof);
    }
    return (linum + 1 > _line_off.size())
        ? mem::BufRef(start + _line_off[linum - 1], _line_off[linum] - _line_off[linum - 1])
        : mem::BufRef(buf.end(), 0);
}


const mem::BufRef FileSrc::content() {
    if (!_buf)
        _buf = file_contents(_path);
    return _buf->ref();
}

} // namespace ulam
