/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_TEST_ALLOC_H
#define NIH_TEST_ALLOC_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <errno.h>
#include <stddef.h>

#include <nih/alloc.h>


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
 * test_alloc_failed:
 *
 * Variable used by TEST_ALLOC_FAIL as the loop counter.
 **/
static int test_alloc_failed = 0;

/**
 * _test_alloc_count:
 *
 * Number of times malloc is called by the TEST_ALLOC_FAIL macro.
 **/
static int _test_alloc_count = 0;

/**
 * _test_alloc_call:
 *
 * Number of times malloc has been called during each cycle.
 **/
static int _test_alloc_call = 0;

/**
 * _test_allocator:
 *
 * Allocator used by TEST_ALLOC_FAIL; when test_alloc_failed is zero, it
 * increments test_alloc_count and returns whatever realloc does.  Otherwise
 * it internally counts the number of times it is called, and if that matches
 * test_alloc_failed, then it returns NULL.
 **/
static inline  __attribute__ ((used)) void *
_test_allocator (void   *ptr,
		 size_t  size)
{
	if (! size)
		return realloc (ptr, size);

	if (! test_alloc_failed) {
		_test_alloc_count++;

		return realloc (ptr, size);
	}

	_test_alloc_call++;
	if (test_alloc_failed == _test_alloc_call) {
		errno = ENOMEM;
		return NULL;
	} else {
		return realloc (ptr, size);
	}
}

/**
 * TEST_ALLOC_FAIL:
 *
 * This macro expands to code that runs the following block repeatedly; the
 * first time (when the special test_alloc_failed variable is zero) is
 * used to determine how many allocations are performed by the following block;
 * subsequent calls (when test_alloc_failed is a positive integer) mean that
 * the test_alloc_failedth call to realloc has failed.
 *
 * This cannot be nested as it relies on setting an alternate allocator
 * and sharing a global state.
 **/
#define TEST_ALLOC_FAIL \
	for (test_alloc_failed = -1; \
	     test_alloc_failed <= (_test_alloc_count + 1); \
	     test_alloc_failed++, _test_alloc_call = 0) \
		if (test_alloc_failed < 0) { \
			_test_alloc_count = 0; \
			nih_alloc_set_allocator (_test_allocator); \
		} else if (test_alloc_failed \
			   && (test_alloc_failed == \
			       (_test_alloc_count + 1))) { \
			nih_alloc_set_allocator (realloc); \
		} else

/**
 * TEST_ALLOC_SAFE:
 *
 * This macro may be used within a TEST_ALLOC_FAIL block to guard the
 * following block of code from failing allocation.
 **/
#define TEST_ALLOC_SAFE \
	for (int _test_alloc_safe = 0; _test_alloc_safe < 3; \
	     _test_alloc_safe++) \
		if (_test_alloc_safe < 1) { \
			nih_alloc_set_allocator (realloc); \
		} else if (_test_alloc_safe > 1) { \
			nih_alloc_set_allocator (_test_allocator); \
		} else


/**
 * struct _test_free_tag:
 * @ptr: allocated block,
 * @tag: tag block.
 *
 * This structure is used to find out whether an nih_alloc() allocated block
 * is freed.  It works by pairing the allocated block with a tag block that
 * is an nih_alloc() child.  When that child is freed, this array is
 * cleared again.
 **/
struct _test_free_tag {
	void *ptr;
	void *tag;
};

/**
 * _test_free_tags:
 *
 * Array of suitable tag pairings, set the upper limit of this to taste.
 **/
static struct _test_free_tag _test_free_tags[1024] = { { NULL, NULL } };

/**
 * _test_free_tag:
 * @ptr: allocated block.
 *
 * Finds the tag pairing structure for @ptr in the array and returns it;
 * NULL can be used to find the first empty structure in the array for use
 * by a new tag.
 *
 * Return: tag structure for @ptr.
 **/
static inline struct _test_free_tag *
_test_free_tag (void *ptr)
{
	int i;

	for (i = 0; i < 1024; i++)
		if (_test_free_tags[i].ptr == ptr)
			return &(_test_free_tags[i]);

	return NULL;
}

/**
 * _test_destructor:
 * @tag: tag block.
 *
 * Destructor used to clear the tag for @tag and its parent block.
 *
 * Returns: zero
 **/
static int __attribute__ ((used))
_test_destructor (void *tag)
{
	struct _test_free_tag *_test_tag;

	_test_tag = _test_free_tag (nih_alloc_parent (tag));
	if (_test_tag)
		_test_tag->ptr = _test_tag->tag = NULL;

	return 0;
}


/**
 * TEST_FREE_TAG:
 * @_ptr: allocated block.
 *
 * This macro is used to tag an nih_alloc() allocated structure or block
 * to determine whether or not it is freed by code between it and either
 * TEST_FREED or TEST_NOT_FREED.
 **/
#define TEST_FREE_TAG(_ptr)						\
	do {								\
		struct _test_free_tag *_test_tag;			\
		assert ((_ptr) != NULL);				\
		_test_tag = _test_free_tag (NULL);			\
		_test_tag->ptr = (_ptr);				\
		_test_tag->tag = nih_alloc_using (realloc, (_ptr), 1);	\
		nih_alloc_set_destructor (_test_tag->tag, _test_destructor); \
	} while (0)

/**
 * TEST_FREE:
 * @_ptr: allocated block.
 *
 * Check that the data structure or block @_ptr was freed as expected; it
 * must have been first prepared by using TEST_FREE_TAG on it.
 **/
#define TEST_FREE(_ptr)						    \
	if (_test_free_tag (_ptr))				    \
		TEST_FAILED ("block %p (%s) not freed as expected", \
			     (_ptr), #_ptr)


/**
 * TEST_NOT_FREE:
 * @_ptr: allocated block.
 *
 * Check that the data structure or block @_ptr was not freed unexpectedly; it
 * must have been first prepared by using TEST_FREE_TAG on it.
 **/
#define TEST_NOT_FREE(_ptr)					 \
	if (! _test_free_tag (_ptr))				 \
		TEST_FAILED ("block %p (%s) freed unexpectedly", \
			     (_ptr), #_ptr)

#endif /* NIH_TEST_ALLOC_H */
