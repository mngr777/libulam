#include "./answer.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {

constexpr char TestFunStart[] = "Int test()";
constexpr char NoMain[] = "<NOMAIN>";
constexpr char Constant[] = "constant";
constexpr char TypeDef[] = "typedef";

} // namespace

bool Answer::has_type_def(const std::string& alias) const {
    return _type_defs.count(alias) > 0;
}

std::string Answer::type_def(const std::string& alias) const {
    auto it = _type_defs.find(alias);
    return (it != _type_defs.end()) ? it->second : std::string{};
}

void Answer::add_type_def(std::string alias, std::string text) {
    assert(!has_type_def(alias));
    _type_defs.emplace(std::move(alias), std::move(text));
}

bool Answer::has_const(const std::string& name) const {
    return _consts.count(name) > 0;
}

std::string Answer::const_(const std::string& name) const {
    auto it = _consts.find(name);
    return (it != _consts.end()) ? it->second : std::string{};
}

void Answer::add_const(std::string name, std::string text) {
    assert(!has_const(name));
    _consts.emplace(std::move(name), std::move(text));
}

bool Answer::has_prop(const std::string& name) const {
    return _props.count(name) > 0;
}

std::string Answer::prop(const std::string& name) const {
    auto it = _props.find(name);
    return (it != _props.end()) ? it->second : std::string{};
}

void Answer::add_prop(std::string name, std::string text) {
    assert(!has_prop(name));
    _props.emplace(std::move(name), std::move(text));
}

Answer parse_answer(const std::string_view text) {
    Answer answer;
    std::size_t pos{0};

    auto error = [&](const std::string& str,
                     std::size_t error_pos = std::string::npos) {
        if (error_pos == std::string::npos)
            error_pos = pos;
        throw std::invalid_argument(
            str + ", around (chr:" + std::to_string(error_pos) + ") '" +
            std::string{text.substr(error_pos, 16)} + "'");
    };

    auto skip_spaces = [&]() {
        while (text[pos] == ' ')
            ++pos;
    };

    auto is_digit = [&]() { return '0' <= text[pos] && text[pos] <= '9'; };
    auto is_upper = [&]() { return 'A' <= text[pos] && text[pos] <= 'Z'; };
    auto is_lower = [&]() { return 'a' <= text[pos] && text[pos] <= 'z'; };
    auto is_alpha = [&]() { return is_upper() || is_lower(); };
    auto is_alnum = [&]() { return is_alpha() || is_digit(); };
    auto is_ident = [&]() { return is_alnum() || text[pos] == '_'; };

    auto at = [&](const std::string& str) {
        return str == text.substr(pos, str.size());
    };

    auto skip = [&](const std::string& str) {
        if (!at(str))
            error(str + " not found");
        pos += str.size();
    };

    auto skip_until = [&](const std::string& str) {
        auto n = text.find(str, pos);
        if (n == std::string::npos)
            error(str + " not found");
        pos = n;
    };

    auto skip_block = [&](char open, char close, char esc = '\0') {
        if (text[pos] != open)
            error(std::string{"unexpectend char, expecting `"} + open + "'");

        auto start = pos;
        ++pos;
        int opened = 1;
        while (opened > 0 && pos < text.size()) {
            if (text[pos] == open) {
                ++opened;
            } else if (text[pos] == close) {
                --opened;
            } else if (text[pos] == esc) {
                ++pos;
            }
            ++pos;
        }
        if (pos == text.size())
            error("not closed", start);
    };

    auto skip_parens = std::bind(skip_block, '(', ')');
    auto skip_brackets = std::bind(skip_block, '[', ']');
    auto skip_braces = std::bind(skip_block, '{', '}');
    auto skip_str_lit = std::bind(skip_block, '"', '"', '\\');

    // {name, is_tpl}
    auto read_class_name =
        [&](bool is_parent = false) -> std::pair<const std::string_view, bool> {
        auto start = pos;
        if (!is_parent && text[pos] != 'U')
            error("class name must start with 'U'");
        // Type
        ++pos;
        while (is_ident())
            ++pos;
        // ()
        bool is_tpl = at("(");
        if (is_tpl)
            skip_parens();
        return {text.substr(start, pos - start), is_tpl};
    };

    auto read_parent_class_name = std::bind(read_class_name, true);

    auto skip_type_ident = [&]() {
        if (!is_upper() && !at("holder") && !at("unresolved"))
            error("type name must start with uppercase letter");
        ++pos;
        while (is_ident())
            ++pos;
    };

    auto skip_type_name = [&]() {
        skip_type_ident();
        // ()
        if (text[pos] == '(')
            skip_parens();
    };

    auto read_type_ident = [&]() {
        auto start = pos;
        skip_type_ident();
        return text.substr(start, pos - start);
    };

    auto read_data_mem_name = [&]() {
        auto start = pos;
        if (!is_lower())
            error("constant or property name must start with lowercase letter");
        ++pos;
        while (is_ident())
            ++pos;
        return text.substr(start, pos - start);
    };

    // {name, text}
    auto read_type_def = [&]() -> std::pair<std::string, std::string> {
        auto start = pos;

        // typedef
        skip(TypeDef);
        skip_spaces();

        // Type()
        skip_type_name();
        skip_spaces();

        // Type
        std::string alias{read_type_ident()};
        skip_spaces();

        // []
        if (text[pos] == '[')
            skip_brackets();

        std::string type_def_text{text.substr(start, pos - start)};

        // ;
        skip_spaces();
        skip(";");

        return {std::move(alias), std::move(type_def_text)};
    };

    // {name, text, is_const}
    auto _read_data_mem = [&](auto& self, auto& read_value_str)
        -> std::tuple<std::string, std::string, bool> {
        auto start = pos;

        // constant
        bool is_const = at(Constant);
        if (is_const)
            skip(Constant);

        // Type()
        skip_spaces();
        skip_type_name();
        skip_spaces();

        // ident
        std::string name{read_data_mem_name()};
        skip_spaces();

        // []
        bool is_array = at("[");
        if (is_array)
            skip_brackets();

        std::string data_mem_text{text.substr(start, pos - start)};

        // value
        skip_spaces();
        if (at("(")) {
            data_mem_text += read_value_str(read_value_str, self, is_array);

        } else if (at("=")) {
            skip("=");
            skip_spaces();
            data_mem_text +=
                " = " + read_value_str(read_value_str, self, is_array);
        }

        // ;
        skip_spaces();
        skip(";");

        return {std::move(name), std::move(data_mem_text), is_const};
    };

    auto read_scalar_value_str = [&]() -> const std::string_view {
        auto start = pos;
        auto ch = text[pos];
        if (ch == '"') {
            skip_str_lit();
        } else {
            while (pos < text.size() && text[pos] != ';' && text[pos] != ')' &&
                   text[pos] != ',' && text[pos] != ' ')
                ++pos;
            if (pos == text.size())
                error("value not terminated", start);
        }
        return text.substr(start, pos - start);
    };

    auto _read_value_str = [&](auto& self, auto& read_data_mem,
                               bool is_array) -> std::string {
        std::string val;
        bool is_obj_item = false;
        bool is_scalar_item = false;

        std::map<std::string, std::string> members;

        auto add_member = [&](std::string&& name, std::string&& text) {
            assert(!name.empty() && !text.empty());
            if (members.count(name) > 0)
                error("member `" + name + "' already exists");
            members[std::move(name)] = std::move(text);
        };

        auto add_obj_item = [&]() {
            assert(is_obj_item);
            if (is_array && !val.empty())
                val += ", ";
            if (is_array)
                val += "("; // wrap array item
            // combine all members, ordered by name
            auto mem_start = val.size();
            for (auto [_, text] : members) {
                if (val.size() > mem_start)
                    val += " ";
                val += text + ";";
            }
            if (is_array)
                val += ")";  // wrap array item
            members.clear(); // reset members for current array item
        };

        bool in_parens = at("(");
        if (in_parens)
            skip("(");
        skip_spaces();

        while (text[pos] != ';') {
            if (at(TypeDef)) {
                if (is_scalar_item) {
                    assert(is_array);
                    error("unexpected typedef in array value");
                }
                is_obj_item = true;
                auto [alias, type_def_text] = read_type_def();
                add_member(std::move(alias), std::move(type_def_text));

            } else if (at(Constant) || is_upper()) {
                if (is_scalar_item) {
                    assert(is_array);
                    error("unexpected data member in array value");
                }
                is_obj_item = true;
                auto [name, data_mem_text, is_const] =
                    read_data_mem(read_data_mem, self);
                if (is_const || members.count(name) == 0) {
                    add_member(std::move(name), std::move(data_mem_text));
                } else {
                    // workaround for t3143, t3145:
                    // `Bar m_bar2[2](Bool val_b[3](...); Bool val_b[3](...);
                    // );`
                    assert(members.size() == 1);
                    add_obj_item();
                    add_member(std::move(name), std::move(data_mem_text));
                }

            } else if (text[pos] == ')') {
                if (!in_parens)
                    error("unexpected `)'");

                // add current object?
                if (is_obj_item)
                    add_obj_item();

                ++pos;
                skip_spaces();

                // ,?
                if (text[pos] == ',') {
                    // comma after closing `)' must be object array
                    if (!(is_array && is_obj_item))
                        error("unexpected `,'");
                    ++pos;
                    skip_spaces();
                    skip("(");
                } else {
                    break; // done
                }

            } else if (text[pos] == ',') {
                if (!is_array && !is_scalar_item)
                    error("unexpected `,'");
                ++pos;
                skip_spaces();

            } else {
                // scalar value
                if (is_obj_item)
                    error("unexpected char in object value");
                is_scalar_item = true;
                if (!val.empty()) {
                    // value string is already not empty, must be array item
                    assert(is_array);
                    val += ", ";
                }
                val += read_scalar_value_str();
            }
            skip_spaces();
        }
        assert(text[pos] == ';');
        skip_spaces();
        return in_parens ? "(" + val + ")" : val;
    };

    auto read_data_mem =
        std::bind(_read_data_mem, _read_data_mem, _read_value_str);

    // class name
    skip_spaces();
    auto [name, is_tpl] = read_class_name();
    answer.set_is_tpl(is_tpl);
    answer.set_class_name(std::string{name});

    // params (TODO)
    skip_spaces();
    if (at("("))
        skip_parens();

    // parents
    skip_spaces();
    if (at(":")) {
        skip(":");
        skip_spaces();

        bool done = false;
        while (!done) {
            auto [name, _] = read_parent_class_name();
            answer.add_parent(std::string{name});
            skip_spaces();

            done = !at("+");
            if (!done)
                skip("+");
            skip_spaces();
        }
    }

    // {
    // skip_spaces();
    skip("{");

    // non-fun members
    skip_spaces();
    std::string base_prefix;
    while (!at(TestFunStart) && !at(NoMain) && !at("}")) {
        if (at("/*")) {
            skip_until("*/");
            skip("*/");

        } else if (at(":")) {
            assert(base_prefix.empty());
            skip(":");
            auto [name, _] = read_parent_class_name();
            base_prefix = std::string{name} + "::";
            skip_spaces();
            skip("<");

        } else if (at(">")) {
            assert(!base_prefix.empty());
            base_prefix.clear();
            skip(">");

        } else if (at(TypeDef)) {
            auto [alias, type_def_text] = read_type_def();
            answer.add_type_def(base_prefix + alias, type_def_text);

        } else {
            auto [name, data_mem_text, is_const] = read_data_mem();
            name = base_prefix + name;
            if (is_const) {
                answer.add_const(name, data_mem_text);
            } else {
                answer.add_prop(name, data_mem_text);
            }
        }
        skip_spaces();
    }

    // `test` fun text
    if (at(TestFunStart)) {
        auto start = pos;

        // Int test()
        skip(TestFunStart);
        skip_spaces();

        // {}
        skip_braces();

        std::string test_fun{text.substr(start, pos - start)};
        answer.set_test_fun(std::move(test_fun));

    } else {
        if (at(NoMain))
            skip(NoMain);
        answer.set_test_fun(NoMain);
    }

    skip_spaces();
    skip("}");

    return answer;
}

void compare_answer_maps(
    const AnswerMap& true_answers, const AnswerMap& answers) {
    for (const auto& [name, true_answer] : true_answers) {
        auto it = answers.find(name);
        if (it == answers.end()) {
            auto message = std::string{"class `"} + name +
                           "' not found in compiled classes";
            throw std::invalid_argument(message);
        }
        compare_answers(true_answer, it->second);
    }
}

void compare_answers(const Answer& truth, const Answer& answer) {
    assert(truth.class_name() == answer.class_name());

    // typedefs
    const auto& type_defs = answer.type_defs();
    for (const auto& [alias, text] : truth.type_defs()) {
        auto it = type_defs.find(alias);
        if (it == type_defs.end()) {
            auto message = std::string{"typedef `"} + alias +
                           "' not found in compiled class `" +
                           answer.class_name() + "'";
            throw std::invalid_argument(message);
        }
        auto answer_text = it->second;
        if (answer_text != text) {
            auto message = std::string{"typedef `"} + alias +
                           "' in compiled class '" + answer.class_name() +
                           "` does not match the answer:\n`" + answer_text +
                           "`\n vs\n`" + text + "`";
            throw std::invalid_argument(message);
        }
    }

    // constants
    const auto& consts = answer.consts();
    for (const auto& [name, text] : truth.consts()) {
        auto it = consts.find(name);
        if (it == consts.end()) {
            auto message = std::string{"constant `"} + name +
                           "' not found in compiled class `" +
                           answer.class_name() + "'";
            throw std::invalid_argument(message);
        }
        auto answer_text = it->second;
        if (answer_text != text) {
            auto message = std::string{"constant `"} + name +
                           "' in compiled class `" + answer.class_name() +
                           "' does not match the answer:\n`" + answer_text +
                           "`\n vs\n`" + text + "`";
            throw std::invalid_argument(message);
        }
    }

    // properties
    const auto& props = answer.props();
    for (const auto& [name, text] : truth.props()) {
        auto it = props.find(name);
        if (it == props.end()) {
            auto message = std::string{"property `"} + name +
                           "' not found in compiled class `" +
                           answer.class_name() + "'";
            throw std::invalid_argument(message);
        }
        auto answer_text = it->second;
        if (answer_text != text) {
            auto message = std::string{"property `"} + name +
                           "' in compiled class `" + answer.class_name() +
                           "' does not match the answer:\n`" + answer_text +
                           "`\n vs\n`" + text + "`";
            throw std::invalid_argument(message);
        }
    }

    // test() fun text
    if (answer.test_fun() != truth.test_fun()) {
        auto message = std::string{"`test' function text of compiled class `"} +
                       answer.class_name() + "' does not match the answer:\n`" +
                       answer.test_fun() + "`\n vs\n`" + truth.test_fun() + "`";
        throw std::invalid_argument(message);
    }
}

std::ostream& operator<<(std::ostream& os, const Answer& answer) {
    os << answer.class_name() << " { ";
    for (const auto& [_, text] : answer.type_defs())
        os << text << " ";
    for (const auto& [_, text] : answer.props())
        os << text << " ";
    os << answer.test_fun();
    os << " }";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AnswerMap& answers) {
    for (const auto& [_, answer] : answers)
        os << answer << "\n";
    return os;
}
