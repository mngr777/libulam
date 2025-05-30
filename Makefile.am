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
	include/libulam/ast/nodes/args.hpp \
	include/libulam/ast/nodes/expr.hpp \
	include/libulam/ast/nodes/exprs.hpp \
	include/libulam/ast/nodes/init.hpp \
	include/libulam/ast/nodes/params.hpp \
	include/libulam/ast/nodes/root.hpp \
	include/libulam/ast/nodes/stmt.hpp \
	include/libulam/ast/nodes/stmts.hpp \
	include/libulam/ast/nodes/type.hpp \
	include/libulam/ast/nodes/var_decl.hpp \
	include/libulam/ast/nodes.inc.hpp

AST_SOURCE_FILES = \
	src/ast/node.cpp \
	src/ast/nodes/root.cpp \
	src/ast/visitor.cpp

SEMANTIC_HEADER_FILES = \
	include/libulam/semantic/decl.hpp \
	include/libulam/semantic/detail/integer.hpp \
	include/libulam/semantic/mangler.hpp \
	include/libulam/semantic/module.hpp \
	include/libulam/semantic/number.hpp \
	include/libulam/semantic/ops.hpp \
	include/libulam/semantic/ops.inc.hpp \
	include/libulam/semantic/program.hpp \
	include/libulam/semantic/scope.hpp \
	include/libulam/semantic/scope/context.hpp \
	include/libulam/semantic/scope/iterator.hpp \
	include/libulam/semantic/scope/stack.hpp \
	include/libulam/semantic/scope/version.hpp \
	include/libulam/semantic/scope/view.hpp \
	include/libulam/semantic/symbol.hpp \
	include/libulam/semantic/type/builtin/atom.hpp \
	include/libulam/semantic/type/builtin/bits.hpp \
	include/libulam/semantic/type/builtin/bool.hpp \
	include/libulam/semantic/type/builtin/fun.hpp \
	include/libulam/semantic/type/builtin/int.hpp \
	include/libulam/semantic/type/builtin/string.hpp \
	include/libulam/semantic/type/builtin/unary.hpp \
	include/libulam/semantic/type/builtin/unsigned.hpp \
	include/libulam/semantic/type/builtin/void.hpp \
	include/libulam/semantic/type/builtin_type_id.hpp \
	include/libulam/semantic/type/builtins.hpp \
	include/libulam/semantic/type/class.hpp \
	include/libulam/semantic/type/class_tpl.hpp \
	include/libulam/semantic/type/class/ancestry.hpp \
	include/libulam/semantic/type/class/base.hpp \
	include/libulam/semantic/type/class/prop.hpp \
	include/libulam/semantic/type/class_kind.hpp \
	include/libulam/semantic/type/conv.hpp \
	include/libulam/semantic/type/element.hpp \
	include/libulam/semantic/type/ops.hpp \
	include/libulam/semantic/type/prim.hpp \
	include/libulam/semantic/type.hpp \
	include/libulam/semantic/type_ops.hpp \
	include/libulam/semantic/type_ops.inc.hpp \
	include/libulam/semantic/type_tpl.hpp \
	include/libulam/semantic/value.hpp \
	include/libulam/semantic/value/bits.hpp \
	include/libulam/semantic/value/bound_fun_set.hpp \
	include/libulam/semantic/value/data.hpp \
	include/libulam/semantic/value/types.hpp \
	include/libulam/semantic/var.hpp \
	include/libulam/semantic/var/base.hpp

SEMANTIC_SOURCE_FILES = \
	src/semantic/decl.cpp \
	src/semantic/detail/leximited.hpp \
	src/semantic/detail/leximited.cpp \
	src/semantic/fun.cpp \
	src/semantic/number.cpp \
	src/semantic/type/builtin_type_id.cpp \
	src/semantic/type/class.cpp \
	src/semantic/type/class_tpl.cpp \
	src/semantic/type/class/ancestry.cpp \
	src/semantic/type/class/base.cpp \
	src/semantic/type/class/prop.cpp \
	src/semantic/type/class_kind.cpp \
	src/semantic/type/prim.cpp \
	src/semantic/mangler.cpp \
	src/semantic/module.cpp \
	src/semantic/ops.cpp \
	src/semantic/type_ops.cpp \
	src/semantic/program.cpp \
	src/semantic/scope.cpp \
	src/semantic/scope/context.cpp \
	src/semantic/scope/iterator.cpp \
	src/semantic/scope/stack.cpp \
	src/semantic/scope/view.cpp \
	src/semantic/type.cpp \
	src/semantic/type/builtin/atom.cpp \
	src/semantic/type/builtin/bits.cpp \
	src/semantic/type/builtin/bool.cpp \
	src/semantic/type/builtin/int.cpp \
	src/semantic/type/builtin/string.cpp \
	src/semantic/type/builtin/unary.cpp \
	src/semantic/type/builtin/unsigned.cpp \
	src/semantic/type/builtins.cpp \
	src/semantic/type/conv.cpp \
	src/semantic/type/element.cpp \
	src/semantic/type/ops.cpp \
	src/semantic/type_tpl.cpp \
	src/semantic/value.cpp \
	src/semantic/value/bits.cpp \
	src/semantic/value/bound_fun_set.cpp \
	src/semantic/value/data.cpp \
	src/semantic/var.cpp \
	src/semantic/var/base.cpp

SEMA_HEADER_FILES = \
	include/libulam/sema.hpp \
	include/libulam/sema/eval.hpp \
	include/libulam/sema/eval/cast.hpp \
	include/libulam/sema/eval/except.hpp \
	include/libulam/sema/eval/expr_visitor.hpp \
	include/libulam/sema/eval/flags.hpp \
	include/libulam/sema/eval/funcall.hpp \
	include/libulam/sema/eval/init.hpp \
	include/libulam/sema/eval/stack.hpp \
	include/libulam/sema/eval/visitor.hpp \
	include/libulam/sema/expr_error.hpp \
	include/libulam/sema/expr_res.hpp \
	include/libulam/sema/init.hpp \
	include/libulam/sema/resolver.hpp \
	include/libulam/sema/visitor.hpp

SEMA_SOURCE_FILES = \
	src/sema.cpp \
	src/sema/eval.cpp \
	src/sema/eval/cast.cpp \
	src/sema/eval/except.cpp \
	src/sema/eval/expr_visitor.cpp \
	src/sema/eval/funcall.cpp \
	src/sema/eval/init.cpp \
	src/sema/eval/stack.cpp \
	src/sema/eval/visitor.cpp \
	src/sema/expr_res.cpp \
	src/sema/out.hpp \
	src/sema/out.cpp \
	src/sema/init.cpp \
	src/sema/resolver.cpp \
	src/sema/visitor.cpp

HEADER_FILES = \
	$(AST_HEADER_FILES) \
	$(SEMANTIC_HEADER_FILES) \
	$(SEMA_HEADER_FILES) \
	include/libulam/context.hpp \
	include/libulam/detail/variant.hpp \
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
	include/libulam/token.hpp \
	include/libulam/types.hpp

SOURCE_FILES = \
	$(HEADER_FILES) \
	$(AST_SOURCE_FILES) \
	$(SEMANTIC_SOURCE_FILES) \
	$(SEMA_SOURCE_FILES) \
	src/debug.hpp \
	src/detail/string.hpp \
	src/detail/variant.hpp \
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
	test_semantic_bits \
	test_sema_basic \
	test_sema_color_utils \
	test_sema_simple_inheritance \
	test_sema_class_member \
	test_sema_expr \
	test_eval_virtual \
	test_ulam
check_PROGRAMS = $(TESTS)

TEST_LIBS = libulam.la

test_memory_notepad1_SOURCES = tests/memory/notepad1.cpp
test_memory_notepad1_LDADD = $(TEST_LIBS)

test_lex_basic_SOURCES = tests/lex/basic.cpp
test_lex_basic_LDADD = $(TEST_LIBS)

TEST_AST_SOURCE_FILES = \
	tests/ast/print.hpp \
	tests/ast/print.cpp

TEST_SEMA_SOURCE_FILES = \
	$(TEST_AST_SOURCE_FILES) \
	tests/sema/common.hpp \
	tests/sema/common.cpp

test_parser_expr_SOURCES = tests/parser/expr.cpp $(TEST_AST_SOURCE_FILES)
test_parser_expr_LDADD = $(TEST_LIBS)

test_semantic_bits_SOURCES = tests/semantic/bits.cpp
test_semantic_bits_LDADD = $(TEST_LIBS)

test_sema_basic_SOURCES = tests/sema/basic.cpp $(TEST_SEMA_SOURCE_FILES)
test_sema_basic_LDADD = $(TEST_LIBS)

test_sema_color_utils_SOURCES = tests/sema/color_utils.cpp $(TEST_SEMA_SOURCE_FILES)
test_sema_color_utils_LDADD = $(TEST_LIBS)

test_sema_simple_inheritance_SOURCES = tests/sema/simple_inheritance.cpp $(TEST_SEMA_SOURCE_FILES)
test_sema_simple_inheritance_LDADD = $(TEST_LIBS)

test_sema_class_member_SOURCES = tests/sema/class_member.cpp $(TEST_SEMA_SOURCE_FILES)
test_sema_class_member_LDADD = $(TEST_LIBS)

test_sema_expr_SOURCES = tests/sema/expr.cpp $(TEST_SEMA_SOURCE_FILES)
test_sema_expr_LDADD = $(TEST_LIBS)

test_eval_virtual_SOURCES = tests/eval/virtual.cpp $(TEST_SEMA_SOURCE_FILES)
test_eval_virtual_LDADD = $(TEST_LIBS)

test_ulam_SOURCES = \
	tests/ast/print.hpp \
	tests/ast/print.cpp \
	tests/ULAM/answer.hpp \
	tests/ULAM/answer.cpp \
	tests/ULAM/answer/parser.hpp \
	tests/ULAM/answer/parser.cpp \
	tests/ULAM/compiler.hpp \
	tests/ULAM/compiler.cpp \
	tests/ULAM/driver.cpp \
	tests/ULAM/eval.hpp \
	tests/ULAM/eval.cpp \
	tests/ULAM/eval/cast.hpp \
	tests/ULAM/eval/cast.cpp \
	tests/ULAM/eval/constext_stack.hpp \
	tests/ULAM/eval/expr_flags.hpp \
	tests/ULAM/eval/expr_res.hpp \
	tests/ULAM/eval/expr_res.cpp \
	tests/ULAM/eval/expr_visitor.hpp \
	tests/ULAM/eval/expr_visitor.cpp \
	tests/ULAM/eval/flags.hpp \
	tests/ULAM/eval/funcall.hpp \
	tests/ULAM/eval/funcall.cpp \
	tests/ULAM/eval/init.hpp \
	tests/ULAM/eval/init.cpp \
	tests/ULAM/eval/stringifier.hpp \
	tests/ULAM/eval/stringifier.cpp \
	tests/ULAM/eval/util.hpp \
	tests/ULAM/eval/visitor.hpp \
	tests/ULAM/eval/visitor.cpp \
	tests/ULAM/out.hpp \
	tests/ULAM/out.cpp \
	tests/ULAM/parser.hpp \
	tests/ULAM/parser.cpp \
	tests/ULAM/test_case.hpp \
	tests/ULAM/test_case.cpp \
	tests/ULAM/test_case/parser.hpp \
	tests/ULAM/test_case/parser.cpp
test_ulam_LDADD = $(TEST_LIBS)
