## Process this file with automake to produce Makefile.in

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR="\"$(localedir)\""
INCLUDES = -I$(top_srcdir)/intl


lib_LTLIBRARIES = libnih.la

libnih_la_SOURCES = \
	list.c

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
	list.h


TESTS = \
	test_list

check_PROGRAMS = $(TESTS)

test_list_SOURCES = tests/test_list.c
test_list_LDADD = list.o


clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
