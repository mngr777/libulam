#include <cstdio>
#include <cstring>
#include <libulam/assert.hpp>
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
    auto buf = content();

    auto off = line_off(linum);
    if (off == NoOff)
        return {buf.end(), 0};

    auto next_off = line_off(linum + 1);
    if (next_off == NoOff)
        next_off = buf.size();
    return {buf.start() + off, next_off - 1 - off};
}

const mem::BufRef
Src::str(linum_t linum, chr_t chr, std::size_t size, int off) {
    auto buf = content();

    ulam_assert(chr > 0);
    off += chr - 1;

    std::size_t start = line_off(linum);
    if (start == NoOff)
        return {buf.end(), 0};

    ulam_assert(off > 0 || (std::size_t)-off <= start);
    start += off;
    return {buf.sub(start, size)};
}

std::size_t Src::line_off(linum_t linum) {
    ulam_assert(linum > 0);
    if (linum == 1)
        return 0;

    const auto idx = linum - 2; // result line offset idx

    auto buf = content();
    std::size_t off = _line_off.empty() ? 0 : _line_off.back();
    const char* cur = buf.start() + off;
    while (_line_off.size() <= idx) {
        while (*cur != '\n' && *cur != '\0')
            ++cur;
        if (*cur == '\0')
            break;
        ++cur;
        _line_off.push_back(cur - buf.start());
    }
    return (idx < _line_off.size()) ? _line_off[idx] : NoOff;
}

const mem::BufRef FileSrc::content() {
    // TODO: error handling
    if (!_buf)
        _buf = file_content(_path);
    ulam_assert(_buf);
    return _buf->ref();
}

} // namespace ulam
