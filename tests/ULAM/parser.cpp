#include "./parser.hpp"
#include <cassert>
#include <stdexcept>

Parser::~Parser() {}

char Parser::chr(std::size_t off) const {
    auto pos = _pos + off;
    return (pos < _text.size()) ? _text[pos] : '\0';
}

bool Parser::at(const std::string& str) const {
    return str == _text.substr(_pos, str.size());
}

bool Parser::eof() const {
    assert(_pos <= _text.size());
    return _pos == _text.size();
}

bool Parser::at(char ch) const { return chr() == ch; }

bool Parser::at_digit() const { return '0' <= chr() && chr() <= '9'; }

bool Parser::at_upper() const { return 'A' <= chr() && chr() <= 'Z'; }

bool Parser::at_lower() const { return 'a' <= chr() && chr() <= 'z'; }

bool Parser::at_alpha() const { return at_upper() || at_lower(); }

bool Parser::at_alnum() const { return at_digit() || at_alpha(); }

void Parser::advance(std::size_t n) {
    if (n == 0)
        error("advance argument is 0");
    if (_pos + n > _text.size())
        error("cannot advance past end of text");
    _pos += n;
}

void Parser::to_eof() { _pos = _text.size(); }

std::size_t Parser::move_to(const std::string& str, SearchMode mode) {
    auto start = _pos;
    auto n = _text.find(str, _pos);
    if (n != std::string::npos) {
        _pos = n;
    } else {
        switch (mode) {
        case SearchRequired:
            error(not_found_str(str));
            break;
        case SearchMaybe:
            break;
        case SearchOrEof:
            to_eof();
            break;
        }
    }
    return _pos - start;
}

std::size_t Parser::move_to(char ch, SearchMode mode) {
    auto start = _pos;
    auto n = _text.find(ch, _pos);
    if (n != std::string::npos) {
        _pos = n;
    } else {
        switch (mode) {
        case SearchRequired:
            error(not_found_str(ch));
            break;
        case SearchMaybe:
            break;
        case SearchOrEof:
            to_eof();
            break;
        }
    }
    return _pos - start;
}

void Parser::skip(const std::string& str) {
    if (!at(str))
        error(not_found_str(str));
    advance(str.size());
}

void Parser::skip(char ch) {
    if (!at(ch))
        error(not_found_str(ch));
    advance();
}

bool Parser::skip_if(const std::string& str) {
    bool is_match = at(str);
    if (is_match)
        advance(str.size());
    return is_match;
}

bool Parser::skip_if(char ch) {
    bool is_match = at(ch);
    if (is_match)
        advance();
    return is_match;
}

void Parser::skip_spaces() {
    while (at(' '))
        advance();
}

const std::string_view Parser::read_line(bool with_nl) {
    assert(!eof());
    auto start = _pos;
    move_to("\n", SearchOrEof);
    if (with_nl)
        advance();
    auto line = substr_from(start);
    if (!with_nl)
        advance();
    return line;
}

const std::string_view Parser::read_block(char open, char close, char esc) {
    if (!at(open))
        error(not_found_str(open));
    auto start = _pos;
    advance();
    unsigned opened = 1;
    while (!eof() && opened > 0) {
        if (at(close)) {
            assert(opened > 0);
            --opened;
        } else if (at(open)) {
            assert(close != open);
            ++opened;
        } else if (at(esc)) {
            advance();
        }
        advance();
    }
    if (eof()) {
        auto message =
            std::string{"closing `"} + close + "' not found opened at";
        error(message, start);
    }
    return substr_from(start);
}

int Parser::read_int() {
    int sign = 1;
    if (at('-')) {
        sign = -1;
        advance();
    }
    if (!at_digit())
        error("not an integer");
    int value = 0;
    while (!eof() && at_digit()) {
        value = value * 10 + chr() - '0';
        advance();
    }
    return sign * value;
}

const std::string_view Parser::substr_from(std::size_t start) const {
    assert(start <= _pos);
    return _text.substr(start, _pos - start);
}

void Parser::error(const std::string& message, std::size_t pos) const {
    if (pos == std::string::npos)
        pos = _pos;
    assert(pos <= _pos);
    throw std::invalid_argument(
        message + " (chr:" + std::to_string(_pos) + ") `" +
        std::string{_text.substr(_pos, 16)} + "'");
}

std::string Parser::not_found_str(const std::string& str) const {
    return "`" + str + "' not found";
}

std::string Parser::not_found_str(char ch) const {
    return std::string{"`"} + ch + "' not found";
}
