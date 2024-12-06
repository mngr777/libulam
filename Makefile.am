AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++17 -I$(srcdir)/include -iquote $(srcdir)/include -iquote $(srcdir) -Wall -Wpedantic -Werror -O0 -g
CXXFLAGS = $(AM_CXXFLAGS) # temp hack to make libtool stop adding flags

AST_HEADER_FILES = \
	include/libulam/ast.hpp \
	include/libulam/ast/context.hpp \
	include/libulam/ast/expr_visitor.hpp \
	include/libulam/ast/node.hpp \
	include/libulam/ast/nodes/access.hpp \
	include/libulam/ast/nodes/expr.hpp \
	include/libulam/ast/nodes/params.hpp \
	include/libulam/ast/nodes/stmt.hpp \
	include/libulam/ast/nodes/stmts.hpp \
	include/libulam/ast/nodes/type.hpp \
	include/libulam/ast/nodes/var_decl.hpp \
	include/libulam/ast/nodes.inc.hpp

AST_SOURCE_FILES = \
	src/ast/node.cpp \
	src/ast/nodes/module.cpp \
	src/ast/visitor.cpp

SEMANTIC_HEADER_FILES = \
	include/libulam/semantic/module.hpp \
	include/libulam/semantic/number.hpp \
	include/libulam/semantic/ops.hpp \
	include/libulam/semantic/ops.inc.hpp \
	include/libulam/semantic/params.hpp \
	include/libulam/semantic/program.hpp \
	include/libulam/semantic/scope.hpp \
	include/libulam/semantic/scope/stack.hpp \
	include/libulam/semantic/scope/state.hpp \
	include/libulam/semantic/scope/object.hpp \
	include/libulam/semantic/scope/version.hpp \
	include/libulam/semantic/symbol.hpp \
	include/libulam/semantic/type/builtin.hpp \
	include/libulam/semantic/type/builtin/bits.hpp \
	include/libulam/semantic/type/builtin/bool.hpp \
	include/libulam/semantic/type/builtin/int.hpp \
	include/libulam/semantic/type/builtin/string.hpp \
	include/libulam/semantic/type/builtin/unary.hpp \
	include/libulam/semantic/type/builtin/unsigned.hpp \
	include/libulam/semantic/type/builtin/void.hpp \
	include/libulam/semantic/type/builtin_type_id.hpp \
	include/libulam/semantic/type/class.hpp \
	include/libulam/semantic/type/class_kind.hpp \
	include/libulam/semantic/type/prim.hpp \
	include/libulam/semantic/type.hpp \
	include/libulam/semantic/type_ops.hpp \
	include/libulam/semantic/type_tpl.hpp \
	include/libulam/semantic/value.hpp \
	include/libulam/semantic/value/bits.hpp \
	include/libulam/semantic/value/object.hpp \
	include/libulam/semantic/var.hpp

SEMANTIC_SOURCE_FILES = \
	src/semantic/detail/integer.hpp \
	src/semantic/fun.cpp \
	src/semantic/number.cpp \
	src/semantic/type/builtin_type_id.cpp \
	src/semantic/type/class.cpp \
	src/semantic/type/class_kind.cpp \
	src/semantic/type/prim.cpp \
	src/semantic/module.cpp \
	src/semantic/ops.cpp \
	src/semantic/program.cpp \
	src/semantic/scope.cpp \
	src/semantic/type.cpp \
	src/semantic/type/builtin/bits.cpp \
	src/semantic/type/builtin/bool.cpp \
	src/semantic/type/builtin/int.cpp \
	src/semantic/type/builtin/string.cpp \
	src/semantic/type/builtin/unary.cpp \
	src/semantic/type/builtin/unsigned.cpp \
	src/semantic/type_tpl.cpp \
	src/semantic/value.cpp

SEMA_HEADER_FILES = \
	include/libulam/sema.hpp \
	include/libulam/sema/eval.hpp \
	include/libulam/sema/expr_visitor.hpp \
	include/libulam/sema/resolve_deps.hpp \
	include/libulam/sema/resolver.hpp \
	include/libulam/sema/helper.hpp \
	include/libulam/sema/helper/param_eval.hpp \
	include/libulam/sema/helper/resolver.hpp \
	include/libulam/sema/visitor.hpp

SEMA_SOURCE_FILES = \
	src/sema.cpp \
	src/sema/eval.cpp \
	src/sema/expr_visitor.cpp \
	src/sema/out.hpp \
	src/sema/out.cpp \
	src/sema/resolve_deps.cpp \
	src/sema/resolver.cpp \
	src/sema/helper/param_eval.cpp \
	src/sema/helper/resolver.cpp \
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
