## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DLOCALEDIR="\"$(localedir)\"" \
	-I$(top_builddir) -I$(top_srcdir) \
	-I$(top_srcdir)/intl


libraries = libnih.la
if HAVE_DBUS
libraries += libnih-dbus.la
endif

if INSTALL_NIH
lib_LTLIBRARIES = $(libraries)
else
noinst_LTLIBRARIES = $(libraries)
endif

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


if INSTALL_NIH
libnih_la_LDFLAGS = \
	-version-info 0:0:0
if HAVE_VERSION_SCRIPT_ARG
libnih_la_LDFLAGS += @VERSION_SCRIPT_ARG@=$(srcdir)/libnih.ver
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


EXTRA_DIST = libnih.ver libnih-dbus.ver libnih.supp libnih-dbus.supp


if HAVE_DBUS
if INSTALL_NIH
bin_SCRIPTS = \
	nih-dbus-tool
else
noinst_SCRIPTS = \
	nih-dbus-tool
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


if INSTALL_NIH
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
	inotify.h \
	test.h

if HAVE_DBUS
nihinclude_HEADERS += \
	dbus.h
endif
endif


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

if HAVE_DBUS
TESTS += \
	test_dbus \
	test_com.netsplit.Nih.Test_object
endif

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

test_dbus_SOURCES = tests/test_dbus.c
test_dbus_CFLAGS = $(DBUS_CFLAGS)
test_dbus_LDFLAGS = -static
test_dbus_LDADD = libnih.la libnih-dbus.la $(DBUS_LIBS)


test_com_netsplit_Nih_Test_object_SOURCES = \
	tests/test_com.netsplit.Nih.Test_object.c \
	tests/com.netsplit.Nih.Test_object.c \
	tests/com.netsplit.Nih.Test_object.h
test_com_netsplit_Nih_Test_object_CFLAGS = $(DBUS_CFLAGS)
test_com_netsplit_Nih_Test_object_LDFLAGS = -static
test_com_netsplit_Nih_Test_object_LDADD = libnih.la libnih-dbus.la $(DBUS_LIBS)

BUILT_SOURCES = \
	tests/com.netsplit.Nih.Test_object.c \
	tests/com.netsplit.Nih.Test_object.h

EXTRA_DIST += tests/com.netsplit.Nih.Test.xml

tests/com.netsplit.Nih.Test_object.c tests/com.netsplit.Nih.Test_object.h: \
		tests/com.netsplit.Nih.Test.xml nih-dbus-tool
	$(mkdir_p) tests
	$(builddir)/nih-dbus-tool --mode=object --prefix=my --output=$@ $<


CLEANFILES += $(BUILT_SOURCES)

clean-local:
	rm -f *.gcno *.gcda

maintainer-clean-local:
	rm -f *.gcov
