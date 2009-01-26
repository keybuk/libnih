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
	dbus.c

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
nihdbusincludedir = $(includedir)/nih-dbus
nihdbusinclude_HEADERS = \
	dbus.h \
	test_dbus.h
endif
endif


if HAVE_DBUS
TESTS = \
	test_dbus \
	test_com.netsplit.Nih.Test_object \
	test_com.netsplit.Nih.Test_proxy

check_PROGRAMS = $(TESTS)

test_dbus_SOURCES = tests/test_dbus.c
test_dbus_CFLAGS = $(DBUS_CFLAGS)
test_dbus_LDFLAGS = -static
test_dbus_LDADD = ../nih/libnih.la libnih-dbus.la $(DBUS_LIBS)

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
