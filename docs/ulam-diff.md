# Differences with original ULAM

## Order of definitions matters

Alias defined after use:
```
constant A cA = 1;
typedef Int A;
```

Class typedef used in parent list:
```
quark A(Int cA) {}

quark B : MyA {
  typedef A(1) MyA;
}

// fix:
local typedef A(1) MyA;
quark B : MyA {
  typedef MyA MyA;
}
```

Template parameter use parameter defined later:
```
quark A(Int cP = cR, Int cR = 1) {}

// fix:
quark A{Int cR = 1, Int cP = cR} {}
```

## Quarks are not directly convertible to Atoms
```
quark C { Int c; }

element B : C {
  Void test() {
    C b;
    B& c = (C&)b;

    // illegal conversions
    Atom a = c;
    Atom& aref = c;

    // fix
    Atom a = (A)c;
    Atom& aref = (A&)c;
  }
}
```

## Status of misc. additional features

* Functions can return array values without typedefs.
* Functions can have default parameter values -- to be tested.
* Initializer list arguments -- would require changes in how overloads are selected.
* User defined type conversions -- currently only "toInt" is supported, but mostly a matter of just parsing conversion operators.
* Multidimensional arrays and initializer lists -- to be tested.
