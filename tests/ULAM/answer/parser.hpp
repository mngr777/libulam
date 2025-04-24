#pragma once
#include "../parser.hpp"
#include <string_view>
#include <string>

class AnswerParser : public Parser {
public:
    using Parser::Parser;

    struct ClassNameData {
        const std::string_view name;
        bool is_tpl{};
    };

    struct TypeDefData {
        const std::string_view alias;
        std::string text; // possibly re-formatted
    };

    struct DataMemData {
        const std::string_view name;
        std::string text; // possibly re-formatted
        bool is_const{};
    };

    ClassNameData read_class_name(bool is_parent = false);
    ClassNameData read_parent_class_name() { return read_class_name(true); }

    const std::string_view read_type_ident();
    void skip_type_ident() { read_type_ident(); }

    const std::string_view read_type_name();
    void skip_type_name() { read_type_name(); }

    const std::string_view read_data_mem_name();
    void skip_data_mem_name() { read_data_mem_name(); }

    TypeDefData read_type_def();
    DataMemData read_data_mem();

    std::string read_value_str(bool is_array);
    std::string read_scalar_value_str();

    void skip_parens() { read_parens(); }
    void skip_brackets() { read_brackets(); }
    void skip_braces() { read_braces(); }
    void skip_comment();

    bool at_ident() const;
};
