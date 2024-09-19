#pragma once
#include "libulam/types.hpp"
#include "src/memory/str_buf.hpp"
#include "src/source/line_storage.hpp"
#include <istream>
#include <memory_resource>
#include <streambuf>

namespace ulam::src {

class LineBuf : public std::streambuf {
public:
    LineBuf(
        LineStorage& lines, std::istream& is, std::pmr::memory_resource* res);

    LineBuf(const LineBuf&) = delete;
    LineBuf& operator=(const LineBuf&) = delete;

    const LineNum linum() const { return _linum; }
    const CharNum chr() const;

    void store();

protected:
    // TODO: other virtual methods
    int underflow() override;

private:
    bool get_next();
    bool read_next();

    LineStorage& _lines;
    std::istream& _is;
    mem::StrBuf _buf;
    LineNum _linum {0};
};

} // namespace ulam::src
