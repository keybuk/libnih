## Process this file with automake to produce Makefile.in

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR="\"$(localedir)\""
INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)


lib_LTLIBRARIES = libnih.la

libnih_la_SOURCES = \
	alloc.c \
	list.c \
	hash.c \
	main.c


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
	list.h \
	hash.h \
	main.h


TESTS = \
	test_alloc \
	test_list \
	test_hash \
	test_main

check_PROGRAMS = $(TESTS)

test_alloc_SOURCES = tests/test_alloc.c
test_alloc_LDADD = alloc.o list.o

test_list_SOURCES = tests/test_list.c
test_list_LDADD = alloc.o list.o

test_hash_SOURCES = tests/test_hash.c
test_hash_LDADD = alloc.o list.o hash.o

test_main_SOURCES = tests/test_main.c
test_main_LDADD = main.o


clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
