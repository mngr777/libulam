#pragma once

namespace ulam {

class Class {
public:
    enum Kind { Element, Quark, Transient };

    Class(Kind kind): _kind{kind} {}

    bool is(Kind kind) { return _kind == kind; }

    Kind kind() const { return _kind; }

private:
    Kind _kind;
};

}
