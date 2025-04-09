#include "./answer.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>

bool Answer::has_prop(const std::string& name) const {
    return _props.count(name) == 1;
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

    auto skip_block = [&](char open, char close) {
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
            }
            ++pos;
        }
        if (pos == text.size())
            error("not closed", start);
    };

    auto skip_parens = std::bind(skip_block, '(', ')');
    auto skip_brackets = std::bind(skip_block, '[', ']');
    auto skip_braces = std::bind(skip_block, '{', '}');

    auto read_class_name = [&]() {
        if (text[pos] != 'U')
            error("class name must start with 'U'");
        auto start = pos++;
        while (is_ident())
            ++pos;
        return text.substr(start, pos - start);
    };

    auto skip_type_name = [&]() {
        // Type
        if (!is_upper())
            error("type name must start with uppercase letter");
        ++pos;
        while (is_ident())
            ++pos;
        // ()
        if (text[pos] == '(')
            skip_parens();
        if (text[pos] == '[')
            skip_brackets();
    };

    auto read_prop_name = [&]() {
        auto start = pos;
        if (!is_lower())
            error("prop name must start with lowercase letter");
        ++pos;
        while (is_ident())
            ++pos;
        return text.substr(start, pos - start);
    };

    // class name
    skip_spaces();
    answer.set_class_name(std::string{read_class_name()});

    // {
    skip_spaces();
    skip("{");

    constexpr char TestFunStart[] = "Int test()";
    constexpr char NoMain[] = "<NOMAIN>";

    // props
    skip_spaces();
    while (!at(TestFunStart) && !at(NoMain)) {
        auto start = pos;

        // Type(...)[...]
        skip_type_name();
        skip_spaces();

        // ident
        std::string name{read_prop_name()};
        skip_spaces();

        // value
        if (text[pos] == '(')
            skip_parens();

        // ;
        skip_spaces();
        skip(";");

        std::string prop_text{text.substr(start, pos - start)};
        answer.add_prop(std::move(name), std::move(prop_text));

        skip_spaces();
    }

    // `test` fun text
    if (at(TestFunStart)) {
        auto start = pos;

        // Int test()
        skip(TestFunStart);
        skip_spaces();

        // {...}
        skip_braces();

        std::string test_fun{text.substr(start, pos - start)};
        answer.set_test_fun(std::move(test_fun));

    } else {
        assert(at(NoMain));
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
                           "' does not match the answer: `" + answer_text +
                           "` vs `" + text + "`";
            throw std::invalid_argument(message);
        }
    }

    // test() fun text
    if (answer.test_fun() != truth.test_fun()) {
        auto message = std::string{"`test' function text of compiled class `"} +
                       answer.class_name() + "' does not match the answer:\n`" +
                       answer.test_fun() + "`\n vs \n`" + truth.test_fun() + "`";
        throw std::invalid_argument(message);
    }
}

std::ostream& operator<<(std::ostream& os, const Answer& answer) {
    os << answer.class_name() << " { ";
    for (const auto& [_, text] : answer.props())
        os << text << " ";
    os << answer.test_fun();
    os << " }";
    return os;
}
