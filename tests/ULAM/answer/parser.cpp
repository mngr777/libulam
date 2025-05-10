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

enum ValueType {
    NoValueType,
    ScalarValue,
    ObjValue,
    ObjMapValue
};

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

    // Type(...)&
    skip_type_name();
    skip_spaces();
    if (at('&')) {
        advance();
        skip_spaces();
    }

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

    ValueType val_type = NoValueType;

    // {name, text}
    std::map<std::string, std::string> members;

    unsigned added = 0;
    auto add_obj_item = [&]() {
        assert(val_type == ObjValue);
        // NOTE: empty strings are printed for empty string values, we still
        // want to separate them, see t3945
        if (is_array && added > 0)
            str += ", ";
        if (is_array)
            str += "("; // wrap array item
        // combine all members, ordered by name
        auto mem_start = str.size();
        for (auto [name, text] : members) {
            if (str.size() > mem_start)
                str += " ";
            if (name.rfind('.') != std::string::npos)
                str += "<" + name + ">";
            str += text + ";";
        }
        if (is_array)
            str += ")"; // wrap array item
        ++added;
        members.clear(); // reset members for current array item
    };

    auto add_member = [&](std::string&& name, std::string&& text) {
        assert(!name.empty() && !text.empty());
        if (members.count(name) > 0) {
            if (is_array) {
                // workaround for t3143, t3145:
                // `Bar m_bar2[2](Bool val_b[3](...); Bool val_b[3](...);
                // );` -- objects not separated
                add_obj_item();
            } else {
                error("member `" + name + "' already exists");
            }
        }
        members[std::move(name)] = std::move(text);
    };

    // NOTE: array values can be in `({<item1>, <item2>, ...})` format, see
    // t3946
    char open = '\0', close = '\0';
    bool in_parens = false;
    auto open_brace_pos = std::string::npos;
    if (at('(')) {
        in_parens = true;
        open = '(', close = ')';
        advance();
        skip_spaces();
    }
    if (at('{')) {
        open = '{', close = '}';
        open_brace_pos = _pos;
        advance();
    }
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
            if (val_type == ScalarValue) {
                assert(is_array);
                error("unexpected typedef in array value");
            }
            val_type = ObjValue;
            auto [alias, type_def_text] = read_type_def();
            add_member(pref.add_prefix(alias), std::move(type_def_text));

        } else if (
            at(Constant) || at(Parameter) ||
            (at_upper() && !at("Atom,") && !at("Atom)") && !at("HexU64") &&
             !at("UNINITIALIZED_STRING"))) {
            // constant/property
            if (val_type == ScalarValue) {
                assert(is_array);
                error("unexpected data member in array value");
            }
            val_type = ObjValue;
            auto [name, data_mem_text, _] = read_data_mem();
            add_member(pref.add_prefix(name), std::move(data_mem_text));

        } else if (at(".")) {
            // object map
            assert(val_type == NoValueType);
            assert(open_brace_pos != std::string::npos);
            val_type = ObjMapValue;
            _pos = open_brace_pos;
            str = read_braces();
            if (in_parens) {
                skip_spaces();
                skip(")");
            }
            break; // done!

        } else if (at(close)) {
            // end of parenthesized/braced value or object item in array
            advance();

            // add current object?
            // NOTE: array may consist of empty objects in `Type
            // name[](),(),...()` format, t3942
            if (val_type == ObjValue || (is_array && val_type != ScalarValue)) {
                assert(val_type != ObjMapValue);
                val_type = ObjValue;
                add_obj_item();
            }

            // ,?
            if (open == '(' && at(',')) {
                // comma after closing `)' must be an object array
                if (!(is_array && val_type == ObjValue))
                    error("unexpected `,'");
                advance();
                skip_spaces();
                skip('(');

            } else {
                // array value in `({...})` format, see t3946
                if (close == '}' && in_parens) {
                    skip_spaces();
                    skip(')');
                }
                break; // done!
            }

        } else if (at(',')) {
            if (!is_array && val_type != ScalarValue)
                error("unexpected `,'");
            advance();

        } else {
            // everything else must be scalar value (treating `Atom` as scalar)
            if (val_type == ObjValue)
                error("unexpected char in object value");
            val_type = ScalarValue;
            if (!str.empty()) {
                // value string is already not empty, must be array item
                // assert(is_array)
                is_array = true; // t41277, class as Bits chunk array
                str += ", ";
            }
            str += read_scalar_value_str(close);
        }
        skip_spaces();
    }
    assert(at(';'));

    if (in_parens)
        open = '(', close = ')';
    return open != '\0' ? open + str + close : str;
}

std::string AnswerParser::read_scalar_value_str(char close) {
    // string literal
    if (at('"'))
        return std::string{read_str_lit()};

    // Hex64U(0x..., 0x...)
    auto start = pos();
    if (at("HexU64(")) {
        skip("HexU64");
        skip_parens();
        return std::string{substr_from(start)};
    }

    while (!eof() && !at(close) && !at(';') && !at(',') && !at(' '))
        advance();
    std::string str{substr_from(start)};

    // skip " cast", see t41101: expression strings are not available at compile
    // time (why is there a cast?)
    skip_if(" cast");

    return str;
}

void AnswerParser::skip_comment() {
    if (at("/*")) {
        move_to("*/");
        advance(2);
    }
}

bool AnswerParser::at_ident() const { return at_alnum() || at('_'); }
