## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DLOCALEDIR="\"$(localedir)\"" \
	-I$(top_builddir) -I$(top_srcdir) \
	-I$(top_srcdir)/intl


if HAVE_DBUS
libraries = libnih-dbus.la

if INSTALL_NIH
lib_LTLIBRARIES = $(libraries)
else
noinst_LTLIBRARIES = $(libraries)
endif
endif

libnih_dbus_la_SOURCES = \
	dbus_error.c \
	dbus_connection.c \
	dbus_message.c \
	dbus_object.c \
	dbus_proxy.c \
	dbus_util.c

libnih_dbus_la_CFLAGS = $(DBUS_CFLAGS)
libnih_dbus_la_LIBS = $(DBUS_LIBS)

if INSTALL_NIH
libnih_dbus_la_LDFLAGS = \
	-version-info 0:0:0
if HAVE_VERSION_SCRIPT_ARG
libnih_dbus_la_LDFLAGS += @VERSION_SCRIPT_ARG@=$(srcdir)/libnih-dbus.ver
endif
endif


EXTRA_DIST = libnih-dbus.ver libnih-dbus.supp


if HAVE_DBUS
if INSTALL_NIH
bin_SCRIPTS = \
	nih-dbus-tool
else
if HAVE_PYTHON
noinst_SCRIPTS = \
	nih-dbus-tool
endif
endif
endif

CLEANFILES = nih-dbus-tool
EXTRA_DIST += nih_dbus_tool.py

do_subst = sed \
	-e '1s|\#!.*|\#!$(PYTHON)|g' \
	-e 's|[@]PACKAGE_NAME[@]|$(PACKAGE_NAME)|g' \
	-e 's|[@]PACKAGE_COPYRIGHT[@]|$(PACKAGE_COPYRIGHT)|g'

nih-dbus-tool: nih_dbus_tool.py
	$(do_subst) $< > $@
	chmod +x $@


if HAVE_DBUS
if INSTALL_NIH
include_HEADERS = \
	libnih-dbus.h

nihdbusincludedir = $(includedir)/nih-dbus
nihdbusinclude_HEADERS = \
	dbus_error.h \
	dbus_connection.h \
	dbus_message.h \
	dbus_object.h \
	dbus_proxy.h \
	dbus_util.h \
	test_dbus.h
endif
endif


if HAVE_DBUS
TESTS = \
	test_dbus_error \
	test_dbus_connection \
	test_dbus_message \
	test_dbus_object \
	test_dbus_proxy \
	test_dbus_util \
	test_com.netsplit.Nih.Test_object \
	test_com.netsplit.Nih.Test_proxy

check_PROGRAMS = $(TESTS)

test_dbus_error_SOURCES = tests/test_dbus_error.c
test_dbus_error_CFLAGS = $(DBUS_CFLAGS)
test_dbus_error_LDFLAGS = -static
test_dbus_error_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_dbus_connection_SOURCES = tests/test_dbus_connection.c
test_dbus_connection_CFLAGS = $(DBUS_CFLAGS)
test_dbus_connection_LDFLAGS = -static
test_dbus_connection_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_dbus_message_SOURCES = tests/test_dbus_message.c
test_dbus_message_CFLAGS = $(DBUS_CFLAGS)
test_dbus_message_LDFLAGS = -static
test_dbus_message_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_dbus_object_SOURCES = tests/test_dbus_object.c
test_dbus_object_CFLAGS = $(DBUS_CFLAGS)
test_dbus_object_LDFLAGS = -static
test_dbus_object_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_dbus_proxy_SOURCES = tests/test_dbus_proxy.c
test_dbus_proxy_CFLAGS = $(DBUS_CFLAGS)
test_dbus_proxy_LDFLAGS = -static
test_dbus_proxy_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_dbus_util_SOURCES = tests/test_dbus_util.c
test_dbus_util_CFLAGS = $(DBUS_CFLAGS)
test_dbus_util_LDFLAGS = -static
test_dbus_util_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_com_netsplit_Nih_Test_object_SOURCES = \
	tests/test_com.netsplit.Nih.Test_object.c \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	tests/com.netsplit.Nih.Test_impl.c tests/com.netsplit.Nih.Test_impl.h
test_com_netsplit_Nih_Test_object_CFLAGS = $(DBUS_CFLAGS)
test_com_netsplit_Nih_Test_object_LDFLAGS = -static
test_com_netsplit_Nih_Test_object_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

test_com_netsplit_Nih_Test_proxy_SOURCES = \
	tests/test_com.netsplit.Nih.Test_proxy.c \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS) \
	tests/com.netsplit.Nih.Test_impl.c tests/com.netsplit.Nih.Test_impl.h
test_com_netsplit_Nih_Test_proxy_CFLAGS = $(DBUS_CFLAGS)
test_com_netsplit_Nih_Test_proxy_LDFLAGS = -static
test_com_netsplit_Nih_Test_proxy_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)


com_netsplit_Nih_Test_object_OUTPUTS = \
	tests/com.netsplit.Nih.Test_object.c \
	tests/com.netsplit.Nih.Test_object.h

com_netsplit_Nih_Test_object_XML = \
	tests/com.netsplit.Nih.Test.xml

$(com_netsplit_Nih_Test_object_OUTPUTS): $(com_netsplit_Nih_Test_object_XML) nih_dbus_tool.py
	$(MAKE) $(AM_MAKEFLAGS) nih-dbus-tool
	$(builddir)/nih-dbus-tool --mode=object --prefix=my --output=$@ $<


com_netsplit_Nih_Test_proxy_OUTPUTS = \
	tests/com.netsplit.Nih.Test_proxy.c \
	tests/com.netsplit.Nih.Test_proxy.h

com_netsplit_Nih_Test_proxy_XML = \
	tests/com.netsplit.Nih.Test.xml

$(com_netsplit_Nih_Test_proxy_OUTPUTS): $(com_netsplit_Nih_Test_proxy_XML) nih_dbus_tool.py
	$(MAKE) $(AM_MAKEFLAGS) nih-dbus-tool
	$(builddir)/nih-dbus-tool --mode=proxy --prefix=proxy --output=$@ $<


BUILT_SOURCES = \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS)

MAINTAINERCLEANFILES = \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS)

EXTRA_DIST += \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_object_XML) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_XML)
endif


clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
