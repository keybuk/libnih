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

#ifndef NIH_HASH_H
#define NIH_HASH_H

/**
 * Provides a generic hash table implementation using NihList for the bins,
 * which means that entries may be freely moved between lists and hash
 * tables.
 *
 * Members are identified by a constant key, which is used for both hashing
 * and comparison.  The key function takes a given member (referenced by
 * its list head) and returns a pointer, this pointer is passed to the
 * hash function for hashing and the comparison function for comparison.
 *
 * The key, hash and comparison function are given when creating the hash
 * table with nih_hash_new().
 *
 * The most common use of this pointer is a string, generally a constant
 * one found as the first member in the structure after the list head.
 * For this case, you may use nih_hash_string_new() instead.
 *
 * Entries may be added to a hash table using nih_hash_add(), no assumption
 * is made about whether duplicate entries are permitted or not.  To add
 * and fail if the entry already exists use nih_hash_add_unique(), to add
 * and replace an existing entry use nih_hash_replace().
 *
 * The hash table may be iterated with nih_hash_search(), passing the return
 * value to subsequent calls iterates all values with the given key.
 *
 * To lookup the first value nih_hash_lookup() is a convenient simpler
 * function.
 **/

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihKeyFunction:
 * @entry: entry to key.
 *
 * This function is used to obtain a constant key for a given table entry.
 *
 * Returns: constant key from entry.
 **/
typedef const void *(*NihKeyFunction) (NihList *entry);

/**
 * NihHashFunction:
 * @key: key to hash.
 *
 * This function is used to generate a 32-bit hash for a given constant
 * key, this will be bounded by the hash size automatically.
 *
 * Returns: 32-bit hash.
 **/
typedef uint32_t (*NihHashFunction) (const void *key);

/**
 * NihCmpFunction:
 * @key1: key to compare,
 * @key2: key to compare against.
 *
 * This function is used to compare constant keys from two given table
 * entries.
 *
 * Returns: integer less than, equal to or greater than zero if @key1 is
 * respectively less then, equal to or greater than @key2.
 **/
typedef int (*NihCmpFunction) (const void *key1, const void *key2);


/**
 * NihHash:
 * @bins: array of bins,
 * @size: size of bins array,
 * @key_function: function used to obtain keys for entries,
 * @hash_function: function used to obtain hash of keys,
 * @cmp_function: function used to compare keys.
 *
 * This structure represents a hash table which is more efficient for
 * looking up members than an ordinary list.
 *
 * Individual members of the hash table are NihList members as are the
 * bins themselves, so to remove an entry from the table you can just
 * use nih_list_remove().
 **/
typedef struct nih_hash {
	NihList         *bins;
	size_t           size;

	NihKeyFunction   key_function;
	NihHashFunction  hash_function;
	NihCmpFunction   cmp_function;
} NihHash;


/**
 * NIH_HASH_FOREACH:
 * @hash: hash table to iterate,
 * @iter: name of iterator variable.
 *
 * Expands to nested for statements that iterate over each entry in each
 * bin of @hash, except the bin head pointer, setting @iter to each entry
 * for the block within the loop.  A variable named _@iter_i is used to
 * iterate the hash bins.
 *
 * This is the cheapest form of iteration, however it is not safe to perform
 * various modifications to the hash; most importantly, you must not change
 * the member being iterated in any way, including removing it from the hash
 * or freeing it.  If you need to do that, use NIH_HASH_FOREACH_SAFE() instead.
 *
 * However since it doesn't modify the hash being iterated in any way, it
 * is safe to traverse or iterate the hash again while iterating.
 **/
#define NIH_HASH_FOREACH(hash, iter)					\
	for (size_t _##iter##_i = 0; _##iter##_i < (hash)->size;	\
	     _##iter##_i++)						\
		NIH_LIST_FOREACH (&(hash)->bins[_##iter##_i], iter)

/**
 * NIH_HASH_FOREACH_SAFE:
 * @hash: hash table to iterate,
 * @iter: name of iterator variable.
 *
 * Expans to nested for statements that iterate over each entry in each
 * bin of @hash, except for the bin head pointer, setting @iter to each
 * entry for the block within the loop.  A variable named _@iter_i is used
 * to iterate the hash bins.
 *
 * The iteration is performed safely by placing a cursor node after @iter;
 * this means that any node including @iter can be removed from the hash,
 * added to a different hash or list, or entries added before or after it.
 *
 * Note that if you add an entry directly after @iter and wish it to be
 * visited, you would need to use NIH_HASH_FOREACH() instead, as this
 * would be placed before the cursor and thus skipped.
 *
 * Also since the hash has an extra node during iteration of a different
 * type, it is expressly not safe to traverse or iterate the hash while
 * iterating - including performing lookups.  If you need to perform
 * multiple iterations, lookups, or reference the next or previous pointers
 * of a node, you must use NIH_HASH_FOREACH().
 **/
#define NIH_HASH_FOREACH_SAFE(hash, iter)				\
	for (size_t _##iter##_i = 0; _##iter##_i < (hash)->size;	\
	     _##iter##_i++)						\
		NIH_LIST_FOREACH_SAFE (&(hash)->bins[_##iter##_i], iter)


/**
 * nih_hash_string_new:
 * @parent: parent of new hash,
 * @entries: rough number of entries expected,
 *
 * Allocates a new hash table, the number of buckets selected is a prime
 * number that is no larger than @entries; this should be set to a rough
 * number of expected entries to ensure optimum distribution.
 *
 * Individual members of the hash table are NihList member which have
 * a constant string as the first member that can be used as the hash key,
 * these will be compared case sensitively.
 *
 * The structure is allocated using nih_alloc() so it can be used as a
 * context to other allocations; there is no non-allocated version of this
 * function because the hash must be usable as a parent context to its bins.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned hash table.  When all parents
 * of the returned hash table are freed, the returned hash table will also be
 * freed.
 *
 * Returns: the new hash table or NULL if the allocation failed.
 **/
#define nih_hash_string_new(parent, entries)		     \
	nih_hash_new (parent, entries,			     \
		      (NihKeyFunction)nih_hash_string_key,   \
		      (NihHashFunction)nih_hash_string_hash, \
		      (NihCmpFunction)nih_hash_string_cmp)


NIH_BEGIN_EXTERN

NihHash *   nih_hash_new          (const void *parent, size_t entries,
				   NihKeyFunction key_function,
				   NihHashFunction hash_function,
				   NihCmpFunction cmp_function)
	__attribute__ ((warn_unused_result, malloc));

NihList *   nih_hash_add          (NihHash *hash, NihList *entry);
NihList *   nih_hash_add_unique   (NihHash *hash, NihList *entry);
NihList *   nih_hash_replace      (NihHash *hash, NihList *entry);

NihList *   nih_hash_search       (NihHash *hash, const void *key,
				   NihList *entry);
NihList *   nih_hash_lookup       (NihHash *hash, const void *key);

const char *nih_hash_string_key   (NihList *entry);
uint32_t    nih_hash_string_hash  (const char *key);
int         nih_hash_string_cmp   (const char *key1, const char *key2);

NIH_END_EXTERN

#endif /* NIH_HASH_H */
