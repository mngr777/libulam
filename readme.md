## About
(This repo is work in progress.)

libulam is a frontend library for ULAM language.

ULAM is a programming language developed by Dave Ackley and Elena S. Ackley for Movable Feast Machine emulator.

## Current status

libulam interpreter can run about 90% of ULAM's `generic/safe` tests. A list of skipped tests can be found in tests/ULAM/driver.cpp:104. Some of these tests are TODO and some are skipped because of changes to he language (see docs/ulam-diff.md).

## Building

```
cd /path/to/repo
./autogen.sh
mkdir build
cd build
../configure
# with debug output:
# CXXFLAGS='-DDEBUG_INIT -DEBUG_RESOLVER -DDEBUG_EVAL -DDEBUG_EVAL_EXPR_VISITOR' ../configure
make -j4

# running tests:
export ULAM_PATH=/path/to/ulam
make check

# running specific ULAM test by name:
path/to/build/dir/test_ulam t3102

# by number:
path/to/build/dir/test_ulam 1
```
