/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
