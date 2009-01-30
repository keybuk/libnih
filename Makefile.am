## Process this file with automake to produce Makefile.in

SUBDIRS = m4 intl nih po

if HAVE_DBUS
SUBDIRS += nih-dbus nih-dbus-tool
endif

EXTRA_DIST = HACKING

ACLOCAL_AMFLAGS = --install -I m4
