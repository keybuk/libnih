# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([libnih], [0.1.0], [libnih-bugs@netsplit.com])
AC_COPYRIGHT([[Copyright © 2006 Scott James Remnant <scott@netsplit.com>.]])
AC_CONFIG_SRCDIR([nih/libnih.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 gnu check-news dist-bzip2])

AM_GNU_GETTEXT_VERSION(0.14.1)
AM_GNU_GETTEXT()

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
NIH_C_C99

# Checks for library functions.

# Other checks
NIH_COMPILER_WARNINGS
NIH_COMPILER_OPTIMISATIONS
NIH_COMPILER_COVERAGE
NIH_LINKER_OPTIMISATIONS
NIH_LINKER_VERSION_SCRIPT

AC_CONFIG_FILES([ Makefile m4/Makefile po/Makefile.in intl/Makefile
		  nih/Makefile ])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
