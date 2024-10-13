AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++17 -I$(srcdir)/include -iquote $(srcdir)/include -iquote $(srcdir) -Wall -Wpedantic -Werror -O0 -g
CXXFLAGS = $(AM_CXXFLAGS) # temp hack to make libtool stop adding flags

AST_HEADER_FILES = \
	include/libulam/ast.hpp \
	include/libulam/ast/context.hpp \
	include/libulam/ast/node.hpp \
	include/libulam/ast/nodes/expr.hpp \
	include/libulam/ast/nodes/params.hpp \
	include/libulam/ast/nodes.inc.hpp \
	include/libulam/ast/traversal.hpp

AST_SOURCE_FILES = \
	src/ast/node.cpp \
	src/ast/nodes/expr.cpp \
	src/ast/traversal.cpp

# Library
HEADER_FILES = \
	include/libulam/libulam.hpp \
	$(AST_HEADER_FILES)
	include/libulam/lang.hpp \
	include/libulam/lang/ops.hpp \
	include/libulam/lang/ops.inc.hpp \
	include/libulam/types.hpp \
	include/libulam/token.hpp

SOURCE_FILES = \
	$(HEADER_FILES) \
	$(AST_SOURCE_FILES) \
	src/context.hpp \
	src/context.cpp \
	src/debug.hpp \
	src/lang/ops.cpp \
	src/lexer.hpp \
	src/lexer.cpp \
	src/lexer/dfa.hpp \
	src/lexer/dfa.cpp \
	src/memory/notepad.hpp \
	src/memory/notepad.cpp \
	src/memory/str_buf.hpp \
	src/memory/str_buf.cpp \
	src/parser.hpp \
	src/parser.cpp \
	src/source.hpp \
	src/source.cpp \
	src/source/line_buf.hpp \
	src/source/line_buf.cpp \
	src/source/line_storage.hpp \
	src/source/line_storage.cpp \
	src/source_manager.hpp \
	src/source_manager.cpp \
	src/str_storage.hpp \
	src/str_storage.cpp \
	src/token.cpp

GENERATED_FILES = \
	src/gen/lexer/dfa.gen.cpp
CLEANFILES = $(GENERATED_FILES)
BUILT_SOURCES = $(GENERATED_FILES)

src/gen/lexer/dfa.gen.cpp: lexer_dfagen
	./lexer_dfagen$(EXEEXT) all > $(srcdir)/src/gen/lexer/dfa.gen.cpp

lib_LTLIBRARIES = libulam.la
libulam_ladir = $(includedir)
nobase_libulam_la_HEADERS = $(HEADER_FILES)
libulam_la_SOURCES = $(SOURCE_FILES)
libulam_CXXFLAGS = $(AM_CXXFLAGS) # -O3

# Tools
noinst_PROGRAMS= \
	lexer_dfagen

# Lexer DFA generator
lexer_dfagen_SOURCES = \
	tools/lexer/dfagen.cpp \
	tools/lexer/dfagen/graph.hpp \
	tools/lexer/dfagen/graph.cpp \
	include/libulam/token.hpp \
	include/libulam/token.inc.hpp \
	src/token.cpp \
	src/lexer/dfa.hpp
lexer_dfagen_CXXFLAGS = $(AM_CXXFLAGS) -ggdb -O0


TESTS = \
	test_memory_str_buf1 \
	test_memory_notepad1 \
	test_source_copy1 \
	test_source_str_buf_store1 \
	test_lexer_basic \
	test_parser_expr
check_PROGRAMS = $(TESTS)

# NOTE: setting CXXFLAGS for test additionally has side effect of
# building separate object files for tests, see
# https://www.gnu.org/software/automake/manual/html_node/Objects-created-both-with-libtool-and-without.html
TEST_CXXFLAGS = $(AM_CXXFLAGS) -O0 -g
TEST_SOURCE_FILES = \
	src/debug.hpp

AST_TEST_SOURCE_FILES = \
	tests/ast/print.hpp \
	tests/ast/print.cpp

# Memory tests
TEST_MEMORY_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_STR_BUF -D DEBUG_NOTEPAD
TEST_MEMORY_SOURCE_FILES = \
	$(TEST_SOURCE_FILES) \
	src/memory/notepad.hpp \
	src/memory/notepad.cpp \
	src/memory/str_buf.hpp \
	src/memory/str_buf.cpp
test_memory_str_buf1_SOURCES = tests/memory/str_buf1.cpp $(TEST_MEMORY_SOURCE_FILES)
test_memory_str_buf1_CXXFLAGS = $(TEST_MEMORY_CXXFLAGS)

test_memory_notepad1_SOURCES = tests/memory/notepad1.cpp $(TEST_MEMORY_SOURCE_FILES)
test_memory_notepad1_CXXFLAGS = $(TEST_MEMORY_CXXFLAGS)

# Source tests
TEST_SOURCE_SOURCE_FILES = \
	$(TEST_MEMORY_SOURCE_FILES) \
	src/source.hpp \
	src/source.cpp \
	src/source_manager.hpp \
	src/source_manager.cpp \
	src/source/line_buf.hpp \
	src/source/line_buf.cpp \
	src/source/line_storage.hpp \
	src/source/line_storage.cpp \
	src/str_storage.hpp \
	src/str_storage.cpp

test_source_copy1_SOURCES = tests/source/copy1.cpp $(TEST_SOURCE_SOURCE_FILES)
test_source_copy1_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_LINEBUF

test_source_str_buf_store1_SOURCES = tests/source/str_buf_store1.cpp $(TEST_SOURCE_SOURCE_FILES)
test_source_str_buf_store1_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_LINEBUF

# Lexer tests
TEST_LEXER_SOURCE_FILES = \
	$(TEST_SOURCE_SOURCE_FILES) \
	include/libulam/token.hpp \
	src/context.hpp \
	src/context.cpp \
	src/lexer.hpp \
	src/lexer.cpp \
	src/lexer/dfa.hpp \
	src/lexer/dfa.cpp \
	src/token.cpp

test_lexer_basic_SOURCES = tests/lexer/basic.cpp $(TEST_LEXER_SOURCE_FILES)
test_lexer_basic_CXXFLAGS = $(TEST_CXXFLAGS)

# Parser tests
TEST_PARSER_SOURCE_FILES = \
	$(TEST_LEXER_SOURCE_FILES) \
	$(AST_HEADER_FILES) \
	$(AST_SOURCE_FILES) \
	$(AST_TEST_SOURCE_FILES) \
	include/libulam/lang/ops.hpp \
	include/libulam/lang/ops.inc.hpp \
	src/lang/ops.cpp \
	src/parser.hpp \
	src/parser.cpp

test_parser_expr_SOURCES = tests/parser/expr.cpp $(TEST_PARSER_SOURCE_FILES)
test_parser_expr_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_PARSER
