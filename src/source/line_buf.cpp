#include "src/source/line_buf.hpp"
#include <cassert>
#include <memory_resource>
#include <string_view>

#ifdef DEBUG_LINEBUF
#    include <cstring>
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::LineBuf] "
#endif
#include "src/debug.hpp"

namespace ulam::src {

LineBuf::LineBuf(
    LineStorage& lines, std::istream& is, std::pmr::memory_resource* res):
    _lines(lines), _is(is), _buf(res) {
    assert(is.tellg() == 0 && "Avoid passing `used' streams for now");
    auto data = _buf.data();
    setg(data, data + _buf.size(), data + _buf.size());
}

const CharNum LineBuf::chr() const { return gptr() - eback(); }

void LineBuf::mark() {
    assert(chr() > 0 && "No line character were read yet");
    _mark = chr() - 1;
}

const std::string_view LineBuf::substr() {
    assert(_mark != NoMark);
    assert(_mark + 1 < chr() && "Trying to get empty substring");
    store();
    return {eback() + _mark, chr() - _mark - 1};
}

void LineBuf::store() {
    // already stored?
    if (eback() != _buf.data())
        return;
    auto rec = _lines.put(
        _linum, _is.tellg(),
        {eback(), static_cast<std::size_t>(egptr() - eback())});
    auto data = const_cast<char_type*>(rec.line.data());
    setg(data, data + chr(), data + rec.line.size());
}

int LineBuf::underflow() {
    if (_is.eof()) {
        assert(gptr() == egptr());
        return traits_type::eof();
    }
    // try getting the next line
    if (get_next()) {
        return traits_type::to_int_type(*gptr());
    }
    assert(gptr() == egptr());
    return traits_type::eof();
}

void LineBuf::unmark() { _mark = NoMark; }

bool LineBuf::get_next() {
    assert(!_is.eof());
    unmark(); // multi-line substrings are not supported

    // is next line already stored?
    if (!_lines.has(_linum + 1))
        return read_next();

    // use stored line
    auto rec = _lines.get(_linum + 1);
    assert(!rec.empty());
    ++_linum; // moved to next line
    auto data = const_cast<char_type*>(rec.line.data());
    setg(data, data, data + rec.line.size());
    return true;
}

bool LineBuf::read_next() {
    assert(!_is.eof());
    auto cur = _buf.data();
    std::size_t num_read = 0;
    while (true) {
        std::streamsize count = _buf.end() - cur - 1 /* for \n */;
#ifdef ULAM_DEBUG
        std::memset(_buf.begin(), 0, _buf.size());
#endif
        assert(count > 0);
        _is.getline(cur, count, '\n');
        num_read += _is.gcount();

        if (num_read == 0) {
            assert(_is.eof());
            return false;

        } else if (!_is.eof() && _is.fail()) {
            // not enough space
            assert(_is.gcount() == count - 1);
            _buf.grow();
            cur = _buf.data() + num_read;
            _is.clear();
            continue; // TODO: set some reasonable limit just in case
        }

        ++_linum; // moved to next line
        char_type* last = _buf.data() + num_read;
        if (num_read > 0 && !_is.eof()) {
            --last;
            *last = '\n';
        }
        setg(_buf.data(), _buf.data(), _buf.data() + num_read);
        break;
    }
    return true;
}

} // namespace ulam::src
