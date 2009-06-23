# libnih
#
# misc.m4 - miscellaneous autoconf macros
#
# Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
# Copyright © 2009 Canonical Ltd.
#
# This file is free software; the author gives unlimited permission to
# copy and/or distribute it, with or without modifications, as long as
# this notice is preserved.


# AC_COPYRIGHT
# -------------
# Wraps the Autoconf AC_COPYRIGHT but also defines PACKAGE_COPYRIGHT,
# required for nih_main_init
m4_rename([AC_COPYRIGHT], [_NIH_AC_COPYRIGHT])
AC_DEFUN([AC_COPYRIGHT],
[_NIH_AC_COPYRIGHT([$1])
m4_ifndef([NIH_PACKAGE_COPYRIGHT], [m4_bmatch([$1], [
], [], [
	m4_define([NIH_PACKAGE_COPYRIGHT],
		  ["m4_bpatsubst([AS_ESCAPE([$1])], [©], [(C)])"])
	AC_DEFINE([PACKAGE_COPYRIGHT], [NIH_PACKAGE_COPYRIGHT],
		  [Define to the copyright message of this package.])])])dnl
	AC_SUBST([PACKAGE_COPYRIGHT], ["$1"])
])# AC_COPYRIGHT


# NIH_INIT([OPTIONS])
# -------------------
# Expands to the set of macros normally required for using libnih within
# another source tree.
#
# Options:
#   install         install libnih (normally not installed)
#   dbus            require that libnih-dbus be built
AC_DEFUN([NIH_INIT],
[m4_foreach_w([_NIH_Option], [$1],
	     [m4_define([_NIH_Option_]m4_bpatsubst(_NIH_Option, [[^a-zA_Z0-9_]], [_]))])

m4_ifdef([_NIH_Option_install], [nih_install=yes])
AM_CONDITIONAL([INSTALL_NIH], [test "x$nih_install" = "xyes"])

PKG_PROG_PKG_CONFIG([0.22])

# Checks for libraries
NIH_LIB_DBUS

# Checks for header files.
AC_CHECK_HEADERS([valgrind/valgrind.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_PROG_CC_C99
AM_PROG_CC_C_O
NIH_C_THREAD

# Other checks
NIH_COMPILER_WARNINGS
NIH_COMPILER_OPTIMISATIONS
NIH_COMPILER_COVERAGE

NIH_LINKER_OPTIMISATIONS
NIH_LINKER_VERSION_SCRIPT
])# NIH_INIT
