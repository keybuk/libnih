# libnih
#
# linker.m4 - autoconf macros for linker settings
#
# Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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


# NIH_LINKER_OPTIMISATIONS
# --------------------------
# Add configure option to disable linker optimisations.
AC_DEFUN([NIH_LINKER_OPTIMISATIONS],
[AC_ARG_ENABLE(linker-optimisations,
	AS_HELP_STRING([--disable-linker-optimisations],
		       [Disable linker optimisations]),
[if test "x$enable_linker_optimisations" = "xno"; then
	[LDFLAGS=`echo "$LDFLAGS" | sed -e "s/ -Wl,-O[0-9]*\b//g"`]
else
	[LDFLAGS="$LDFLAGS -Wl,-O1"]
fi], [LDFLAGS="$LDFLAGS -Wl,-O1"])dnl
])# NIH_LINKER_OPTIMISATIONS

# NIH_LINKER_VERSION_SCRIPT
# ------------------
# Detect whether the linker supports version scripts
AC_DEFUN([NIH_LINKER_VERSION_SCRIPT],
[AC_MSG_CHECKING([for linker version script argument])
for nih_try_arg in "-Wl,--version-script"; do
	nih_old_libs="$LIBS"
	LIBS="$LIBS $nih_try_arg=conftest.ver"

	cat >conftest.ver <<EOF
TEST {
	global:
		*;
};
EOF

	AC_TRY_LINK([], [], [
		rm -f conftest.ver
		LIBS="$nih_old_libs"

		AC_MSG_RESULT([$nih_try_arg])
		AC_SUBST(VERSION_SCRIPT_ARG, [$nih_try_arg])
		break
	])

	rm -f conftest.ver
	LIBS="$nih_old_libs"
done

AM_CONDITIONAL(HAVE_VERSION_SCRIPT_ARG, [test -n "$VERSION_SCRIPT_ARG"])
if test -z "$VERSION_SCRIPT_ARG"; then
	AC_MSG_RESULT([unknown])
fi
])dnl
])# NIH_LINKER_VERSION_SCRIPT
