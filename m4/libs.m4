# libnih
#
# libs.m4 - autoconf macros for library detection
#
# Copyright © 2008 Scott James Remnant <scott@netsplit.com>.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# NIH_LIB_DBUS
# ------------
# Detect whether we can/should build the optional dbus bindings
AC_DEFUN([NIH_LIB_DBUS],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
m4_define([DBUS_MODULES], [dbus-1 >= 1.1.1])
m4_ifdef([_NIH_Option_dbus],
	 [nih_with_dbus=yes],
	 [AC_ARG_WITH(dbus,
	  	      AS_HELP_STRING([--with-dbus], [Build dbus bindings]),
		      [AS_IF([test "x$withval" != "xno"],
		             [nih_with_dbus=yes], [nih_with_dbus=no])])])
AS_IF([test "x$nih_with_dbus" = "xyes"],
      [PKG_CHECK_MODULES([DBUS], [DBUS_MODULES])
       AM_PATH_PYTHON([2.5])],
      [AS_IF([test "x$nih_with_dbus" != "xno"],
             [PKG_CHECK_MODULES([DBUS], [DBUS_MODULES],, [nih_have_dbus=no])
	      AM_PATH_PYTHON([2.5],, [nih_have_dbus=no])],
	     [nih_have_dbus=no])])
AM_CONDITIONAL([HAVE_DBUS], [test "x$nih_have_dbus" != "xno"])
])# NIH_LIB_DBUS
