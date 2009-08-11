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

#ifndef NIH_TEST_ALLOC_H
#define NIH_TEST_ALLOC_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <errno.h>
#include <stddef.h>

#include <nih/alloc.h>
#include <nih/list.h>


/* When testing, we need to be able to override the malloc, realloc and free
 * functions called by nih_alloc().  We can't use libc malloc hooks because
 * valgrind doesn't implement them - and I like valgrind.
 */
extern void *(*__nih_malloc)(size_t size);
extern void *(*__nih_realloc)(void *ptr, size_t size);
extern void (*__nih_free)(void *ptr);


/**
 * TEST_ALLOC_SIZE:
 * @_ptr: allocated pointer,
 * @_sz: expected size.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc(), and has
 * enough space for at least @_sz bytes.
 **/
#define TEST_ALLOC_SIZE(_ptr, _sz)					\
	if ((_ptr) == NULL) {						\
		TEST_FAILED ("wrong value for block %s, got unexpected NULL", \
			     #_ptr);					\
	} else if (nih_alloc_size (_ptr) < (_sz))			\
		TEST_FAILED ("wrong size of block %p (%s), expected %zu got %zu", \
			     (_ptr), #_ptr, (size_t)(_sz),		\
			     nih_alloc_size (_ptr))

/**
 * TEST_ALLOC_PARENT:
 * @_ptr: allocated pointer,
 * @_parent: expected parent.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc() and has
 * the other block @_parent as a parent.  @_parent may be the special
 * NULL parent.
 **/
#define TEST_ALLOC_PARENT(_ptr, _parent)				\
	if ((_ptr) == NULL) {						\
		TEST_FAILED ("wrong value for block %s, got unexpected NULL", \
			     #_ptr);					\
	} else if (! nih_alloc_parent ((_ptr), (_parent)))		\
		TEST_FAILED ("wrong parent of block %p (%s), expected %p (%s)",	\
			     (_ptr), #_ptr, (_parent), #_parent)

/**
 * TEST_ALLOC_NOT_PARENT:
 * @_ptr: allocated pointer,
 * @_parent: expected non-parent.
 *
 * Check that the pointer @_ptr was allocated with nih_alloc() and does not
 * have the other block @_parent as a parent.  @_parent may be the special
 * NULL parent.
 **/
#define TEST_ALLOC_NOT_PARENT(_ptr, _parent)				\
	if ((_ptr) == NULL) {						\
		TEST_FAILED ("wrong value for block %s, got unexpected NULL", \
			     #_ptr);					\
	} else if (nih_alloc_parent ((_ptr), (_parent)))		\
		TEST_FAILED ("wrong parent of block %p (%s), got unexpected %p (%s)",	\
			     (_ptr), #_ptr, (_parent), #_parent)


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
 * _test_realloc:
 *
 * realloc() wrapper used by TEST_ALLOC_FAIL.
 *
 * When test_alloc_failed is zero, it increments test_alloc_count and returns
 * whatever realloc does.  Otherwise it internally counts the number of times
 * it is called, and if that matches test_alloc_failed, then it returns NULL.
 **/
static inline  __attribute__ ((used)) void *
_test_realloc (void   *ptr,
	       size_t  size)
{
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
 * _test_malloc:
 *
 * malloc() wrapped used by TEST_ALLOC_FAIL.
 *
 * Calls _test_realloc with a NULL pointer.
 **/
static inline __attribute__ ((used)) void *
_test_malloc (size_t size)
{
	return _test_realloc (NULL, size);
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
#define TEST_ALLOC_FAIL						   \
	for (test_alloc_failed = -1, _test_alloc_count = 0;	   \
	     test_alloc_failed <= (_test_alloc_count + 1);	   \
	     test_alloc_failed++, _test_alloc_call = 0)		   \
		if (test_alloc_failed < 0) {			   \
			__nih_malloc = _test_malloc;		   \
			__nih_realloc = _test_realloc;		   \
		} else if (test_alloc_failed			   \
			   && (test_alloc_failed ==		   \
			       (_test_alloc_count + 1))) {	   \
			__nih_malloc = malloc;			   \
			__nih_realloc = realloc;		   \
		} else

/**
 * TEST_ALLOC_SAFE:
 *
 * This macro may be used within a TEST_ALLOC_FAIL block to guard the
 * following block of code from failing allocation.
 **/
#define TEST_ALLOC_SAFE						   \
	for (int _test_alloc_safe = 0; _test_alloc_safe < 3;	   \
	     _test_alloc_safe++)				   \
		if (_test_alloc_safe < 1) {			   \
			__nih_malloc = malloc;			   \
			__nih_realloc = realloc;		   \
		} else if (_test_alloc_safe > 1) {		   \
			__nih_malloc = _test_malloc;		   \
			__nih_realloc = _test_realloc;		   \
		} else



/**
 * struct _test_free_tag:
 * @entry: list entry,
 * @ptr: tagged object.
 *
 * This structure is used to find out whether an nih_alloc() allocated object
 * has been freed or not.  It works by being allocated as a child of the
 * tagged object, and added to a linked list of known tags.  When freed,
 * it is removed from the linked list.
 **/
struct _test_free_tag {
	NihList  entry;
	void    *ptr;
};

/**
 * _test_free_tags:
 *
 * Linked list of tagged blocks.
 **/
static NihList _test_free_tags = { NULL, NULL };

/**
 * _test_free_tag:
 * @ptr: tagged object.
 *
 * Returns: TRUE if @ptr is tagged (not freed), FALSE if not (freed).
 **/
static inline int
_test_free_tag (void *ptr)
{
	NIH_LIST_FOREACH (&_test_free_tags, iter) {
		struct _test_free_tag *tag = (struct _test_free_tag *)iter;

		if (tag->ptr == ptr)
			return TRUE;
	}

	return FALSE;
}


/**
 * TEST_FREE_TAG:
 * @_ptr: allocated object.
 *
 * This macro is used to tag an nih_alloc() allocated object to determine
 * whether or not it is freed.  It works by allocating a child object of
 * @_ptr and storing it in a linked list.
 *
 * This can be tested with either the TEST_FREE or TEST_NOT_FREE macros as
 * many times as you like.
 **/
#define TEST_FREE_TAG(_ptr)						\
	do {								\
		void *(*_test__nih_malloc)(size_t size) = __nih_malloc; \
		struct _test_free_tag *_test_tag;			\
									\
		__nih_malloc = malloc;					\
		_test_tag = nih_new ((_ptr), struct _test_free_tag);	\
		assert ((_ptr) != NULL);				\
		__nih_malloc = _test__nih_malloc;			\
									\
		nih_list_init (&_test_tag->entry);			\
		_test_tag->ptr = (_ptr);				\
		nih_alloc_set_destructor (_test_tag, nih_list_destroy); \
									\
		if (! _test_free_tags.next)				\
			nih_list_init (&_test_free_tags);		\
		nih_list_add (&_test_free_tags, &_test_tag->entry);	\
	} while (0)

/**
 * TEST_FREE:
 * @_ptr: allocated object.
 *
 * Check that the nih_alloc() allocated object @_ptr was freed as expected; it
 * must have been first prepared by using TEST_FREE_TAG on it otherwise this
 * will always fail.
 **/
#define TEST_FREE(_ptr)						    \
	if (_test_free_tag (_ptr))				    \
		TEST_FAILED ("block %p (%s) not freed as expected", \
			     (_ptr), #_ptr)


/**
 * TEST_NOT_FREE:
 * @_ptr: allocated block.
 *
 * Check that the nih_alloc() allocated object @_ptr was not freed
 * unexpectedly; it must have been first prepared by using TEST_FREE_TAG
 * on it otherwise this will always succeed.
 **/
#define TEST_NOT_FREE(_ptr)					 \
	if (! _test_free_tag (_ptr))				 \
		TEST_FAILED ("block %p (%s) freed unexpectedly", \
			     (_ptr), #_ptr)

#endif /* NIH_TEST_ALLOC_H */
