AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++17 -I$(srcdir)/include -iquote $(srcdir)/include -iquote $(srcdir) -Wall -Wpedantic -Werror -O0 -g
CXXFLAGS = $(AM_CXXFLAGS) # temp hack to make libtool stop adding flags

AST_HEADER_FILES = \
	include/libulam/ast.hpp \
	include/libulam/ast/context.hpp \
	include/libulam/ast/node.hpp \
	include/libulam/ast/nodes/access.hpp \
	include/libulam/ast/nodes/expr.hpp \
	include/libulam/ast/nodes/params.hpp \
	include/libulam/ast/nodes/stmt.hpp \
	include/libulam/ast/nodes/stmts.hpp \
	include/libulam/ast/nodes.inc.hpp \
	include/libulam/ast/traversal.hpp

AST_SOURCE_FILES = \
	src/ast/node.cpp \
	src/ast/nodes/expr.cpp \
	src/ast/traversal.cpp

LANG_HEADER_FILES = \
	include/libulam/lang.hpp \
	include/libulam/lang/class.hpp \
	include/libulam/lang/ops.hpp \
	include/libulam/lang/ops.inc.hpp \
	include/libulam/lang/symbol.hpp \
	include/libulam/lang/type_ops.hpp

LANG_SOURCE_FILES = \
	src/lang/ops.cpp \
	src/lang/symbol.cpp

# Library
HEADER_FILES = \
	$(AST_HEADER_FILES) \
	$(LANG_HEADER_FILES) \
	include/libulam/memory/buf.hpp \
	include/libulam/memory/notepad.hpp \
	include/libulam/parser.hpp \
	include/libulam/preproc.hpp \
	include/libulam/src.hpp \
	include/libulam/src_loc.hpp \
	include/libulam/src_mngr.hpp \
	include/libulam/str_pool.hpp \
	include/libulam/token.hpp

SOURCE_FILES = \
	$(HEADER_FILES) \
	$(AST_SOURCE_FILES) \
	$(LANG_SOURCE_FILES) \
	src/debug.hpp \
	src/detail/str.hpp \
	src/memory/notepad.cpp \
	src/memory/buf.cpp \
	src/parser.cpp \
	src/parser/number.hpp \
	src/parser/number.cpp \
	src/parser/string.hpp \
	src/parser/string.cpp \
	src/preproc.cpp \
	src/src.cpp \
	src/src_loc.cpp \
	src/src_mngr.cpp \
	src/str_pool.cpp \
	src/token.cpp

lib_LTLIBRARIES = libulam.la
libulam_ladir = $(includedir)
nobase_libulam_la_HEADERS = $(HEADER_FILES)
libulam_la_SOURCES = $(SOURCE_FILES)
libulam_CXXFLAGS = $(AM_CXXFLAGS) # -O3

TESTS = \
	test_memory_notepad1 \
	test_parser_expr \
	test_lex_basic
check_PROGRAMS = $(TESTS)

# NOTE: setting CXXFLAGS for test additionally has side effect of
# building separate object files for tests, see
# https://www.gnu.org/software/automake/manual/html_node/Objects-created-both-with-libtool-and-without.html

# Memory tests
TEST_MEMORY_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_NOTEPAD
TEST_MEMORY_SOURCE_FILES = \
	src/debug.hpp \
	libulam/include/memory/notepad.hpp \
	src/memory/notepad.cpp

test_memory_notepad1_SOURCES = tests/memory/notepad1.cpp $(TEST_MEMORY_SOURCE_FILES)
test_memory_notepad1_CXXFLAGS = $(TEST_MEMORY_CXXFLAGS)

# TODO: source tests

TEST_LEX_SOURCE_FILES = \
	$(TEST_MEMORY_SOURCE_FILES) \
	include/libulam/token.hpp \
	include/libulam/lex.hpp \
	include/libulam/preproc.hpp \
	include/libulam/src.hpp \
	include/libulam/src_mngr.hpp \
	src/detail/str.hpp \
	src/lex.cpp \
	src/preproc.cpp \
	src/src.cpp \
	src/src_mngr.cpp \
	src/token.cpp
test_lex_basic_SOURCES = tests/lex/basic.cpp $(TEST_LEX_SOURCE_FILES)
test_lex_basic_CXXFLAGS = $(TEST_CXXFLAGS)


# Parser tests
TEST_PARSER_SOURCE_FILES = \
	$(TEST_LEX_SOURCE_FILES) \
	$(AST_HEADER_FILES) \
	$(AST_SOURCE_FILES) \
	$(LANG_HEADER_FILES) \
	$(LANG_SOURCE_FILES) \
	$(AST_TEST_SOURCE_FILES) \
	include/libulam/parser.hpp \
	src/parser.cpp \
	src/parser/number.hpp \
	src/parser/number.cpp \
	src/parser/string.hpp \
	src/parser/string.cpp \
	tests/ast/print.hpp \
	tests/ast/print.cpp

test_parser_expr_SOURCES = tests/parser/expr.cpp $(TEST_PARSER_SOURCE_FILES)
test_parser_expr_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_PARSER
