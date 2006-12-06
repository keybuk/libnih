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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <sys/types.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>


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
 **/
#define TEST_FAILED(_fmt, ...) \
	do { \
		printf ("BAD: " _fmt "\n\tat %s:%d (%s).\n", \
			##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__); \
		abort (); \
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
 * TEST_EQ_STR:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are equal.
 **/
#define TEST_EQ_STR(_a, _b) \
	if (strcmp ((_a), (_b))) \
		TEST_FAILED ("wrong value for %s, expected '%s' got '%s'", \
			     #_a, (_b), (_a))

/**
 * TEST_EQ_MEM:
 * @_a: first memory area,
 * @_b: second memory area,
 * @_l: length of @_a and @_b.
 *
 * Check that the two @_l byte long areas of memory at @_a and @_b are
 * identical.
 **/
#define TEST_EQ_MEM(_a, _b, _l) \
	if (memcmp ((_a), (_b), (_l))) \
		TEST_FAILED ("wrong %d bytes at %p (%s), expected %p (%s)", \
			     (_l), (_a), #_a, (_b), #_b)

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
 * Check that the two pointers @_a and @_b are not equal.
 **/
#define TEST_NE_P(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %p", \
			     #_a, (_b))

/**
 * TEST_NE_STR:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are not equal.
 **/
#define TEST_NE_STR(_a, _b) \
	if (! strcmp ((_a), (_b))) \
		TEST_FAILED ("wrong value for %s, got unexpected '%s'", \
			     #_a, (_b))

/**
 * TEST_NE_MEM:
 * @_a: first memory area,
 * @_b: second memory area,
 * @_l: length of @_a and @_b.
 *
 * Check that the two @_l byte long areas of memory at @_a and @_b are
 * different.
 **/
#define TEST_NE_MEM(_a, _b, _l) \
	if (! memcmp ((_a), (_b), (_l))) \
		TEST_FAILED ("wrong %d bytes at %p (%s), got unexpected %p (%s)", \
			     (_l), (_a), #_a, (_b), #_b)

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
 * TEST_CHILD:
 * @_pid: variable to store pid in.
 *
 * Spawn a child in which a test can be performed without affecting the
 * main flow of the process.  The pid of the child is stored in @_pid.
 *
 * This macro ensures that the child has begun exectution before the
 * parent is allowed to continue through the usual use of a pipe.
 *
 * A block of code should follow this macro, which is the code that will
 * be run in the child process; if the block ends, the child will abort.
 **/
#define TEST_CHILD(_pid) \
	do { \
		int _test_fds[2]; \
		pipe (_test_fds); \
		_pid = fork (); \
		if (_pid > 0) { \
			char _test_buf[1]; \
			close (_test_fds[1]); \
			read (_test_fds[0], _test_buf, 1); \
			close (_test_fds[0]); \
		} else if (_pid == 0) { \
			close (_test_fds[0]); \
			write (_test_fds[1], "\n", 1); \
			close (_test_fds[1]); \
		} \
	} while (0); \
	if (_pid == 0) \
		for (int _test_half = 0; _test_half < 2; _test_half++) \
			if (_test_half) { \
				abort (); \
			} else

/**
 * TEST_FILENAME:
 * @_var: variable to store filename in.
 *
 * Generate a filename that may be used for testing, it's unlinked it if
 * exists and it's up to you to unlink it when done.  @_var should be at
 * least PATH_MAX long.
 **/
#define TEST_FILENAME(_var) \
	snprintf ((_var), sizeof (_var), "/tmp/%s:%s:%d:%d", \
		  strrchr (__FILE__, '/') ? strrchr (__FILE__, '/') + 1 : __FILE__, \
		  __FUNCTION__, __LINE__, getpid ()); \
	unlink (_var)


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
		TEST_FAILED ("wrong size of block %p (%s), expected %zu got %zu", \
			     (_ptr), #_ptr, (size_t)(_sz), \
			     nih_alloc_size (_ptr))

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
		TEST_FAILED ("wrong parent of block %p (%s), expected %p (%s) got %p", \
			     (_ptr), #_ptr, (_parent), #_parent, \
			     nih_alloc_parent (_ptr))


/**
 * TEST_LIST_EMPTY:
 * @_list: entry in list.
 *
 * Check that the list of which @_list is a member is empty, ie. that
 * @_list is the sole member.
 **/
#define TEST_LIST_EMPTY(_list) \
	if (! NIH_LIST_EMPTY (_list)) \
		TEST_FAILED ("list %p (%s) not empty as expected", \
			     (_list), #_list)

/**
 * TEST_LIST_NOT_EMPTY:
 * @_list: entry in list.
 *
 * Check that the list of which @_list is a member is not empty, ie. that
 * there are more members than just @_list.
 **/
#define TEST_LIST_NOT_EMPTY(_list) \
	if (NIH_LIST_EMPTY (_list)) \
		TEST_FAILED ("list %p (%s) empty, expected multiple members", \
			     (_list), #_list)


#endif /* NIH_TEST_H */
