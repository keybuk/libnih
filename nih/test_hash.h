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

#ifndef NIH_TEST_HASH_H
#define NIH_TEST_HASH_H

#ifndef NIH_IN_TEST_H
# error "This header may only be included by <nih/test.h>"
#endif /* NIH_IN_TEST_H */

#include <stddef.h>

#include <nih/list.h>


/**
 * TEST_HASH_EMPTY:
 * @_hash: hash table.
 *
 * Check that the hash table @_hash is empty.
 **/
#define TEST_HASH_EMPTY(_hash) \
	for (size_t _hash_i = 0; _hash_i < (_hash)->size; _hash_i++) \
		if (! NIH_LIST_EMPTY (&(_hash)->bins[_hash_i])) \
			TEST_FAILED ("hash %p (%s) not empty as expected", \
				     (_hash), #_hash)

/**
 * TEST_HASH_NOT_EMPTY:
 * @_list: entry in list.
 *
 * Check that the hash table @_hash is not empty.
 **/
#define TEST_HASH_NOT_EMPTY(_hash) \
	do { \
		int _hash_empty = 1; \
		for (size_t _hash_i = 0; _hash_i < (_hash)->size; _hash_i++) \
			if (! NIH_LIST_EMPTY (&(_hash)->bins[_hash_i])) \
				_hash_empty = 0; \
		if (_hash_empty) \
			TEST_FAILED ("hash %p (%s) empty, expected multiple members", \
				     (_hash), #_hash); \
	} while (0)

#endif /* NIH_TEST_HASH_H */
