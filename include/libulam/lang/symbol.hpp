#pragma once
#include <string>

namespace ulam {

class Tpl {};

class Type {};

class Var {};

class Fun {};

class Symbol {
public:
    enum Kind { Tpl, Type, Fun, Var };

    Symbol(std::string name) {}

    const std::string& name() const { return _name; }
    Kind metatype() const { return _kind; }

private:
    std::string _name;
    Kind _kind;
};

} // namespace ulam
