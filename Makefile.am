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
	src/ast/nodes/module.cpp \
	src/ast/traversal.cpp \
	src/ast/visitor.cpp

SEMANTIC_HEADER_FILES = \
	include/libulam/semantic/module.hpp \
	include/libulam/semantic/ops.hpp \
	include/libulam/semantic/ops.inc.hpp \
	include/libulam/semantic/params.hpp \
	include/libulam/semantic/program.hpp \
	include/libulam/semantic/scope.hpp \
	include/libulam/semantic/symbol.hpp \
	include/libulam/semantic/type.hpp \
	include/libulam/semantic/type_ops.hpp \
	include/libulam/semantic/var.hpp

SEMANTIC_SOURCE_FILES = \
	src/semantic/module.cpp \
	src/semantic/ops.cpp \
	src/semantic/program.cpp \
	src/semantic/symbol.cpp \
	src/semantic/type.cpp

SEMA_HEADER_FILES = \
	include/libulam/sema.hpp \
	include/libulam/sema/type_init.hpp \
	include/libulam/sema/visitor.hpp

SEMA_SOURCE_FILES = \
	src/sema.cpp \
	src/sema/type_init.cpp \
	src/sema/visitor.cpp

# Library
HEADER_FILES = \
	$(AST_HEADER_FILES) \
	$(SEMANTIC_HEADER_FILES) \
	$(SEMA_HEADER_FILES) \
	include/libulam/context.hpp \
	include/libulam/diag.hpp \
	include/libulam/lex.hpp \
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
	$(SEMANTIC_SOURCE_FILES) \
	$(SEMA_SOURCE_FILES) \
	src/debug.hpp \
	src/detail/string.hpp \
	src/diag.cpp \
	src/lex.cpp \
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
	test_lex_basic \
	test_parser_expr \
	test_sema_basic
check_PROGRAMS = $(TESTS)

TEST_LIBS = libulam.la

test_memory_notepad1_SOURCES = tests/memory/notepad1.cpp
test_memory_notepad1_LDADD = $(TEST_LIBS)

test_lex_basic_SOURCES = tests/lex/basic.cpp
test_lex_basic_LDADD = $(TEST_LIBS)

TEST_AST_SOURCE_FILES = \
	tests/ast/print.hpp \
	tests/ast/print.cpp

test_parser_expr_SOURCES = tests/parser/expr.cpp $(TEST_AST_SOURCE_FILES)
test_parser_expr_LDADD = $(TEST_LIBS)

test_sema_basic_SOURCES = tests/sema/basic.cpp $(TEST_AST_SOURCE_FILES)
test_sema_basic_LDADD = $(TEST_LIBS)
