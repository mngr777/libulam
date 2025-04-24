#pragma once
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <stack>

class Answer {
public:
    bool is_tpl() const { return _is_tpl; }
    void set_is_tpl(bool is_tpl) { _is_tpl = is_tpl; }

    const std::string& class_name() const { return _class_name; }
    void set_class_name(std::string name) { _class_name = std::move(name); }

    const std::list<std::string>& parents() const { return _parents; }
    void add_parent(std::string parent) {
        _parents.push_back(std::move(parent));
    }

    const std::string& test_fun() const { return _test_fun; }
    void set_test_fun(std::string text) { _test_fun = std::move(text); }

    bool has_type_def(const std::string& alias) const;
    std::string type_def(const std::string& alias) const;
    void add_type_def(std::string alias, std::string text);

    bool has_const(const std::string& name) const;
    std::string const_(const std::string& name) const;
    void add_const(std::string name, std::string text);

    bool has_prop(const std::string& name) const;
    std::string prop(const std::string& name) const;
    void add_prop(std::string name, std::string text);

    const auto& props() const { return _props; }
    const auto& consts() const { return _consts; }
    const auto& type_defs() const { return _type_defs; }

private:
    bool _is_tpl{false};
    std::string _class_name;
    std::list<std::string> _parents;
    std::string _test_fun;
    std::map<std::string, std::string> _type_defs;
    std::map<std::string, std::string> _consts;
    std::map<std::string, std::string> _props;
};

using AnswerMap = std::map<std::string, Answer>;

class AnswerBasePrefixStack {
public:
    std::string add_prefix(std::string name);

    void push(std::string name);
    void pop();

private:
    std::stack<std::string> _stack;
};

Answer parse_answer(const std::string_view text);

void compare_answer_maps(
    const AnswerMap& true_answers, const AnswerMap& answers);
void compare_answers(const Answer& truth, const Answer& answer);

std::ostream& operator<<(std::ostream& os, const Answer& answer);
std::ostream& operator<<(std::ostream& os, const AnswerMap& answers);
