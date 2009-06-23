# libnih
#
# compiler.m4 - autoconf macros for compiler settings
#
# Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
# Copyright © 2009 Canonical Ltd.
#
# This file is free software; the author gives unlimited permission to
# copy and/or distribute it, with or without modifications, as long as
# this notice is preserved.


# NIH_COMPILER_WARNINGS
# ---------------------
# Add configure option to enable additional compiler warnings and treat
# them as errors.
AC_DEFUN([NIH_COMPILER_WARNINGS],
[AC_ARG_ENABLE(compiler-warnings,
	AS_HELP_STRING([--enable-compiler-warnings],
	               [Enable additional compiler warnings]),
[AS_IF([test "x$enable_compiler_warnings" = "xyes"],
       [AS_IF([test "x$GCC" = "xyes"],
              [CFLAGS="-Wall -Wextra -Wno-empty-body -Wno-missing-field-initializers -Wno-unused-parameter -Wformat-security -Werror -D_FORTIFY_SOURCE=2 $CFLAGS"])
        AS_IF([test "x$GXX" = "xyes"],
       	      [CXXFLAGS="-Wall -Wextra -Wno-empty-body -Wno-missing-field-initializers -Wno-unused-parameter -Wformat-security -Werror -D_FORTIFY_SOURCE=2 $CXXFLAGS"])])
])dnl
])# NIH_COMPILER_WARNINGS

# NIH_COMPILER_OPTIMISATIONS
# --------------------------
# Add configure option to disable optimisations.
AC_DEFUN([NIH_COMPILER_OPTIMISATIONS],
[AC_ARG_ENABLE(compiler-optimisations,
	AS_HELP_STRING([--disable-compiler-optimisations],
		       [Disable compiler optimisations]),
[AS_IF([test "x$enable_compiler_optimisations" = "xno"],
       [[CFLAGS=`echo "$CFLAGS" | sed -e "s/ -O[1-9s]*\b/ -O0/g"`
	 CXXFLAGS=`echo "$CXXFLAGS" | sed -e "s/ -O[1-9s]*\b/ -O0/g"`]])
])dnl
])# NIH_COMPILER_OPTIMISATIONS

# NIH_COMPILER_COVERAGE
# ---------------------
# Add configure option to enable coverage data.
AC_DEFUN([NIH_COMPILER_COVERAGE],
[AC_ARG_ENABLE(compiler-coverage,
	AS_HELP_STRING([--enable-compiler-coverage],
		       [Enable generation of coverage data]),
[AS_IF([test "x$enable_compiler_coverage" = "xyes"],
       [AS_IF([test "x$GCC" = "xyes"],
	      [CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])
	AS_IF([test "x$GXX" = "xyes"],
	      [CXXFLAGS="$CXXFLAGS -fprofile-arcs -ftest-coverage"])])
])dnl
])# NIH_COMPILER_COVERAGE

# NIH_TRY_C99([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -----------------------------------------------------
# Try compiling some C99 code to see whether it works.
AC_DEFUN([NIH_TRY_C99],
[AC_TRY_COMPILE([
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>


/* Variadic macro arguments */
#define variadic_macro(foo, ...) printf(foo, __VA_ARGS__)
],
[
	/* Compound initialisers */
	struct { int a, b; } foo = { .a = 1, .b = 2 };

	/* Boolean type */
	bool bar = false;

	/* Specific size type */
	uint32_t baz = 0;

	/* C99-style for-loop declarations */
	for (int i = 0; i < 10; i++)
		continue;

	/* Magic __func__ variable */
	printf("%s", __func__);
], [$1], [$2])dnl
])# NIH_TRY_C99

# NIH_C_C99
# ---------
# Check whether the compiler can do C99, adding a compiler flag if
# necessary.
AC_DEFUN([NIH_C_C99],
[AC_CACHE_CHECK([whether compiler supports C99 features], [nih_cv_c99],
	[NIH_TRY_C99([nih_cv_c99=yes], [nih_cv_c99=no])])
AS_IF([test "x$nih_cv_c99" = "xyes"],
	[AC_DEFINE([HAVE_C99], 1, [Define to 1 if the compiler supports C99.])],
	[AC_CACHE_CHECK([what argument makes compiler support C99 features],
		[nih_cv_c99_arg],
		[nih_cv_c99_arg=none
		 nih_save_CC="$CC"
		 for arg in "-std=gnu99" "-std=c99" "-c99"; do
		    CC="$nih_save_CC $arg"
		    NIH_TRY_C99([nih_arg_worked=yes], [nih_arg_worked=no])
		    CC="$nih_save_CC"

		    AS_IF([test "x$nih_arg_worked" = "xyes"],
			  [nih_cv_c99_arg="$arg"; break])
		 done])
	 AS_IF([test "x$nih_cv_c99_arg" != "xnone"],
	       [CC="$CC $nih_cv_c99_arg"
		AC_DEFINE([HAVE_C99], 1)])])[]dnl
])# NIH_C_C99

# NIH_C_THREAD
# ------------
# Check whether compiler supports __thread.
AC_DEFUN([NIH_C_THREAD],
[AC_ARG_ENABLE(threading,
	AS_HELP_STRING([--enable-threading],
		       [Enable support for multi-threading]),
[], [enable_threading=no])dnl
AS_IF([test "x$enable_threading" != "xno" ],
[AC_CACHE_CHECK([whether compiler supports __thread], [nih_cv_c_thread],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int __thread a;]], [])],
                   [nih_cv_c_thread=yes], [nih_cv_c_thread=no])])
AS_IF([test "x$nih_cv_c_thread" = "xno"],
      [AC_DEFINE([__thread],,
                 [Define to empty if `__thread' is not supported.])])],
[AC_DEFINE([__thread], )])dnl
])# NIH_C_THREAD
