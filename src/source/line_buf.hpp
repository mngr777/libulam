#pragma once
#include "libulam/types.hpp"
#include "src/memory/str_buf.hpp"
#include "src/str_storage.hpp"
#include <istream>
#include <streambuf>
#include <string_view>

namespace ulam::src {

class LineBuf : public std::streambuf {
public:
    LineBuf(StrStorage& ss, std::istream* is);

    void set_input(std::istream* is) { _is = is; }

    const StrId line_id();
    const LineNum linum() const { return _linum; }
    const CharNum chr() const;

protected:
    // TODO: sync and other methods??
    int underflow() override;

private:
    const std::string_view line() const;

    StrStorage& _ss;
    std::istream* _is;
    mem::StrBuf _buf;
    LineNum _linum{0};
    StrId _line_id{NoStrId};
};

} // namespace ulam::src
