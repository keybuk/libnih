## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DLOCALEDIR="\"$(localedir)\"" \
	-I$(top_builddir) -I$(top_srcdir) \
	-I$(top_srcdir)/intl


lib_LTLIBRARIES = libnih.la

libnih_la_SOURCES = \
	alloc.c \
	string.c \
	list.c \
	hash.c \
	tree.c \
	timer.c \
	signal.c \
	child.c \
	io.c \
	file.c \
	watch.c \
	main.c \
	option.c \
	command.c \
	config.c \
	logging.c \
	error.c

libnih_la_LDFLAGS = \
	-version-info 1:0:0
if HAVE_VERSION_SCRIPT_ARG
libnih_la_LDFLAGS += @VERSION_SCRIPT_ARG@=$(srcdir)/libnih.ver
endif

libnih_la_LIBADD = -lrt


include_HEADERS = \
	libnih.h

nihincludedir = $(includedir)/nih
nihinclude_HEADERS = \
	macros.h \
	alloc.h \
	string.h \
	list.h \
	hash.h \
	tree.h \
	timer.h \
	signal.h \
	child.h \
	io.h \
	file.h \
	watch.h \
	main.h \
	option.h \
	command.h \
	config.h \
	logging.h \
	error.h \
	errors.h \
	test.h \
	test_output.h \
	test_values.h \
	test_process.h \
	test_divert.h \
	test_files.h \
	test_alloc.h \
	test_list.h \
	test_hash.h


pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = libnih.pc


EXTRA_DIST = libnih.ver libnih.supp libnih.pc.in


LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TESTS = \
	test_alloc \
	test_string \
	test_list \
	test_hash \
	test_tree \
	test_timer \
	test_signal \
	test_child \
	test_io \
	test_file \
	test_watch \
	test_main \
	test_option \
	test_command \
	test_config \
	test_logging \
	test_error

check_PROGRAMS = $(TESTS)

test_alloc_SOURCES = tests/test_alloc.c
test_alloc_LDFLAGS = -static
test_alloc_LDADD = libnih.la

test_string_SOURCES = tests/test_string.c
test_string_LDFLAGS = -static
test_string_LDADD = libnih.la -lutil

test_list_SOURCES = tests/test_list.c
test_list_LDFLAGS = -static
test_list_LDADD = libnih.la

test_hash_SOURCES = tests/test_hash.c
test_hash_LDFLAGS = -static
test_hash_LDADD = libnih.la

test_tree_SOURCES = tests/test_tree.c
test_tree_LDFLAGS = -static
test_tree_LDADD = libnih.la

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

test_file_SOURCES = tests/test_file.c
test_file_LDFLAGS = -static
test_file_LDADD = libnih.la

test_watch_SOURCES = tests/test_watch.c
test_watch_LDFLAGS = -static
test_watch_LDADD = libnih.la

test_main_SOURCES = tests/test_main.c
test_main_LDFLAGS = -static
test_main_LDADD = libnih.la

test_option_SOURCES = tests/test_option.c
test_option_LDFLAGS = -static
test_option_LDADD = libnih.la

test_command_SOURCES = tests/test_command.c
test_command_LDFLAGS = -static
test_command_LDADD = libnih.la

test_config_SOURCES = tests/test_config.c
test_config_LDFLAGS = -static
test_config_LDADD = libnih.la

test_logging_SOURCES = tests/test_logging.c
test_logging_LDFLAGS = -static
test_logging_LDADD = libnih.la

test_error_SOURCES = tests/test_error.c
test_error_LDFLAGS = -static
test_error_LDADD = libnih.la


.PHONY: tests
tests: $(BUILT_SOURCES) $(check_PROGRAMS)

clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
