# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([libnih], [0.3.1], [http://bugs.launchpad.net/libnih/])
AC_COPYRIGHT([[Copyright © 2009 Scott James Remnant <scott@netsplit.com>.]])
AC_CONFIG_SRCDIR([nih/libnih.h])
AC_CONFIG_MACRO_DIR([m4])

AC_GNU_SOURCE

AM_INIT_AUTOMAKE([1.10 gnu nostdinc check-news dist-bzip2])

LT_PREREQ(2.2.4)
LT_INIT

AM_GNU_GETTEXT_VERSION([0.16.1])
AM_GNU_GETTEXT()

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
NIH_INIT([install])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Other checks

AC_CONFIG_FILES([ Makefile m4/Makefile intl/Makefile
		  nih/Makefile nih-dbus/Makefile nih-dbus-tool/Makefile
		  po/Makefile.in ])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
