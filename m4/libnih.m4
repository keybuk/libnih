# libnih
#
# libnih.m4 - autoconf macros
#
# Copyright © 2010 Scott James Remnant <scott@netsplit.com>.
# Copyright © 2010 Canonical Ltd.
#
# This file is free software; the author gives unlimited permission to
# copy and/or distribute it, with or without modifications, as long as
# this notice is preserved.


# This MUST be incremented for any changes to this file, otherwise aclocal
# may overwrite the local copy in the libnih source tree with any installed
# version.

# serial 2 libnih.m4


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

# NIH_LINKER_OPTIMISATIONS
# ------------------------
# Add configure option to disable linker optimisations.
AC_DEFUN([NIH_LINKER_OPTIMISATIONS],
[AC_ARG_ENABLE(linker-optimisations,
	AS_HELP_STRING([--disable-linker-optimisations],
		       [Disable linker optimisations]),
[AS_IF([test "x$enable_linker_optimisations" = "xno"],
       [LDFLAGS=`echo "$LDFLAGS" | sed -e "s/ -Wl,-O[0-9]*\b//g"`],
       [LDFLAGS="$LDFLAGS -Wl,-O1"])
])dnl
])# NIH_LINKER_OPTIMISATIONS


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


# NIH_LINKER_VERSION_SCRIPT
# -------------------------
# Detect whether the linker supports version scripts
AC_DEFUN([NIH_LINKER_VERSION_SCRIPT],
[AC_CACHE_CHECK([for linker version script argument], [nih_cv_version_script],
[nih_cv_version_script=none
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
		nih_cv_version_script="$nih_try_arg"
		break
	])

	rm -f conftest.ver
	LIBS="$nih_old_libs"
done])
AS_IF([test "x$nih_cv_version_script" != "xnone"],
      [AC_SUBST(VERSION_SCRIPT_ARG, [$nih_cv_version_script])])
AM_CONDITIONAL(HAVE_VERSION_SCRIPT_ARG,
	       [test "x$nih_cv_version_script" != "xnone"])
])# NIH_LINKER_VERSION_SCRIPT

# NIH_LINKER_SYMBOLIC_FUNCTIONS
# -----------------------------
# Detect whether the linker supports symbolic functions
AC_DEFUN([NIH_LINKER_SYMBOLIC_FUNCTIONS],
[AC_CACHE_CHECK([for linker symbolic functions argument],
[nih_cv_symbolic_functions],
[nih_cv_symbolic_functions=none
for nih_try_arg in "-Wl,-Bsymbolic-functions"; do
	nih_old_ldflags="$LDFLAGS"
	LDFLAGS="$LDFLAGS $nih_try_arg"
	AC_TRY_LINK([], [], [
		LDFLAGS="$nih_old_ldflags"
		nih_cv_symbolic_functions="$nih_try_arg"
		break
	])

	LDFLAGS="$nih_old_ldflags"
done])
AS_IF([test "x$nih_cv_symbolic_functions" != "xnone"],
      [LDFLAGS="$LDFLAGS $nih_cv_symbolic_functions"])dnl
])# NIH_LINKER_SYMBOLIC_FUNCTIONS


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


# NIH_COPYRIGHT
# --------------
# Wraps the Autoconf AC_COPYRIGHT but also defines PACKAGE_COPYRIGHT,
# required for nih_main_init
AC_DEFUN([NIH_COPYRIGHT],
[AC_COPYRIGHT([$1])
m4_ifndef([NIH_PACKAGE_COPYRIGHT], [m4_bmatch([$1], [
], [], [
	m4_define([NIH_PACKAGE_COPYRIGHT],
		  ["m4_bpatsubst([AS_ESCAPE([$1])], [©], [(C)])"])
	AC_DEFINE([PACKAGE_COPYRIGHT], [NIH_PACKAGE_COPYRIGHT],
		  [Define to the copyright message of this package.])])])dnl
	AC_SUBST([PACKAGE_COPYRIGHT], ["$1"])
])# AC_COPYRIGHT


# NIH_WITH_LOCAL_LIBNIH
# ---------------------
# Adds a configure option to build with a local libnih.
AC_DEFUN([NIH_WITH_LOCAL_LIBNIH],
[AC_ARG_WITH(local-libnih,
	AS_HELP_STRING([[[--with-local-libnih[=DIR]]]],
		       [Use libnih from source tree DIR]),
[AS_IF([test "x$with_local_libnih" != "xno"],
       [AS_IF([! test -f "$withval/nih/alloc.c"],
       	      [AC_MSG_ERROR([$withval doesn't look like a libnih source tree])])

	nih_dir="`cd $withval && pwd`"

        NIH_CFLAGS="-I\"$nih_dir\""
        NIH_LIBS="\"$nih_dir/nih/libnih.la\""
        NIH_DBUS_CFLAGS="-I\"$nih_dir\""
	NIH_DBUS_LIBS="\"$nih_dir/nih-dbus/libnih-dbus.la\""
	NIH_DBUS_TOOL="\"$nih_dir/nih-dbus-tool/nih-dbus-tool\""])])
])# NIH_WITH_LOCAL_LIBNIH
