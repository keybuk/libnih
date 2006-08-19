## Process this file with automake to produce Makefile.in

DEFAULT_INCLUDES = -I$(top_builddir) -I$(top_srcdir)

localedir = $(datadir)/locale
DEFS += -DLOCALEDIR="\"$(localedir)\""
INCLUDES = -I$(top_srcdir)/intl


lib_LTLIBRARIES = libnih.la

libnih_la_SOURCES = \
	alloc.c \
	string.c \
	list.c \
	hash.c \
	timer.c \
	signal.c \
	child.c \
	io.c \
	main.c \
	option.c \
	logging.c \
	error.c


libnih_la_LDFLAGS = \
	-version-info 0:0:0
if HAVE_VERSION_SCRIPT_ARG
libnih_la_LDFLAGS += @VERSION_SCRIPT_ARG@=$(srcdir)/libnih.ver
endif

EXTRA_DIST = libnih.ver


include_HEADERS = \
	libnih.h

nihincludedir = $(includedir)/nih
nihinclude_HEADERS = \
	macros.h \
	alloc.h \
	string.h \
	list.h \
	hash.h \
	timer.h \
	signal.h \
	child.h \
	io.h \
	main.h \
	option.h \
	logging.h \
	error.h \
	errors.h


TESTS = \
	test_alloc \
	test_string \
	test_list \
	test_hash \
	test_timer \
	test_signal \
	test_child \
	test_io \
	test_main \
	test_option \
	test_logging \
	test_error

check_PROGRAMS = $(TESTS)

test_alloc_SOURCES = tests/test_alloc.c
test_alloc_LDFLAGS = -static
test_alloc_LDADD = libnih.la

test_string_SOURCES = tests/test_string.c
test_string_LDFLAGS = -static
test_string_LDADD = libnih.la

test_list_SOURCES = tests/test_list.c
test_list_LDFLAGS = -static
test_list_LDADD = libnih.la

test_hash_SOURCES = tests/test_hash.c
test_hash_LDFLAGS = -static
test_hash_LDADD = libnih.la

test_timer_SOURCES = tests/test_timer.c
test_timer_LDFLAGS = -static
test_timer_LDADD = libnih.la

test_signal_SOURCES = tests/test_signal.c
test_signal_LDFLAGS = -static
test_signal_LDADD = libnih.la

test_child_SOURCES = tests/test_child.c
test_child_LDFLAGS = -static
test_child_LDADD = libnih.la

test_io_SOURCES = tests/test_io.c
test_io_LDFLAGS = -static
test_io_LDADD = libnih.la

test_main_SOURCES = tests/test_main.c
test_main_LDFLAGS = -static
test_main_LDADD = libnih.la

test_option_SOURCES = tests/test_option.c
test_option_LDFLAGS = -static
test_option_LDADD = libnih.la

test_logging_SOURCES = tests/test_logging.c
test_logging_LDFLAGS = -static
test_logging_LDADD = libnih.la

test_error_SOURCES = tests/test_error.c
test_error_LDFLAGS = -static
test_error_LDADD = libnih.la


clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
