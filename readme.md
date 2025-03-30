## About
(This repo is work in progress.)

libulam is a frontend library for ULAM language.

[ULAM](https://github.com/elenasa/ULAM/) is a programming language developed by [Dave](https://www.cs.unm.edu/~ackley/) [Ackley](https://github.com/daveackley/) and [Elena](https://esa.ackleyshack.com/) S. [Ackley](https://github.com/elenasa) for [Movable Feast Machine](https://github.com/DaveAckley/MFM) emulator.

See [docs/overview.md](docs/overview.md) for some implementation details.

## Current status

libulam interpreter can run about 90% of ULAM's `generic/safe` tests. A list of skipped tests can be found in [tests/ULAM/driver.cpp](tests/ULAM/driver.cpp#L104). Some of these tests are TODO and some are skipped because of changes to the language (see [docs/ulam-diff.md](docs/ulam-diff.md)).

## Building
GNU autotools suite is used to build the library.
```
cd /path/to/repo
./autogen.sh
mkdir build
cd build
../configure
# with debug output:
# ../configure CPPFLAGS='-DDEBUG_INIT -DEBUG_RESOLVER -DDEBUG_EVAL -DDEBUG_EVAL_EXPR_VISITOR'
make -j4

# running tests:
export ULAM_PATH=/path/to/ulam
make check

# running specific ULAM test by name:
path/to/build/dir/test_ulam t3102

# by number:
path/to/build/dir/test_ulam 1
```
