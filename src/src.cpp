#include <cstdio>
#include <cstring>
#include <libulam/memory/buf.hpp>
#include <libulam/src.hpp>

namespace ulam {

namespace {

mem::Buf file_content(const std::filesystem::path& path) {
    auto fd = std::fopen(path.c_str(), "r");
    if (!fd) {
        perror("failed to read input file");
        return {};
    }
    std::fseek(fd, 0, SEEK_END);
    std::size_t size = std::ftell(fd);
    std::rewind(fd);
    mem::Buf buf{size + 1};
    std::size_t num_read = std::fread(buf.start(), 1, size, fd);
    fclose(fd);
    *(buf.start() + num_read) = '\0';
    return buf;
}

} // namespace

const mem::BufRef Src::line(linum_t linum) {
    assert(linum > 0);
    const auto buf = content();
    const char* start = buf.start();
    if (linum > _line_off.size()) {
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
        } while (linum > _line_off.size() && !is_eof);
    }
    if (linum > _line_off.size())
        return {buf.end(), 0};
    std::size_t off = (linum < 2) ? 0 : _line_off[linum - 2];
    return {start + off, _line_off[linum - 1] - off};
}

const mem::BufRef FileSrc::content() {
    if (!_buf)
        _buf = file_content(_path);
    return _buf->ref();
}

} // namespace ulam
