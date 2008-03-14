/* libnih
 *
 * Copyright © 2008 Scott James Remnant <scott@netsplit.com>.
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

#ifndef NIH_HASH_H
#define NIH_HASH_H

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
 * If you wish to modify the hash, e.g. remove entries, use
 * NIH_HASH_FOREACH_SAFE() instead.
 **/
#define NIH_HASH_FOREACH(hash, iter) \
	for (size_t _##iter##_i = 0; _##iter##_i < (hash)->size; \
	     _##iter##_i++) \
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
 * The iteration is performed safely by caching the next entry in the bin
 * in another variable (named _@iter); this means that @iter can be removed
 * from the bin, added to a different list or hash table, or entries added
 * before or after it.
 *
 * Note that if you wish an entry added after @iter to be visited, you
 * would need to use NIH_HASH_FOREACH() instead, as this would skip it.
 **/
#define NIH_HASH_FOREACH_SAFE(hash, iter) \
	for (size_t _##iter##_i = 0; _##iter##_i < (hash)->size; \
	     _##iter##_i++) \
		NIH_LIST_FOREACH_SAFE (&(hash)->bins[_##iter##_i], iter)


/**
 * nih_hash_pointer_new:
 * @parent: parent of new hash,
 * @entries: rough number of entries expected,
 *
 * Allocates a new hash table, the number of buckets selected is a prime
 * number that is no larger than @entries; this should be set to a rough
 * number of expected entries to ensure optimum distribution.
 *
 * Individual members of the hash table are unique NihList members, for
 * which their pointers will be used as the hash key and compared directly.
 *
 * The structure is allocated using nih_alloc() so it can be used as a
 * context to other allocations; there is no non-allocated version of this
 * function because the hash must be usable as a parent context to its bins.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.
 *
 * Returns: the new hash table or NULL if the allocation failed.
 **/
#define nih_hash_pointer_new(parent, entries)			\
	nih_hash_new (parent, entries, nih_hash_pointer_key,	\
		      nih_hash_pointer_hash, nih_hash_pointer_cmp)

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
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned block will be freed too.
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

const void *nih_hash_pointer_key  (NihList *entry);
uint32_t    nih_hash_pointer_hash (const void *key);
int         nih_hash_pointer_cmp  (const void *key1, const void *key2);

const char *nih_hash_string_key   (NihList *entry);
uint32_t    nih_hash_string_hash  (const char *key);
int         nih_hash_string_cmp   (const char *key1, const char *key2);

NIH_END_EXTERN

#endif /* NIH_HASH_H */
