/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef NIH_TEST_OUTPUT_H
#define NIH_TEST_OUTPUT_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include <nih/macros.h>

static char TEST_BLOCK_NAME[PATH_MAX] = "" ;
static char TEST_FEATURE_NAME[PATH_MAX] = "" ;
static int TEST_COUNT = 0;
static int TEST_PLAN_CALLED = FALSE;
static char TEST_EXIT_LOCK[PATH_MAX] = "";
static inline void print_last ( void );

/**
 * TEST_PLAN:
 * @_num: number of planned tests to run
 *
 * Output the number of tests expected to be executed. This can be
 * declared only once, at the beginning or the end of the test output.
 *
 * If not called, an exit plan with total number of executed tests
 * will be printed.
 **/
#define TEST_PLAN(_num) \
	printf ("1..%d\n", _num); TEST_PLAN_CALLED = TRUE

/**
 * TEST_PLAN_END:
 *
 * Output the number of tests that got executed.
 **/
#define TEST_PLAN_END() \
	if (! TEST_PLAN_CALLED) printf ("1..%d\n", TEST_COUNT)

/**
 * TEST_PRINT_RESULT(_ok)
 * @_ok: test result
 *
 * Output result of last run function if any.
 **/
#define TEST_PRINT_RESULT(_ok)						\
	if (TEST_COUNT == 0) {						\
		sprintf (TEST_EXIT_LOCK, "%d", getpid() );		\
		setenv ("TEST_EXIT_LOCK", TEST_EXIT_LOCK, 1);		\
		atexit (print_last);					\
	}								\
        if (TEST_COUNT > 0) printf ("%s %d - %s %s\n", _ok ? "ok" : "not ok", TEST_COUNT, TEST_BLOCK_NAME, TEST_FEATURE_NAME)

/**
 * print_last:
 *
 * atexit handler to print the last test result, and exit plan if needed.
 **/
static inline void
print_last ( void )
{
	if ( atoi (getenv ("TEST_EXIT_LOCK")) == getpid ()) {
		TEST_PRINT_RESULT (TRUE);
		TEST_PLAN_END();
	}
}


/**
 * TEST_GROUP:
 * @_name: name of group being tested.
 *
 * Output a message indicating that a group of tests testing @_name are
 * being performed.
 **/
#define TEST_GROUP(_name) TEST_PRINT_RESULT (TRUE); TEST_COUNT++; strcpy(TEST_BLOCK_NAME, _name); strcpy(TEST_FEATURE_NAME, "")

/**
 * TEST_FUNCTION:
 * @_func: name of function being tested.
 *
 * Output a message indicating that tests of the function named @_func are
 * being performed.
 **/
#define TEST_FUNCTION(_func) TEST_PRINT_RESULT (TRUE); TEST_COUNT++; strcpy(TEST_BLOCK_NAME, _func); strcat(TEST_BLOCK_NAME, "()"); strcpy(TEST_FEATURE_NAME, "")

/**
 * TEST_FUNCTION_FEATURE:
 * @_func: name of function being tested,
 * @_feat: feature being tested.
 *
 * Output a message indicating that tests of the function named @_func are
 * being performed, specifically of the @_feat feature.
 **/
#define TEST_FUNCTION_FEATURE(_func, _feat) TEST_GROUP(_func##_feat)

/**
 * TEST_FEATURE:
 * @_feat: name of function or group feature being tested.
 *
 * Output a message indicating that a sub-test of a function or
 * group is being performed, specifically the feature named _feat.
 **/
#define TEST_FEATURE(_feat) TEST_PRINT_RESULT (TRUE); TEST_COUNT++; strcpy(TEST_FEATURE_NAME, _feat)

/**
 * TEST_FAILED:
 * @_fmt: format string.
 *
 * Output a formatted message indicating that a test has failed, including
 * the file, line number and function where the failure happened.
 **/
#ifdef TEST_TAP_BAIL_ON_FAIL
#define TEST_BAIL() printf ("Bail out!\n"); abort ()
#else
#define TEST_BAIL() {}
#endif

#define TEST_FAILED(_fmt, ...)					       \
	do {							       \
		if (++TEST_COUNT)				       \
			TEST_PRINT_RESULT(FALSE);		       \
		printf (" BAD: " _fmt "\n\tat %s:%d (%s).\n",		\
			##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
		TEST_BAIL();						\
	} while (0)

#endif /* NIH_TEST_OUTPUT_H */
