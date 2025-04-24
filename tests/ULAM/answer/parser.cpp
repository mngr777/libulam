#include "./parser.hpp"
#include "../answer.hpp"
#include <cassert>
#include <map>

namespace {

constexpr char TypeDef[] = "typedef";
constexpr char Constant[] = "constant";
constexpr char Parameter[] = "parameter";
constexpr char Holder[] = "holder";
constexpr char Unresolved[] = "unresolved";

} // namespace

AnswerParser::ClassNameData AnswerParser::read_class_name(bool is_parent) {
    auto start = pos();
    // Type
    if (!is_parent && !at('U'))
        error("class name must start with 'U'");
    while (at_ident())
        advance();
    // ()
    bool is_tpl = at('(');
    if (is_tpl)
        skip_parens();
    return {substr_from(start), is_tpl};
}

const std::string_view AnswerParser::read_type_ident() {
    auto start = pos();
    if (!at_upper()) {
        if (at(Holder)) {
            skip(Holder);
        } else if (at(Unresolved)) {
            skip(Unresolved);
        } else {
            error("type name must start with uppercase letter");
        }
        return substr_from(start);
    }
    advance();
    while (at_ident())
        advance();
    return substr_from(start);
}

const std::string_view AnswerParser::read_type_name() {
    auto start = pos();
    skip_type_ident();
    if (at('('))
        skip_parens();
    return substr_from(start);
}

const std::string_view AnswerParser::read_data_mem_name() {
    auto start = pos();
    if (!at_lower())
        error("constant or property name must start with lowercase letter");
    advance();
    while (at_ident())
        advance();
    return substr_from(start);
}

AnswerParser::TypeDefData AnswerParser::read_type_def() {
    auto start = pos();

    // typedef
    skip(TypeDef);
    skip_spaces();

    // Type()
    skip_type_name();
    skip_spaces();

    // alias
    auto alias = read_type_ident();

    // []
    if (at('['))
        skip_brackets();

    std::string text{substr_from(start)};

    // ;
    skip_spaces();
    skip(';');

    return {alias, std::move(text)};
}

AnswerParser::DataMemData AnswerParser::read_data_mem() {
    auto start = pos();

    // constant/parameter
    bool is_const = false;
    if (at(Constant)) {
        is_const = true;
        skip(Constant);
    } else if (at(Parameter)) {
        is_const = true;
        skip(Parameter);
    }
    skip_spaces();

    // Type()
    skip_type_name();
    skip_spaces();

    // name
    auto name = read_data_mem_name();

    // []
    bool is_array = at('[');
    if (is_array)
        skip_brackets();

    std::string text{substr_from(start)};
    skip_spaces();

    // value
    if (at('(')) {
        text += read_value_str(is_array);

    } else if (at('=')) {
        advance();
        skip_spaces();
        text += " = " + read_value_str(is_array);
    }

    // ;
    skip_spaces();
    skip(';');

    return {name, std::move(text), is_const};
}

std::string AnswerParser::read_value_str(bool is_array) {
    std::string str;

    bool is_obj_item = false;
    bool is_scalar_item = false;

    // {name, text}
    std::map<const std::string_view, std::string> members;

    auto add_obj_item = [&]() {
        assert(is_obj_item);
        if (is_array && !str.empty())
            str += ", ";
        if (is_array)
            str += "("; // wrap array item
        // combine all members, ordered by name
        auto mem_start = str.size();
        for (auto [_, text] : members) {
            if (str.size() > mem_start)
                str += " ";
            str += text + ";";
        }
        if (is_array)
            str += ")";  // wrap array item
        members.clear(); // reset members for current array item
    };

    auto add_member = [&](const std::string_view name, std::string&& text) {
        assert(!name.empty() && !text.empty());
        if (members.count(name) > 0) {
            if (is_array) {
                // workaround for t3143, t3145:
                // `Bar m_bar2[2](Bool val_b[3](...); Bool val_b[3](...);
                // );` -- objects not separated
                add_obj_item();
            } else {
                error("member `" + std::string{name} + "' already exists");
            }
        }
        members[name] = std::move(text);
    };

    bool in_parens = at('(');
    if (in_parens)
        advance();
    skip_spaces();

    AnswerBasePrefixStack pref;
    while (!at(';')) {
        if (at(':') || at('^')) {
            // :ParentType< ... > | ^GrandparentType< ... >
            bool is_grandparent = at('^');
            advance();
            auto [name, _] = read_parent_class_name();
            pref.push((is_grandparent ? "^" : "") + std::string{name});
            skip_spaces();
            skip('<');

        } else if (at('>')) {
            pref.pop();
            advance();

        } else if (at(TypeDef)) {
            // typedef
            if (is_scalar_item) {
                assert(is_array);
                error("unexpected typedef in array value");
            }
            is_obj_item = true;
            auto [alias, type_def_text] = read_type_def();
            add_member(alias, std::move(type_def_text));

        } else if (at(Constant) || at(Parameter) || at_upper()) {
            // constant/property
            if (is_scalar_item) {
                assert(is_array);
                error("unexpected data member in array value");
            }
            is_obj_item = true;
            auto [name, data_mem_text, _] = read_data_mem();
            add_member(name, std::move(data_mem_text));

        } else if (at(')')) {
            // end of parenthesized value or object item in array
            if (!in_parens)
                error("unexpected `)'");
            advance();

            // add current object?
            if (is_obj_item)
                add_obj_item();

            // ,?
            if (at(',')) {
                // comma after closing `)' must be an object array
                if (!(is_array && is_obj_item))
                    error("unexpected `,'");
                advance();
                skip_spaces();
                skip("(");
            } else {
                break; // done!
            }

        } else if (at(',')) {
            if (!is_array && !is_scalar_item)
                error("unexpected `,'");
            advance();

        } else {
            // everything else must be scalar value
            if (is_obj_item)
                error("unexpected char in object value");
            is_scalar_item = true;
            if (!str.empty()) {
                // value string is already not empty, must be array item
                assert(is_array);
                str += ", ";
            }
            str += read_scalar_value_str();
        }
        skip_spaces();
    }
    assert(at(';'));
    return in_parens ? '(' + str + ')' : str;
}

std::string AnswerParser::read_scalar_value_str() {
    if (at('"'))
        return std::string{read_str_lit()};
    auto start = pos();
    while (!eof() && !at(';') && !at(')') && !at(',') && !at(' '))
        advance();
    return std::string{substr_from(start)};
}

void AnswerParser::skip_comment() {
    if (at("/*")) {
        move_to("*/");
        advance(2);
    }
}

bool AnswerParser::at_ident() const { return at_alnum() || at('_'); }
