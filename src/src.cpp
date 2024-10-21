#include "src/src.hpp"
#include <cstring>
#include <cstdio>

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

const mem::BufRef FileSrc::content() {
    if (!_buf)
        _buf = file_contents(_path);
    return _buf->ref();
}

} // namespace ulam
