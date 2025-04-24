#include "./answer.hpp"
#include "./answer/parser.hpp"
#include <cassert>
#include <iostream>
#include <stack>
#include <stdexcept>

namespace {

constexpr char TypeDef[] = "typedef";
constexpr char TestFunStart[] = "Int test()";
constexpr char NoMain[] = "<NOMAIN>";

} // namespace

// Answer

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

// AnswerBasePrefixStack

std::string AnswerBasePrefixStack::add_prefix(const std::string_view name) {
    return add_prefix(std::string{name});
}

std::string AnswerBasePrefixStack::add_prefix(std::string name) {
    return !_stack.empty() ? _stack.top() + name : std::move(name);
}

void AnswerBasePrefixStack::push(std::string name) {
    _stack.push(add_prefix(name) + '.');
}

void AnswerBasePrefixStack::pop() {
    assert(!_stack.empty());
    _stack.pop();
}

Answer parse_answer(const std::string_view text) {
    AnswerParser p{text};
    Answer answer;

    // class name
    p.skip_spaces();
    auto [name, is_tpl] = p.read_class_name();
    answer.set_is_tpl(is_tpl);
    answer.set_class_name(std::string{name});
    p.skip_spaces();

    // params (TODO: needed for templates)
    if (p.at('('))
        p.skip_parens();
    p.skip_spaces();

    // parents
    if (p.at(':')) {
        p.advance();
        while (true) {
            p.skip_spaces();
            auto [name, _] = p.read_parent_class_name();
            answer.add_parent(std::string{name});
            p.skip_spaces();
            if (!p.at('+'))
                break;
        }
        p.skip_spaces();
    }

    p.skip('{');
    p.skip_spaces();

    // non-fun members
    AnswerBasePrefixStack pref;
    while (!p.at(TestFunStart) && !p.at(NoMain) && !p.at('}')) {
        if (p.at("/*")) {
            p.skip_comment();

        } else if (p.at(':') || p.at('^')) {
            // :ParentType< ... > | ^GrandparentType< ... >
            bool is_grandarent = p.at('^');
            p.advance();
            auto [name, _] = p.read_parent_class_name();
            pref.push((is_grandarent ? "^" : "") + std::string{name});
            p.skip_spaces();
            p.skip('<');

        } else if (p.at('>')) {
            pref.pop();
            p.advance();
            p.skip_spaces();

        } else if (p.at(TypeDef)) {
            auto [alias, text] = p.read_type_def();
            answer.add_type_def(pref.add_prefix(alias), text);

        } else {
            // must be a constant/property
            auto [name, text, is_const] = p.read_data_mem();
            if (is_const) {
                answer.add_const(pref.add_prefix(name), text);
            } else {
                answer.add_prop(pref.add_prefix(name), text);
            }
        }
        p.skip_spaces();
    }

    // `test` fun text
    if (p.at(TestFunStart)) {
        auto start = p.pos();

        // Int test()
        p.skip(TestFunStart);
        p.skip_spaces();

        // {...}
        p.skip_braces();

        answer.set_test_fun(std::string{p.substr_from(start)});

    } else {
        if (p.at(NoMain))
            p.skip(NoMain);
        answer.set_test_fun(NoMain);
    }
    p.skip_spaces();
    p.skip('}');

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

    // parents
    {
        const auto& parents = answer.parents();
        auto size = parents.size();
        auto truth_size = truth.parents().size();
        if (size != truth_size) {
            auto message = std::string{"class `"} + answer.class_name() +
                           "' has " + std::to_string(size) +
                           " parents, correct number is " +
                           std::to_string(truth_size);
            throw std::invalid_argument(message);
        }

        auto it = parents.begin();
        auto truth_it = truth.parents().begin();
        unsigned n = 1;
        for (; truth_it != truth.parents().end(); ++truth_it, ++it) {
            assert(it != parents.end());
            assert(truth_it != truth.parents().end());
            if (*it != *truth_it) {
                auto message = std::string{
                    "class `" + answer.class_name() +
                    "': parent lists do not match (#" + std::to_string(n) +
                    " is `" + *it + "' vs `" + *truth_it + "')"};
                throw std::invalid_argument(message);
            }
        }
    }

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
            auto message = std::string{"constant or parameter `"} + name +
                           "' not found in compiled class `" +
                           answer.class_name() + "'";
            throw std::invalid_argument(message);
        }
        auto answer_text = it->second;
        if (answer_text != text) {
            auto message = std::string{"constant or parameter `"} + name +
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
