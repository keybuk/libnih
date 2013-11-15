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
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include <nih/macros.h>

static char TEST_NAME[PATH_MAX]="";
static size_t TEST_COUNT = 0;
static int TEST_PLAN_CALLED = FALSE;


/* Forward weak declarations of functions. This header is protected
   from multiple inclusions, but that protection is only for a single
   translation unit. With weak symbols, however, we can link multiple
   objects that included this header without causing multiple
   declarations error.
*/
void TEST_PRINT_RESULT(const char *)
	__attribute__((weak));

void print_last ( void )
	__attribute__((weak));

#define OK "ok"
#define BAD "not ok"
#define TEST_PRINT_OK() TEST_PRINT_RESULT (OK)
#define TEST_PRINT_BAD() TEST_PRINT_RESULT (BAD)

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
	if (! TEST_PLAN_CALLED) printf ("1..%zu\n", TEST_COUNT)

/**
 * TEST_PRINT_RESULT(ok)
 * @ok: test result
 *
 * Output result of last run function if any.
 **/
void
TEST_PRINT_RESULT(const char * ok)
{		
	if (strlen(TEST_NAME) > 0) {
		printf ("%s %zu - %s\n", ok, TEST_COUNT, TEST_NAME);
		return;
	}

	static int initialised = 0;

	if (! initialised) {
		char TEST_EXIT_LOCK[30] = "";
		sprintf (TEST_EXIT_LOCK, "%d", getpid() );		
		setenv ("TEST_EXIT_LOCK", TEST_EXIT_LOCK, 1);		
		atexit (print_last);
		initialised = 1;
	}
}

/**
 * print_last:
 *
 * atexit handler to print the last test result, and exit plan if needed.
 **/
void
print_last ( void )
{
	if ( atoi (getenv ("TEST_EXIT_LOCK")) == getpid ()) {
		TEST_PRINT_RESULT ("ok");
		TEST_PLAN_END();
	}
}

/**
 * TEST_NAME:
 * @_name: name of group being tested.
 *
 * Output a message indicating that a group of tests testing @_name are
 * being performed.
 **/
#define TEST_START(_name) TEST_PRINT_OK(); strncpy(TEST_NAME, _name, PATH_MAX-1); TEST_COUNT++;

#ifdef NIH_TAP_OUTPUT
#define TEST_FAILED_ABORT() {}
#else
#define TEST_FAILED_ABORT() abort ()
#endif

/**
 * TEST_FAILED:
 * @_fmt: format string.
 *
 * Output a formatted message indicating that a test has failed, including
 * the file, line number and function where the failure happened.
 **/
#define TEST_FAILED(_fmt, ...)						\
	do {								\
		TEST_PRINT_BAD(); strcpy(TEST_NAME, "");		\
		printf ("\t" _fmt "\n\tat %s:%d (%s).\n", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
		TEST_FAILED_ABORT ();					\
	} while (0)

#define TEST_GROUP TEST_START
#define TEST_FUNCTION TEST_START
#define TEST_FEATURE TEST_START
#define TEST_FUNCTION_FEATURE(_func, _feat) TEST_START(_func); TEST_START(_feat)

#endif /* NIH_TEST_OUTPUT_H */
