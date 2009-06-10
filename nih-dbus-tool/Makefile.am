## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	$(DBUS_CFLAGS)

AM_CPPFLAGS = \
	-DLOCALEDIR="\"$(localedir)\"" \
	-I$(top_builddir) -I$(top_srcdir) -iquote$(srcdir) \
	-I$(top_srcdir)/intl


dist_man_MANS = \
	man/nih-dbus-tool.1


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
	indent.c indent.h \
	type.c type.h \
	marshal.c marshal.h \
	demarshal.c demarshal.h \
	node.c node.h \
	interface.c interface.h \
	method.c method.h \
	signal.c signal.h \
	property.c property.h \
	argument.c argument.h \
	annotation.c annotation.h \
	parse.c parse.h \
	output.c output.h \
	errors.h

nih_dbus_tool_LDADD = \
	../nih-dbus/libnih-dbus.la \
	../nih/libnih.la \
	$(LTLIBINTL) \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)


TESTS = \
	test_main \
	test_symbol \
	test_indent \
	test_type \
	test_marshal \
	test_demarshal \
	test_node \
	test_interface \
	test_method \
	test_signal \
	test_property \
	test_argument \
	test_annotation \
	test_parse \
	test_output \
	test_com.netsplit.Nih.Test_object \
	test_com.netsplit.Nih.Test_proxy

check_PROGRAMS = \
	$(TESTS) \
	marshal_factory \
	demarshal_factory \
	method_factory \
	signal_factory \
	property_factory

test_main_SOURCES = tests/test_main.c main.c
test_main_CFLAGS = $(AM_CFLAGS) -DTEST
test_main_LDFLAGS = -static
test_main_LDADD = ../nih/libnih.la

test_symbol_SOURCES = tests/test_symbol.c
test_symbol_LDFLAGS = -static
test_symbol_LDADD = \
	symbol.o \
	../nih/libnih.la

test_indent_SOURCES = tests/test_indent.c
test_indent_LDFLAGS = -static
test_indent_LDADD = \
	indent.o \
	../nih/libnih.la

test_type_SOURCES = tests/test_type.c
test_type_LDFLAGS = -static
test_type_LDADD = \
	type.o \
	../nih/libnih.la \
	$(DBUS_LIBS)

test_marshal_SOURCES = \
	tests/test_marshal.c tests/marshal_code.h
nodist_test_marshal_SOURCES = \
	tests/marshal_code.c
test_marshal_LDFLAGS = -static
test_marshal_LDADD = \
	marshal.o type.o indent.o \
	../nih/libnih.la \
	$(DBUS_LIBS)

test_demarshal_SOURCES = \
	tests/test_demarshal.c tests/demarshal_code.h
nodist_test_demarshal_SOURCES = \
	tests/demarshal_code.c
test_demarshal_LDFLAGS = -static
test_demarshal_LDADD = \
	demarshal.o type.o indent.o \
	../nih/libnih.la \
	$(DBUS_LIBS)

test_node_SOURCES = tests/test_node.c
test_node_LDFLAGS = -static
test_node_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_interface_SOURCES = tests/test_interface.c
test_interface_LDFLAGS = -static
test_interface_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_method_SOURCES = \
	tests/test_method.c tests/method_code.h
nodist_test_method_SOURCES = \
	tests/method_code.c
test_method_LDFLAGS = -static
test_method_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih-dbus/libnih-dbus.la \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_signal_SOURCES = \
	tests/test_signal.c tests/signal_code.h
nodist_test_signal_SOURCES = \
	tests/signal_code.c
test_signal_LDFLAGS = -static
test_signal_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih-dbus/libnih-dbus.la \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_property_SOURCES = \
	tests/test_property.c tests/property_code.h
nodist_test_property_SOURCES = \
	tests/property_code.c
test_property_LDFLAGS = -static
test_property_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih-dbus/libnih-dbus.la \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_argument_SOURCES = tests/test_argument.c
test_argument_LDFLAGS = -static
test_argument_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_annotation_SOURCES = tests/test_annotation.c
test_annotation_LDFLAGS = -static
test_annotation_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_parse_SOURCES = tests/test_parse.c
test_parse_LDFLAGS = -static
test_parse_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

test_output_SOURCES = tests/test_output.c
test_output_LDFLAGS = -static
test_output_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	output.o parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)


test_com_netsplit_Nih_Test_object_SOURCES = \
	tests/test_com.netsplit.Nih.Test_object.c \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	tests/com.netsplit.Nih.Test_impl.c tests/com.netsplit.Nih.Test_impl.h
test_com_netsplit_Nih_Test_object_LDFLAGS = -static
test_com_netsplit_Nih_Test_object_LDADD = ../nih-dbus/libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)

test_com_netsplit_Nih_Test_proxy_SOURCES = \
	tests/test_com.netsplit.Nih.Test_proxy.c \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS) \
	tests/com.netsplit.Nih.Test_impl.c tests/com.netsplit.Nih.Test_impl.h
test_com_netsplit_Nih_Test_proxy_LDFLAGS = -static
test_com_netsplit_Nih_Test_proxy_LDADD = ../nih-dbus/libnih-dbus.la ../nih/libnih.la $(DBUS_LIBS)


com_netsplit_Nih_Test_object_OUTPUTS = \
	tests/com.netsplit.Nih.Test_object.c \
	tests/com.netsplit.Nih.Test_object.h

com_netsplit_Nih_Test_object_XML = \
	tests/com.netsplit.Nih.Test.xml

$(com_netsplit_Nih_Test_object_OUTPUTS): $(com_netsplit_Nih_Test_object_XML) $(builddir)/nih-dbus-tool
	$(MKDIR_P) `echo "$@" | \
		    sed '/\//!d;s,/[^/]*$$,,' | \
                    sort -u`
	$(builddir)/nih-dbus-tool --mode=object --prefix=my --output=$@ $<


com_netsplit_Nih_Test_proxy_OUTPUTS = \
	tests/com.netsplit.Nih.Test_proxy.c \
	tests/com.netsplit.Nih.Test_proxy.h

com_netsplit_Nih_Test_proxy_XML = \
	tests/com.netsplit.Nih.Test.xml

$(com_netsplit_Nih_Test_proxy_OUTPUTS): $(com_netsplit_Nih_Test_proxy_XML) $(builddir)/nih-dbus-tool
	$(MKDIR_P) `echo "$@" | \
		    sed '/\//!d;s,/[^/]*$$,,' | \
                    sort -u`
	$(builddir)/nih-dbus-tool --mode=proxy --prefix=your --output=$@ $<


# To be able to list them in POTFILES.in, these files must be built by
# an ordinary "make all" pass
BUILT_SOURCES = \
	$(com_netsplit_Nih_Test_object_OUTPUTS) \
	$(com_netsplit_Nih_Test_proxy_OUTPUTS)

EXTRA_DIST = \
	$(com_netsplit_Nih_Test_object_XML) \
	$(com_netsplit_Nih_Test_proxy_XML)


marshal_factory_SOURCES = tests/marshal_factory.c
marshal_factory_LDFLAGS = -static
marshal_factory_LDADD = \
	marshal.o type.o indent.o \
	../nih/libnih.la \
	$(DBUS_LIBS)

tests/marshal_code.c: $(builddir)/marshal_factory
	$(builddir)/marshal_factory > $@


demarshal_factory_SOURCES = tests/demarshal_factory.c
demarshal_factory_LDFLAGS = -static
demarshal_factory_LDADD = \
	demarshal.o type.o indent.o \
	../nih/libnih.la \
	$(DBUS_LIBS)

tests/demarshal_code.c: $(builddir)/demarshal_factory
	$(builddir)/demarshal_factory > $@


method_factory_SOURCES = tests/method_factory.c
method_factory_LDFLAGS = -static
method_factory_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

tests/method_code.c: $(builddir)/method_factory
	$(builddir)/method_factory > $@


signal_factory_SOURCES = tests/signal_factory.c
signal_factory_LDFLAGS = -static
signal_factory_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

tests/signal_code.c: $(builddir)/signal_factory
	$(builddir)/signal_factory > $@


property_factory_SOURCES = tests/property_factory.c
property_factory_LDFLAGS = -static
property_factory_LDADD = \
	demarshal.o marshal.o type.o indent.o \
	parse.o annotation.o argument.o property.o signal.o method.o \
	interface.o node.o symbol.o \
	../nih/libnih.la \
	$(EXPAT_LIBS) \
	$(DBUS_LIBS)

tests/property_code.c: $(builddir)/property_factory
	$(builddir)/property_factory > $@


CLEANFILES = \
	$(nodist_test_marshal_SOURCES) \
	$(nodist_test_demarshal_SOURCES) \
	$(nodist_test_method_SOURCES) \
	$(nodist_test_signal_SOURCES) \
	$(nodist_test_property_SOURCES)


.PHONY: tests
tests: $(BUILT_SOURCES) $(check_PROGRAMS)

clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
