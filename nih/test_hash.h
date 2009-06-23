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
