/* libnih
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef NIH_TEST_H
#define NIH_TEST_H

#include <stdio.h>
#include <setjmp.h>

#include <nih/macros.h>
#include <nih/alloc.h>


/**
 * INIT_TEST:
 * @_ret: variable to store failure in.
 *
 * Initialise the test framework so that tests may be used within and
 * beneath the scope of this call; normally this macro is required once
 * per function at the top.
 *
 * The int variable named in @_ret will be set to TRUE if any test fails,
 * otherwise left alone.
 **/
#define INIT_TEST(_ret) \
	int *_test_failed = &(_ret)

/**
 * CALL_TEST:
 * @_func: function to call.
 *
 * Call the test function @_func, which should accept no arguments and
 * return an integer; if TRUE then that function is considered to be
 * a test failure.
 **/
#define CALL_TEST(_func) \
	*_test_failed |= _func ()

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
 *
 * Also sets the test failure variable to TRUE.
 **/
#define TEST_FAILED(_fmt, ...) \
	do { \
		*_test_failed = TRUE; \
		printf ("BAD: " _fmt "\n\tat %s:%d (%s).\n", \
			##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
	} while (0)


/**
 * TEST_TRUE:
 * @_expr: value or expression to test.
 *
 * Check that the expression @_expr evaluates to TRUE.
 **/
#define TEST_TRUE(_expr) \
	if (! (_expr)) \
		TEST_FAILED ("wrong value for %s, expected TRUE got FALSE", \
			     #_expr)

/**
 * TEST_FALSE:
 * @_expr: value or expression to test.
 *
 * Check that the expression @_expr evaluates to FALSE.
 **/
#define TEST_FALSE(_expr) \
	if (_expr) \
		TEST_FAILED ("wrong value for %s, expected FALSE got TRUE", \
			     #_expr)

/**
 * TEST_EQ:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the two numeric values @_a and @_b are equal, they are cast
 * to ssize_t for display purposes.
 **/
#define TEST_EQ(_a, _b) \
	if ((_a) != (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zi got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_EQ_P:
 * @_a: first pointer,
 * @_b: second pointer.
 *
 * Check that the two pointers @_a and @_b are equal.
 **/
#define TEST_EQ_P(_a, _b) \
	if ((_a) != (_b)) \
		TEST_FAILED ("wrong value for %s, expected %p got %p", \
			     #_a, (_b), (_a))

/**
 * TEST_NE:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the two numeric values @_a and @_b are not equal, they are
 * cast to ssize_t for display purposes.
 **/
#define TEST_NE(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %zi", \
			     #_a, (ssize_t)(_b))

/**
 * TEST_NE_P:
 * @_a: first pointer,
 * @_b: second pointer.
 *
 * Check that the two pointers @_a and @_b are not equa.
 **/
#define TEST_NE_P(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %p", \
			     #_a, (_b))

/**
 * TEST_LT:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is less than the numeric value @_b,
 * they are cast to ssize_t for display purposes.
 **/
#define TEST_LT(_a, _b) \
	if ((_a) >= (_b)) \
		TEST_FAILED ("wrong value for %s, expected less than %zi got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_LE:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is less than or equal to the numeric
 * value @_b, they are cast to ssize_t for display purposes.
 **/
#define TEST_LE(_a, _b) \
	if ((_a) > (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zi or lower got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_GT:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is greater than the numeric value @_b,
 * they are cast to ssize_t for display purposes.
 **/
#define TEST_GT(_a, _b) \
	if ((_a) <= (_b)) \
		TEST_FAILED ("wrong value for %s, expected greater than %zi got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))

/**
 * TEST_GE:
 * @_a: first integer,
 * @_b: second integer.
 *
 * Check that the numeric value @_a is greater than or equal to the numeric
 * value @_b, they are cast to ssize_t for display purposes.
 **/
#define TEST_GE(_a, _b) \
	if ((_a) < (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zi or greater got %zi", \
			     #_a, (ssize_t)(_b), (ssize_t)(_a))


/**
 * TEST_ALLOC_SIZE:
 * @_ptr: allocated pointer,
 * @_sz: expected size.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc(), and is @_sz
 * bytes in length (which includes the context).
 **/
#define TEST_ALLOC_SIZE(_ptr, _sz) \
	if (nih_alloc_size (_ptr) != (_sz)) \
		TEST_FAILED ("wrong size of block %p, expected %zu got %zu", \
			     (_ptr), (size_t)(_sz), nih_alloc_size (_ptr))

/**
 * TEST_ALLOC_PARENT:
 * @_ptr: allocated pointer,
 * @_parent: expected parent.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc() and has
 * the other block @_parent as a parent.
 **/
#define TEST_ALLOC_PARENT(_ptr, _parent) \
	if (nih_alloc_parent (_ptr) != (_parent)) \
		TEST_FAILED ("wrong parent of block %p, expected %p got %p", \
			     (_ptr), (_parent), nih_alloc_parent (_ptr))


#endif /* NIH_TEST_H */
