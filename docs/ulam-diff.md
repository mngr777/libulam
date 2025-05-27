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

## Misc
* `&&` has higher priority than `||` (same priority in ULAM: `true || false && false` is `false`)
* `p ? a : b = c` is parsed as `p ? a : (b = c)`, not `(p ? a : b) = c`

## Status of misc. additional features

* Functions can return array values without typedefs.
* Functions can have default parameter values -- to be tested.
* Initializer list arguments -- would require changes in how overloads are selected.
* User defined type conversions -- currently only "toInt" is supported, but mostly a matter of just parsing conversion operators.
* Multidimensional arrays and initializer lists -- to be tested.
