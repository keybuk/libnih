## Process this file with automake to produce Makefile.in

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR="\"$(localedir)\""
INCLUDES = -I$(top_srcdir)/intl


lib_LTLIBRARIES = libnih.la

libnih_la_SOURCES =

libnih_la_LDFLAGS = \
	-version-info 0:0:0


pkginclude_HEADERS =


TESTS =

check_PROGRAMS = $(TESTS)


clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
