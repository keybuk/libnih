# libnih
#
# compiler.m4 - autoconf macros for compiler settings
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


# Ensure that misc.m4 is included
m4_ifndef([_NIH_AC_COPYRIGHT], [m4_include([m4/misc.m4])])


# NIH_COMPILER_WARNINGS
# ---------------------
# Add configure option to enable additional compiler warnings and treat
# them as errors.
AC_DEFUN([NIH_COMPILER_WARNINGS],
[AC_ARG_ENABLE(compiler-warnings,
	AS_HELP_STRING([--enable-compiler-warnings],
	               [Enable additional compiler warnings]),
[if test "x$enable_compiler_warnings" = "xyes"; then
	if test "x$GCC" = "xyes"; then
                CFLAGS="-Wall -Werror -pedantic $CFLAGS"
        fi
	if test "x$GXX" = "xyes"; then
		CXXFLAGS="-Wall -Werror -pedantic $CXXFLAGS"
	fi
fi])dnl
])# NIH_COMPILER_WARNINGS

# NIH_COMPILER_OPTIMISATIONS
# --------------------------
# Add configure option to disable optimisations.
AC_DEFUN([NIH_COMPILER_OPTIMISATIONS],
[AC_ARG_ENABLE(compiler-optimisations,
	AS_HELP_STRING([--disable-compiler-optimisations],
		       [Disable compiler optimisations]),
[if test "x$enable_compiler_optimisations" = "xno"; then
	[CFLAGS=`echo "$CFLAGS" | sed -e "s/ -O[1-9]*\b/ -O0/g"`]
	[CXXFLAGS=`echo "$CXXFLAGS" | sed -e "s/ -O[1-9]*\b/ -O0/g"`]
fi])dnl
])# NIH_COMPILER_OPTIMISATIONS

# NIH_COMPILER_COVERAGE
# ----------------------
# Add configure option to enable coverage data.
AC_DEFUN([NIH_COMPILER_COVERAGE],
[AC_ARG_ENABLE(compiler-coverage,
	AS_HELP_STRING([--enable-compiler-coverage],
		       [Enable generation of coverage data]),
[if test "x$enable_compiler_coverage" = "xyes"; then
	if test "x$GCC" = "xyes"; then
		CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	fi
	if test "x$GXX" = "xyes"; then
		CXXFLAGS="$CXXFLAGS -fprofile-arcs -ftest-coverage"
	fi
fi])dnl
])# NIH_COMPILER_COVERAGE
