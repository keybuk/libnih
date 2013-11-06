## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	$(DBUS_CFLAGS)

AM_CPPFLAGS = \
	-DLOCALEDIR="\"$(localedir)\"" \
	-I$(top_builddir) -I$(top_srcdir) \
	-I$(top_srcdir)/intl


lib_LTLIBRARIES = libnih-dbus.la

libnih_dbus_la_SOURCES = \
	dbus_error.c \
	dbus_connection.c \
	dbus_message.c \
	dbus_object.c \
	dbus_pending_data.c \
	dbus_proxy.c \
	dbus_util.c

libnih_dbus_la_LDFLAGS = \
	-version-info 1:0:0
if HAVE_VERSION_SCRIPT_ARG
libnih_dbus_la_LDFLAGS += @VERSION_SCRIPT_ARG@=$(srcdir)/libnih-dbus.ver
endif

libnih_dbus_la_LIBADD = \
	../nih/libnih.la \
	$(DBUS_LIBS) \
	-lrt


include_HEADERS = \
	libnih-dbus.h

nihdbusincludedir = $(includedir)/nih-dbus
nihdbusinclude_HEADERS = \
	dbus_error.h \
	dbus_connection.h \
	dbus_message.h \
	dbus_interface.h \
	dbus_object.h \
	dbus_pending_data.h \
	dbus_proxy.h \
	dbus_util.h \
	errors.h \
	test_dbus.h


pkgconfigdir = $(prefix)/lib/pkgconfig
pkgconfig_DATA = libnih-dbus.pc


EXTRA_DIST = libnih-dbus.ver libnih-dbus.supp libnih-dbus.pc.in


LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
TESTS = \
	test_dbus_error \
	test_dbus_connection \
	test_dbus_message \
	test_dbus_object \
	test_dbus_pending_data \
	test_dbus_proxy \
	test_dbus_util

check_PROGRAMS = $(TESTS)

test_dbus_error_SOURCES = tests/test_dbus_error.c
test_dbus_error_LDFLAGS = -static
test_dbus_error_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_dbus_connection_SOURCES = tests/test_dbus_connection.c
test_dbus_connection_LDFLAGS = -static
test_dbus_connection_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_dbus_message_SOURCES = tests/test_dbus_message.c
test_dbus_message_LDFLAGS = -static
test_dbus_message_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_dbus_object_SOURCES = tests/test_dbus_object.c
test_dbus_object_LDFLAGS = -static
test_dbus_object_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_dbus_pending_data_SOURCES = tests/test_dbus_pending_data.c
test_dbus_pending_data_LDFLAGS = -static
test_dbus_pending_data_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_dbus_proxy_SOURCES = tests/test_dbus_proxy.c
test_dbus_proxy_LDFLAGS = -static
test_dbus_proxy_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_dbus_util_SOURCES = tests/test_dbus_util.c
test_dbus_util_LDFLAGS = -static
test_dbus_util_LDADD = libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)


.PHONY: tests
tests: $(BUILT_SOURCES) $(check_PROGRAMS)

clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
