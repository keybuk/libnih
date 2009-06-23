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

#ifndef NIH_TEST_VALUES_H
#define NIH_TEST_VALUES_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <assert.h>
#include <stddef.h>
#include <string.h>


/**
 * assert0:
 * @_expr: expression to check.
 *
 * Wrapper around the usual assert() function that handles the common case
 * of asserting that @_expr returns zero, rather than a TRUE value.
 **/
#define assert0(_expr) \
	assert ((_expr) == 0)


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
 * TEST_EQ_U:
 * @_a: first unsigned integer,
 * @_b: second unsigned integer.
 *
 * Check that the two numeric values @_a and @_b are equal, they are cast
 * to size_t for display purposes.
 **/
#define TEST_EQ_U(_a, _b) \
	if ((_a) != (_b)) \
		TEST_FAILED ("wrong value for %s, expected %zu got %zu", \
			     #_a, (size_t)(_b), (size_t)(_a))

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
	if ((_a) == NULL) {						\
		TEST_FAILED ("wrong value for %s, expected '%s' got NULL", \
			     #_a, (_b));				\
	} else if (strcmp ((_a), (_b)))					\
		TEST_FAILED ("wrong value for %s, expected '%s' got '%s'", \
			     #_a, (_b), (_a))

/**
 * TEST_EQ_STRN:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are equal, up to the length of
 * the second string.
 **/
#define TEST_EQ_STRN(_a, _b) \
	if ((_a) == NULL) {						\
		TEST_FAILED ("wrong value for %s, expected '%.*s' got NULL", \
			     #_a, (int)strlen (_b), (_b));		\
	} else if (strncmp ((_a), (_b), strlen (_b)))			\
		TEST_FAILED ("wrong value for %s, expected '%.*s' got '%.*s'", \
			     #_a, (int)strlen (_b), (_b),		\
			     (int)strlen (_b), (_a))

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
	if ((_a) == NULL) {						\
		TEST_FAILED ("wrong value for %s, got unexpected NULL",	\
			     #_a);					\
	} else if (memcmp ((_a), (_b), (_l)))				\
		TEST_FAILED ("wrong %zu bytes at %p (%s), expected %p (%s)", \
			     (size_t)(_l), (_a), #_a, (_b), #_b)

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
 * TEST_NE_U:
 * @_a: first unsigned integer,
 * @_b: second unsigned integer.
 *
 * Check that the two numeric values @_a and @_b are not equal, they are
 * cast to size_t for display purposes.
 **/
#define TEST_NE_U(_a, _b) \
	if ((_a) == (_b)) \
		TEST_FAILED ("wrong value for %s, got unexpected %zu", \
			     #_a, (size_t)(_b))

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
	if ((_a) == NULL) {						\
		TEST_FAILED ("wrong value for %s, expected string got NULL", \
			     #_a);					\
	} else if (! strcmp ((_a), (_b)))				\
		TEST_FAILED ("wrong value for %s, got unexpected '%s'", \
			     #_a, (_b))

/**
 * TEST_NE_STRN:
 * @_a: first string,
 * @_b: second string.
 *
 * Check that the two strings @_a and @_b are not equal, up to the length
 * of the second string.
 **/
#define TEST_NE_STRN(_a, _b) \
	if ((_a) == NULL) {						\
		TEST_FAILED ("wrong value for %s, got unexpected NULL",	\
			     #_a);					\
	} else if (! strncmp ((_a), (_b), strlen (_b)))			\
		TEST_FAILED ("wrong value for %s, got unexpected '%.*s'", \
			     #_a, (int)strlen (_b), (_b))

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
	if ((_a) == NULL) {						\
		TEST_FAILED ("wrong value for %s, got unexpected NULL",	\
			     #_a);					\
	} else if (! memcmp ((_a), (_b), (_l)))				\
		TEST_FAILED ("wrong %zu bytes at %p (%s), got unexpected %p (%s)", \
			     (size_t)(_l), (_a), #_a, (_b), #_b)

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

#endif /* NIH_TEST_VALUES_H */
