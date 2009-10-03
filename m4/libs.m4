# libnih
#
# libs.m4 - autoconf macros for library detection
#
# Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
# Copyright © 2009 Canonical Ltd.
#
# This file is free software; the author gives unlimited permission to
# copy and/or distribute it, with or without modifications, as long as
# this notice is preserved.


# NIH_LIB_DBUS
# ------------
# Detect whether we can/should build the optional dbus bindings
AC_DEFUN([NIH_LIB_DBUS],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
m4_define([DBUS_MODULES], [dbus-1 >= 1.2.16])
m4_ifdef([_NIH_Option_dbus],
	 [nih_with_dbus=yes],
	 [AC_ARG_WITH(dbus,
	  	      AS_HELP_STRING([--with-dbus], [Build dbus bindings]),
		      [AS_IF([test "x$withval" != "xno"],
		             [nih_with_dbus=yes], [nih_with_dbus=no])])])
AS_IF([test "x$nih_with_dbus" = "xyes"],
      [PKG_CHECK_MODULES([DBUS], [DBUS_MODULES])
       AC_CHECK_LIB([expat], [XML_ParserCreate],
		    [AC_CHECK_LIB([expat], [XML_StopParser],
				  [AC_SUBST([EXPAT_LIBS], [-lexpat])],
				  [AC_MSG_ERROR([expat >= 2.0.0 required])])],
		    [AC_MSG_ERROR([expat library not found])])],
      [AS_IF([test "x$nih_with_dbus" != "xno"],
             [PKG_CHECK_MODULES([DBUS], [DBUS_MODULES],, [nih_have_dbus=no])
	      AC_CHECK_LIB([expat], [XML_ParserCreate],
			   [AC_CHECK_LIB([expat], [XML_StopParser],
					 [AC_SUBST([EXPAT_LIBS], [-lexpat])],
					 [nih_have_dbus=no])],
			   [nih_have_dbus=no])],			
	     [nih_have_dbus=no])])
AM_CONDITIONAL([HAVE_DBUS], [test "x$nih_have_dbus" != "xno"])

AS_IF([test "x$nih_have_dbus" != "xno"],
      [AC_ARG_VAR([NIH_DBUS_TOOL], [Path to external nih-dbus-tool when cross-compiling])
       AS_IF([test "$cross_compiling" = "yes"],
             [AC_CHECK_PROGS([NIH_DBUS_TOOL], [nih-dbus-tool])
	      AS_IF([test -z "$NIH_DBUS_TOOL"],
	      	    [AC_MSG_WARN([nih-dbus-tool not found, but you are cross-compiling.  Using built copy, which is probably not what you want.  Set NIH_DBUS_TOOL maybe?])
		     AC_SUBST([NIH_DBUS_TOOL], ["\${top_builddir}/nih-dbus-tool/nih-dbus-tool"])])],
	     [AC_SUBST([NIH_DBUS_TOOL], ["\${top_builddir}/nih-dbus-tool/nih-dbus-tool"])])])
])# NIH_LIB_DBUS
