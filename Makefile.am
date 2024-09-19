AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++17 -iquote $(srcdir)/include -iquote $(srcdir) -Wall -Wpedantic -Werror
CXXFLAGS = $(AM_CXXFLAGS) # temp hack to make libtool stop adding flags

HEADER_FILES = \
	include/libulam/libulam.hpp \
	include/libulam/types.hpp \
	include/libulam/token.hpp

SOURCE_FILES = \
	src/context.hpp \
	src/context.cpp \
	src/debug.hpp \
	src/lexer.hpp \
	src/lexer.cpp \
	src/memory/notepad.hpp \
	src/memory/notepad.cpp \
	src/memory/str_buf.hpp \
	src/memory/str_buf.cpp \
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

lib_LTLIBRARIES = libulam.la
libulam_ladir = $(includedir)
nobase_libulam_la_HEADERS = $(HEADER_FILES)
libulam_la_SOURCES = $(SOURCE_FILES)
libulam_CXXFLAGS = $(AM_CXXFLAGS) # -O3

TESTS = \
	test_memory_str_buf1 \
	test_memory_notepad1 \
	test_source_copy1 \
	test_source_str_buf_store1
check_PROGRAMS = $(TESTS)

# NOTE: setting CXXFLAGS for test additionally has side effect of
# building separate object files for tests, see
# https://www.gnu.org/software/automake/manual/html_node/Objects-created-both-with-libtool-and-without.html
TEST_CXXFLAGS = $(AM_CXXFLAGS) -O0 -g
TEST_SOURCES = \
	src/debug.hpp

# Memory tests
TEST_MEMORY_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_STR_BUF -D DEBUG_NOTEPAD
TEST_MEMORY_SOURCES = \
	$(TEST_SOURCES) \
	src/memory/notepad.hpp \
	src/memory/notepad.cpp \
	src/memory/str_buf.hpp \
	src/memory/str_buf.cpp

test_memory_str_buf1_SOURCES = tests/memory/str_buf1.cpp $(TEST_MEMORY_SOURCES)
test_memory_str_buf1_CXXFLAGS = $(TEST_MEMORY_CXXFLAGS)

test_memory_notepad1_SOURCES = tests/memory/notepad1.cpp $(TEST_MEMORY_SOURCES)
test_memory_notepad1_CXXFLAGS = $(TEST_MEMORY_CXXFLAGS)

# Source tests
TEST_SOURCE_SOURCES = \
	$(TEST_SOURCES) \
	src/memory/notepad.hpp \
	src/memory/notepad.cpp \
	src/memory/str_buf.hpp \
	src/memory/str_buf.cpp \
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

test_source_copy1_SOURCES = tests/source/copy1.cpp $(TEST_SOURCE_SOURCES)
test_source_copy1_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_LINEBUF

test_source_str_buf_store1_SOURCES = tests/source/str_buf_store1.cpp $(TEST_SOURCE_SOURCES)
test_source_str_buf_store1_CXXFLAGS = $(TEST_CXXFLAGS) -D DEBUG_LINEBUF
