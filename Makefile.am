AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++17 -I$(srcdir) -iquote $(srcdir) -Wall -Wpedantic -Werror -O2
#CXXFLAGS = $(AM_CXXFLAGS) # temp(?) hack to make libtool stop adding flags

AST_HEADER_FILES = \
	libulam/ast.hpp \
	libulam/ast/context.hpp \
	libulam/ast/expr_visitor.hpp \
	libulam/ast/node.hpp \
	libulam/ast/nodes/access.hpp \
	libulam/ast/nodes/args.hpp \
	libulam/ast/nodes/expr.hpp \
	libulam/ast/nodes/exprs.hpp \
	libulam/ast/nodes/init.hpp \
	libulam/ast/nodes/module.hpp \
	libulam/ast/nodes/params.hpp \
	libulam/ast/nodes/root.hpp \
	libulam/ast/nodes/stmt.hpp \
	libulam/ast/nodes/stmts.hpp \
	libulam/ast/nodes/type.hpp \
	libulam/ast/nodes/var_def.hpp \
	libulam/ast/nodes.hpp \
	libulam/ast/nodes.inc.hpp \
	libulam/ast/str.hpp \
	libulam/ast/visitor.hpp

AST_SOURCE_FILES = \
	src/ast/expr_visitor.cpp \
	src/ast/node.cpp \
	src/ast/nodes/module.cpp \
	src/ast/nodes/root.cpp \
	src/ast/visitor.cpp

SEMANTIC_HEADER_FILES = \
	libulam/semantic/def.hpp \
	libulam/semantic/export.hpp \
	libulam/semantic/fun.hpp \
	libulam/semantic/mangler.hpp \
	libulam/semantic/module.hpp \
	libulam/semantic/number.hpp \
	libulam/semantic/ops.hpp \
	libulam/semantic/ops.inc.hpp \
	libulam/semantic/prim_types.inc.hpp \
	libulam/semantic/program.hpp \
	libulam/semantic/scope.hpp \
	libulam/semantic/scope/as_cond.hpp \
	libulam/semantic/scope/class.hpp \
	libulam/semantic/scope/flags.hpp \
	libulam/semantic/scope/fun.hpp \
	libulam/semantic/scope/iter.hpp \
	libulam/semantic/scope/module.hpp \
	libulam/semantic/scope/options.hpp \
	libulam/semantic/scope/program.hpp \
	libulam/semantic/scope/stack.hpp \
	libulam/semantic/scope/version.hpp \
	libulam/semantic/scope/view.hpp \
	libulam/semantic/symbol.hpp \
	libulam/semantic/type/builtin_type_id.hpp \
	libulam/semantic/type/builtin/atom.hpp \
	libulam/semantic/type/builtin/bits.hpp \
	libulam/semantic/type/builtin/bool.hpp \
	libulam/semantic/type/builtin/fun.hpp \
	libulam/semantic/type/builtin/int.hpp \
	libulam/semantic/type/builtin/string.hpp \
	libulam/semantic/type/builtin/unary.hpp \
	libulam/semantic/type/builtin/unsigned.hpp \
	libulam/semantic/type/builtin/void.hpp \
	libulam/semantic/type/builtins.hpp \
	libulam/semantic/type/class.hpp \
	libulam/semantic/type/class_tpl.hpp \
	libulam/semantic/type/class/ancestry.hpp \
	libulam/semantic/type/class/base.hpp \
	libulam/semantic/type/class/options.hpp \
	libulam/semantic/type/class/prop.hpp \
	libulam/semantic/type/class/registry.hpp \
	libulam/semantic/type/class_kind.hpp \
	libulam/semantic/type/class_name_kind.hpp \
	libulam/semantic/type/conv.hpp \
	libulam/semantic/type/element.hpp \
	libulam/semantic/type/ops.hpp \
	libulam/semantic/type/prim.hpp \
	libulam/semantic/type/visitor.hpp \
	libulam/semantic/type.hpp \
	libulam/semantic/types.inc.hpp \
	libulam/semantic/typed_value.hpp \
	libulam/semantic/type_ops.hpp \
	libulam/semantic/type_ops.inc.hpp \
	libulam/semantic/type_tpl.hpp \
	libulam/semantic/utils/class_name.hpp \
	libulam/semantic/utils/strf.hpp \
	libulam/semantic/value.hpp \
	libulam/semantic/value/bits.hpp \
	libulam/semantic/value/bound_fun_set.hpp \
	libulam/semantic/value/flags.hpp \
	libulam/semantic/value/data.hpp \
	libulam/semantic/value/types.hpp \
	libulam/semantic/var.hpp \
	libulam/semantic/var/base.hpp

SEMANTIC_SOURCE_FILES = \
	src/semantic/def.cpp \
	src/semantic/export.cpp \
	src/semantic/fun.cpp \
	src/semantic/number.cpp \
	src/semantic/type/prim.cpp \
	src/semantic/mangler.cpp \
	src/semantic/module.cpp \
	src/semantic/ops.cpp \
	src/semantic/type_ops.cpp \
	src/semantic/program.cpp \
	src/semantic/scope.cpp \
	src/semantic/scope/iter.cpp \
	src/semantic/scope/stack.cpp \
	src/semantic/scope/view.cpp \
	src/semantic/type.cpp \
	src/semantic/type/builtin_type_id.cpp \
	src/semantic/type/builtin/atom.cpp \
	src/semantic/type/builtin/bits.cpp \
	src/semantic/type/builtin/bool.cpp \
	src/semantic/type/builtin/fun.cpp \
	src/semantic/type/builtin/int.cpp \
	src/semantic/type/builtin/string.cpp \
	src/semantic/type/builtin/unary.cpp \
	src/semantic/type/builtin/unsigned.cpp \
	src/semantic/type/builtin/void.cpp \
	src/semantic/type/builtins.cpp \
	src/semantic/type/class.cpp \
	src/semantic/type/class_tpl.cpp \
	src/semantic/type/class/ancestry.cpp \
	src/semantic/type/class/base.cpp \
	src/semantic/type/class/prop.cpp \
	src/semantic/type/class/registry.cpp \
	src/semantic/type/class_kind.cpp \
	src/semantic/type/conv.cpp \
	src/semantic/type/element.cpp \
	src/semantic/type/ops.cpp \
	src/semantic/type/visitor.cpp \
	src/semantic/type_tpl.cpp \
	src/semantic/utils/class_name.cpp \
	src/semantic/utils/strf.cpp \
	src/semantic/value.cpp \
	src/semantic/value/bits.cpp \
	src/semantic/value/bound_fun_set.cpp \
	src/semantic/value/data.cpp \
	src/semantic/var.cpp \
	src/semantic/var/base.cpp

SEMA_HEADER_FILES = \
	libulam/sema.hpp \
	libulam/sema/debug/out.hpp \
	libulam/sema/eval.hpp \
	libulam/sema/eval/base.hpp \
	libulam/sema/eval/cast.hpp \
	libulam/sema/eval/cond.hpp \
	libulam/sema/eval/cond_res.hpp \
	libulam/sema/eval/env.hpp \
	libulam/sema/eval/except.hpp \
	libulam/sema/eval/expr_visitor.hpp \
	libulam/sema/eval/flags.hpp \
	libulam/sema/eval/funcall.hpp \
	libulam/sema/eval/helper.hpp \
	libulam/sema/eval/init.hpp \
	libulam/sema/eval/options.hpp \
	libulam/sema/eval/stack.hpp \
	libulam/sema/eval/visitor.hpp \
	libulam/sema/eval/which.hpp \
	libulam/sema/expr_error.hpp \
	libulam/sema/expr_res.hpp \
	libulam/sema/init.hpp \
	libulam/sema/resolver.hpp \
	libulam/sema/resolver/class.hpp \
	libulam/sema/visitor.hpp

SEMA_SOURCE_FILES = \
	src/sema.cpp \
	src/sema/eval.cpp \
	src/sema/eval/base.cpp \
	src/sema/eval/cast.cpp \
	src/sema/eval/cond.cpp \
	src/sema/eval/env.cpp \
	src/sema/eval/except.cpp \
	src/sema/eval/expr_visitor.cpp \
	src/sema/eval/funcall.cpp \
	src/sema/eval/helper.cpp \
	src/sema/eval/init.cpp \
	src/sema/eval/stack.cpp \
	src/sema/eval/visitor.cpp \
	src/sema/eval/which.cpp \
	src/sema/expr_res.cpp \
	src/sema/debug/out.cpp \
	src/sema/init.cpp \
	src/sema/resolver.cpp \
	src/sema/resolver/class.cpp \
	src/sema/visitor.cpp

META_HEADER_FILES = \
	libulam/meta.hpp \
	libulam/meta/field.hpp \
	libulam/meta/fields.inc.hpp \
	libulam/meta/id.hpp \
	libulam/meta/type.hpp

META_SOURCE_FILES = \
	src/meta.cpp \
	src/meta/field.cpp \
	src/meta/parser.hpp \
	src/meta/parser.cpp

HEADER_FILES = \
	$(AST_HEADER_FILES) \
	$(SEMANTIC_HEADER_FILES) \
	$(SEMA_HEADER_FILES) \
	$(META_HEADER_FILES) \
	libulam/assert.hpp \
	libulam/context.hpp \
	libulam/detail/variant.hpp \
	libulam/diag.hpp \
	libulam/memory/buf.hpp \
	libulam/memory/notepad.hpp \
	libulam/memory/ptr.hpp \
	libulam/options.hpp \
	libulam/parser.hpp \
	libulam/parser/options.hpp \
	libulam/parser/lex.hpp \
	libulam/parser/preproc.hpp \
	libulam/parser/token.hpp \
	libulam/parser/token.inc.hpp \
	libulam/src.hpp \
	libulam/src_loc.hpp \
	libulam/src_man.hpp \
	libulam/str_pool.hpp \
	libulam/types.hpp \
	libulam/utils/integer.hpp \
	libulam/utils/file.hpp \
	libulam/utils/leximited.hpp

SOURCE_FILES = \
	$(HEADER_FILES) \
	$(AST_SOURCE_FILES) \
	$(SEMANTIC_SOURCE_FILES) \
	$(SEMA_SOURCE_FILES) \
	$(META_SOURCE_FILES) \
	src/debug.hpp \
	src/detail/string.hpp \
	src/detail/variant.hpp \
	src/diag.cpp \
	src/memory/notepad.cpp \
	src/memory/buf.cpp \
	src/parser.cpp \
	src/parser/lex.cpp \
	src/parser/number.hpp \
	src/parser/number.cpp \
	src/parser/preproc.cpp \
	src/parser/string.hpp \
	src/parser/string.cpp \
	src/parser/token.cpp \
	src/src.cpp \
	src/src_loc.cpp \
	src/src_man.cpp \
	src/str_pool.cpp \
	src/utils/file.cpp \
	src/utils/leximited.cpp

lib_LTLIBRARIES = libulam.la
libulam_ladir = $(includedir)
nobase_libulam_la_HEADERS = $(HEADER_FILES)
libulam_la_SOURCES = $(SOURCE_FILES)

TESTS = \
	test_memory_notepad1 \
	test_lex_basic \
	test_parser_expr \
	test_parser_init_list1 \
	test_parser_cast1 \
	test_semantic_bits \
	test_sema_basic \
	test_sema_color_utils \
	test_sema_simple_inheritance \
	test_sema_class_member \
	test_sema_expr \
	test_eval_virtual \
	test_meta_parse1 \
	test_meta_class1 \
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

test_parser_init_list1_SOURCES = tests/parser/init_list1.cpp $(TEST_AST_SOURCE_FILES)
test_parser_init_list1_LDADD = $(TEST_LIBS)

test_parser_cast1_SOURCES = tests/parser/cast1.cpp $(TEST_AST_SOURCE_FILES)
test_parser_cast1_LDADD = $(TEST_LIBS)

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

test_meta_parse1_SOURCES = tests/meta/parse1.cpp
test_meta_parse1_LDADD = $(TEST_LIBS)

test_meta_class1_SOURCES = tests/meta/class1.cpp
test_meta_class1_LDADD = $(TEST_LIBS)

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
	tests/ULAM/eval/codegen.hpp \
	tests/ULAM/eval/codegen.cpp \
	tests/ULAM/eval/codegen/context_stack.hpp \
	tests/ULAM/eval/codegen/context_stack.cpp \
	tests/ULAM/eval/cond.hpp \
	tests/ULAM/eval/cond.cpp \
	tests/ULAM/eval/env.hpp \
	tests/ULAM/eval/env.cpp \
	tests/ULAM/eval/expr_flags.hpp \
	tests/ULAM/eval/expr_res.hpp \
	tests/ULAM/eval/expr_res.cpp \
	tests/ULAM/eval/expr_visitor.hpp \
	tests/ULAM/eval/expr_visitor.cpp \
	tests/ULAM/eval/flags.hpp \
	tests/ULAM/eval/funcall.hpp \
	tests/ULAM/eval/funcall.cpp \
	tests/ULAM/eval/helper.hpp \
	tests/ULAM/eval/helper.cpp \
	tests/ULAM/eval/init.hpp \
	tests/ULAM/eval/init.cpp \
	tests/ULAM/eval/native.hpp \
	tests/ULAM/eval/native.cpp \
	tests/ULAM/eval/stringifier.hpp \
	tests/ULAM/eval/stringifier.cpp \
	tests/ULAM/eval/test_context.hpp \
	tests/ULAM/eval/test_context.cpp \
	tests/ULAM/eval/util.hpp \
	tests/ULAM/eval/visitor.hpp \
	tests/ULAM/eval/visitor.cpp \
	tests/ULAM/eval/which.hpp \
	tests/ULAM/eval/which.cpp \
	tests/ULAM/out.hpp \
	tests/ULAM/out.cpp \
	tests/ULAM/parser.hpp \
	tests/ULAM/parser.cpp \
	tests/ULAM/test_case.hpp \
	tests/ULAM/test_case.cpp \
	tests/ULAM/test_case/parser.hpp \
	tests/ULAM/test_case/parser.cpp \
	tests/ULAM/utils.hpp \
	tests/ULAM/utils.cpp
test_ulam_LDADD = $(TEST_LIBS)
