## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	$(DBUS_CFLAGS)

AM_CPPFLAGS = \
	-DLOCALEDIR="\"$(localedir)\"" \
	-I$(top_builddir) -I$(top_srcdir) -iquote$(srcdir) \
	-I$(top_srcdir)/intl


if INSTALL_NIH
bin_PROGRAMS = \
	nih-dbus-tool
else
noinst_PROGRAMS = \
	nih-dbus-tool
endif

nih_dbus_tool_SOURCES = \
	main.c \
	symbol.c symbol.h \
	node.c node.h \
	interface.c interface.h \
	method.c method.h \
	signal.c signal.h \
	property.c property.h \
	argument.c argument.h \
	annotation.c annotation.h \
	parse.c parse.h \
	indent.c indent.h \
	errors.h

nih_dbus_tool_LDADD = \
	../nih/libnih.la \
	../nih-dbus/libnih-dbus.la \
	$(LTLIBINTL) \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)


TESTS = \
	test_main \
	test_symbol \
	test_node \
	test_interface \
	test_method \
	test_signal \
	test_property \
	test_argument \
	test_annotation \
	test_parse \
	test_indent \
	test_com.netsplit.Nih.Test_object \
	test_com.netsplit.Nih.Test_proxy

check_PROGRAMS = $(TESTS)

test_main_SOURCES = tests/test_main.c main.c
test_main_CFLAGS = $(AM_CFLAGS) -DTEST
test_main_LDFLAGS = -static
test_main_LDADD = ../nih/libnih.la

test_symbol_SOURCES = tests/test_symbol.c
test_symbol_LDFLAGS = -static
test_symbol_LDADD = \
	symbol.o \
	../nih/libnih.la

test_node_SOURCES = tests/test_node.c
test_node_LDFLAGS = -static
test_node_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_interface_SOURCES = tests/test_interface.c
test_interface_LDFLAGS = -static
test_interface_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_method_SOURCES = tests/test_method.c
test_method_LDFLAGS = -static
test_method_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_signal_SOURCES = tests/test_signal.c
test_signal_LDFLAGS = -static
test_signal_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_property_SOURCES = tests/test_property.c
test_property_LDFLAGS = -static
test_property_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_argument_SOURCES = tests/test_argument.c
test_argument_LDFLAGS = -static
test_argument_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_annotation_SOURCES = tests/test_annotation.c
test_annotation_LDFLAGS = -static
test_annotation_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_parse_SOURCES = tests/test_parse.c
test_parse_LDFLAGS = -static
test_parse_LDADD = \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_indent_SOURCES = tests/test_indent.c
test_indent_LDFLAGS = -static
test_indent_LDADD = \
	indent.o \
	../nih/libnih.la

test_com_netsplit_Nih_Test_object_SOURCES = \
	tests/test_com.netsplit.Nih.Test_object.c \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	tests/com.netsplit.Nih.Test_impl.c tests/com.netsplit.Nih.Test_impl.h
test_com_netsplit_Nih_Test_object_CFLAGS = $(DBUS_CFLAGS)
test_com_netsplit_Nih_Test_object_LDFLAGS = -static
test_com_netsplit_Nih_Test_object_LDADD = ../nih/libnih.la ../nih-dbus/libnih-dbus.la $(DBUS_LIBS)

test_com_netsplit_Nih_Test_proxy_SOURCES = \
	tests/test_com.netsplit.Nih.Test_proxy.c \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS) \
	tests/com.netsplit.Nih.Test_impl.c tests/com.netsplit.Nih.Test_impl.h
test_com_netsplit_Nih_Test_proxy_CFLAGS = $(DBUS_CFLAGS)
test_com_netsplit_Nih_Test_proxy_LDFLAGS = -static
test_com_netsplit_Nih_Test_proxy_LDADD = ../nih/libnih.la ../nih-dbus/libnih-dbus.la $(DBUS_LIBS)


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

EXTRA_DIST = \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_object_XML) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_XML)


tests: $(BUILT_SOURCES) $(check_PROGRAMS)

clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
