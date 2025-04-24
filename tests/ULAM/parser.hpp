#pragma once
#include <string>
#include <string_view>

class Parser {
public:
    enum SearchMode { SearchRequired, SearchMaybe, SearchOrEof };

    Parser(const std::string_view text): _text{text}, _pos{0} {}
    virtual ~Parser();

    std::size_t pos() { return _pos; }

    char chr(std::size_t off = 0) const;
    bool at(const std::string& str) const;
    bool at(char ch) const;
    bool eof() const;

    bool at_digit() const;
    bool at_upper() const;
    bool at_lower() const;
    bool at_alpha() const;
    bool at_alnum() const;

    void advance(std::size_t n = 1);
    void to_eof();

    std::size_t
    move_to(const std::string& str, SearchMode mode = SearchRequired);
    std::size_t move_to(char ch, SearchMode mode = SearchRequired);

    void skip(const std::string& str);
    void skip(char ch);
    void skip_spaces();

    const std::string_view read_line(bool with_nl = false);

    int read_int();

    const std::string_view substr_from(std::size_t start) const;

    void error(const std::string& message) const;

    std::string not_found_str(const std::string& str) const;
    std::string not_found_str(char ch) const;

    const std::string_view _text;
    std::size_t _pos;
};
