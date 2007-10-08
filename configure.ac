# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT([libnih], [0.1.0], [libnih-bugs@netsplit.com])
AC_COPYRIGHT([[Copyright © 2007 Scott James Remnant <scott@netsplit.com>.]])
AC_CONFIG_SRCDIR([nih/libnih.h])
AC_CONFIG_MACRO_DIR([m4])

AC_GNU_SOURCE

AM_INIT_AUTOMAKE([1.10 gnu nostdinc check-news dist-bzip2])

AM_GNU_GETTEXT_VERSION([0.15])
AM_GNU_GETTEXT()

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
NIH_INIT

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Other checks

AC_CONFIG_FILES([ Makefile m4/Makefile po/Makefile.in intl/Makefile
		  nih/Makefile ])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
