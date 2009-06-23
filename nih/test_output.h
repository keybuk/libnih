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


/**
 * TEST_GROUP:
 * @_name: name of group being tested.
 *
 * Output a message indicating that a group of tests testing @_name are
 * being performed.
 **/
#define TEST_GROUP(_name) \
	printf ("Testing %s\n", _name)

/**
 * TEST_FUNCTION:
 * @_func: name of function being tested.
 *
 * Output a message indicating that tests of the function named @_func are
 * being performed.
 **/
#define TEST_FUNCTION(_func) \
	printf ("Testing %s()\n", _func)

/**
 * TEST_FUNCTION_FEATURE:
 * @_func: name of function being tested,
 * @_feat: feature being tested.
 *
 * Output a message indicating that tests of the function named @_func are
 * being performed, specifically of the @_feat feature.
 **/
#define TEST_FUNCTION_FEATURE(_func, _feat)			\
	printf ("Testing %s() %s\n", _func, _feat)

/**
 * TEST_FEATURE:
 * @_feat: name of function feature being tested.
 *
 * Output a message indicating that a sub-test of a function is being
 * performed, specifically the feature named _feat.
 **/
#define TEST_FEATURE(_feat) \
	printf ("...%s\n", _feat);

/**
 * TEST_FAILED:
 * @_fmt: format string.
 *
 * Output a formatted message indicating that a test has failed, including
 * the file, line number and function where the failure happened.
 **/
#define TEST_FAILED(_fmt, ...) \
	do { \
		printf ("BAD: " _fmt "\n\tat %s:%d (%s).\n", \
			##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
		abort (); \
	} while (0)

#endif /* NIH_TEST_OUTPUT_H */
