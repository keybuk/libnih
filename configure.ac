# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.62)
AC_INIT([libnih], [1.0.3], [http://bugs.launchpad.net/libnih/])
NIH_COPYRIGHT([[Copyright © 2010 Scott James Remnant <scott@netsplit.com>.]])
AC_CONFIG_SRCDIR([nih/libnih.h])
AC_CONFIG_MACRO_DIR([m4])

AC_GNU_SOURCE

AM_INIT_AUTOMAKE([1.11 gnu nostdinc check-news color-tests silent-rules])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

LT_PREREQ(2.2.4)
LT_INIT

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT()

# Checks for programs.
AC_PROG_CC
PKG_PROG_PKG_CONFIG([0.22])

# Checks for libraries.
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.2.16])
AC_CHECK_LIB([expat], [XML_ParserCreate],
	     [AC_CHECK_LIB([expat], [XML_StopParser],
	     		   [AC_SUBST([EXPAT_LIBS], [-lexpat])],
			   [AC_MSG_ERROR([expat >= 2.0.0 required])])],
	     [AC_MSG_ERROR([expat library not found])])

# Checks for header files.
AC_CHECK_HEADERS([valgrind/valgrind.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_PROG_CC_C99
AM_PROG_CC_C_O
NIH_C_THREAD

# Checks for library functions.

# Other checks
NIH_COMPILER_WARNINGS
NIH_COMPILER_OPTIMISATIONS
NIH_COMPILER_COVERAGE

NIH_LINKER_OPTIMISATIONS
NIH_LINKER_VERSION_SCRIPT

# Allow a pre-build nih-dbus-tool to be used when cross-compiling.
# Programs can just use AC_CHECK_PROGS for this
AC_ARG_VAR([NIH_DBUS_TOOL], [Path to external nih-dbus-tool when cross-compiling])
AS_IF([test "$cross_compiling" = "yes"],
      [AC_CHECK_PROGS([NIH_DBUS_TOOL], [nih-dbus-tool])
       AS_IF([test -z "$NIH_DBUS_TOOL"],
	     [AC_MSG_WARN([nih-dbus-tool not found, but you are cross-compiling.  Using built copy, which is probably not what you want.  Set NIH_DBUS_TOOL maybe?])
	      AC_SUBST([NIH_DBUS_TOOL], ["\${top_builddir}/nih-dbus-tool/nih-dbus-tool"])])],
      [AC_SUBST([NIH_DBUS_TOOL], ["\${top_builddir}/nih-dbus-tool/nih-dbus-tool"])])

AC_CONFIG_FILES([ Makefile m4/Makefile intl/Makefile
		  nih/Makefile nih/libnih.pc
		  nih-dbus/Makefile nih-dbus/libnih-dbus.pc
		  nih-dbus-tool/Makefile
		  po/Makefile.in ])
AC_CONFIG_HEADERS([config.h])
AC_OUTPUT
