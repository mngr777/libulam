#include "src/source/line_buf.hpp"
#include <cassert>

#ifdef DEBUG_LINEBUF
#include <cstring>
#  define ULAM_DEBUG
// #  define ULAM_DEBUG_PREFIX "[ulam::LineBuf] "
#endif
#include "src/debug.hpp"

namespace ulam::src {

LineBuf::LineBuf(StrStorage& ss, std::istream* is): _ss(ss), _is(is), _buf(nullptr) {
    auto data = _buf.data();
    setg(data, data + _buf.size(), data + _buf.size());
}

const StrId LineBuf::line_id() {
    if (_line_id == NoStrId) {
        _line_id = _ss.put(line(), true /* copy */);
        debug() << "Line " << _linum << " stored, ID: " << _line_id;
    }
    return _line_id;
}

const CharNum LineBuf::chr() const { return gptr() - eback(); }

int LineBuf::underflow() {
    assert(_is && "Source stream not set");
    if (gptr() < egptr())
        return traits_type::to_int_type(*gptr());
    if (_is->eof())
        return traits_type::eof();

    auto cur = _buf.data();
    std::size_t num_read = 0;
    do {
        std::streamsize count = _buf.end() - cur - 1 /* for \n */;
#ifdef ULAM_DEBUG
        std::memset(_buf.begin(), 0, _buf.size());
#endif
        assert(count > 0);
        _is->getline(cur, count, '\n');
        num_read += _is->gcount();

        if (_is->gcount() == 0) {
            // no more data
            return traits_type::eof();

        } else if (_is->fail()) {
            // not enough space
            assert(_is->gcount() == count - 1);
            _buf.grow();
            cur = _buf.data() + num_read;
            _is->clear();
            continue;
        }
        ++_linum;
        traits_type::char_type* last = _buf.data() + num_read - 1;
        *last = '\n';
        setg(_buf.data(), _buf.data(), last + 1);
        break;
    } while (true); // TODO: set some reasonable limit just in case
    return traits_type::to_int_type(*gptr());
}

const std::string_view LineBuf::line() const {
    return {
        eback(), static_cast<std::string_view::size_type>(egptr() - eback())};
}

} // namespace ulam::src
