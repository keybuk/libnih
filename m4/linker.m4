# libnih
#
# linker.m4 - autoconf macros for linker settings
#
# Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
# Copyright © 2009 Canonical Ltd.
#
# This file is free software; the author gives unlimited permission to
# copy and/or distribute it, with or without modifications, as long as
# this notice is preserved.


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
