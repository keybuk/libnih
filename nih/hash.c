/* libnih
 *
 * hash.c - Fuller/Noll/Vo hash table implementation
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <string.h>

#include <nih/macros.h>
#include <nih/logging.h>
#include <nih/alloc.h>

#include "hash.h"


/**
 * FNV_PRIME:
 *
 * This constant is defined in the FNV description based on the size of
 * the hash, in our case 32-bits.
 **/
#define FNV_PRIME        16777619UL

/**
 * FNV_OFFSET_BASIS:
 *
 * This constant is also defined in the FNV description and is the result
 * of hashing a known string wth the FNV-0 algorithm and the above prime.
 **/
#define FNV_OFFSET_BASIS 2166136261UL


/**
 * primes:
 *
 * Prime numbers always give the best hash table sizes, this is a selected
 * list of primes giving a reasonable spread.  We pick the largest one that
 * is smaller than the estimated number of entries for the hash.
 **/
static const uint32_t primes[] = {
	17, 37, 79, 163, 331, 673, 1259, 2521, 5051, 10103, 20219, 40459,
	80929, 160231, 320449, 640973, 1281563, 2566637, 5136083, 10250323
};

/**
 * num_primes:
 *
 * Number of prime numbers defined above.
 **/
static const size_t num_primes = sizeof (primes) / sizeof (uint32_t);


/**
 * nih_hash_new:
 * @parent: parent of new hash,
 * @entries: rough number of entries expected,
 * @key_function: function used to obtain keys for entries,
 * @hash_function: function used to obtain hash for keys,
 * @cmp_function: function used to compare keys.
 *
 * Allocates a new hash table, the number of buckets selected is a prime
 * number that is no larger than @entries; this should be set to a rough
 * number of expected entries to ensure optimum distribution.
 *
 * Individual members of the hash table are NihList members, so to
 * associate them with a constant key @key_function must be provided, to
 * convert that key into a hash @hash_function must be provided and to
 * compare keys @cmp_function must be provided.  The nih_hash_string_new()
 * macro wraps this function for the most common case of a string key as
 * the first structure member.
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
NihHash *
nih_hash_new (const void      *parent,
	      size_t           entries,
	      NihKeyFunction   key_function,
	      NihHashFunction  hash_function,
	      NihCmpFunction   cmp_function)
{
	NihHash *hash;
	size_t   i;

	nih_assert (key_function != NULL);
	nih_assert (hash_function != NULL);
	nih_assert (cmp_function != NULL);

	hash = nih_new (parent, NihHash);
	if (! hash)
		return NULL;

	/* Pick the largest prime number smaller than the number of entries */
	hash->size = primes[0];
	for (i = 0; (i < num_primes) && (primes[i] < entries); i++)
		hash->size = primes[i];

	/* Allocate bins */
	hash->bins = nih_alloc (hash, sizeof (NihList) * hash->size);
	if (! hash->bins) {
		nih_free (hash);
		return NULL;
	}

	/* Initialise bins */
	for (i = 0; i < hash->size; i++)
		nih_list_init (&hash->bins[i]);

	hash->key_function = key_function;
	hash->hash_function = hash_function;
	hash->cmp_function = cmp_function;

	return hash;
}


/**
 * nih_hash_add:
 * @hash: destination hash table,
 * @entry: entry to be added.
 *
 * Adds @entry to @hash using the value returned by the hash functions
 * to indicate which bin the entry should be placed into.
 *
 * For speed reasons, this function does not check whether an entry already
 * exists with the key.  If you need that constraint use either
 * nih_hash_add_unique() or nih_hash_replace().
 *
 * If @entry is already in another list it is removed so there is no need
 * to call nih_list_remove() before this function.
 *
 * Returns: @entry which is now a member of one of @hash's bins.
 **/
NihList *
nih_hash_add (NihHash *hash,
	      NihList *entry)
{
	const void *key;
	uint32_t    hashval;
	NihList    *bin;

	nih_assert (hash != NULL);
	nih_assert (entry != NULL);

	key = hash->key_function (entry);
	hashval = hash->hash_function (key) % hash->size;
	bin = &hash->bins[hashval];

	return nih_list_add (bin, entry);
}

/**
 * nih_hash_add_unique:
 * @hash: destination hash table,
 * @entry: entry to be added.
 *
 * Adds @entry to @hash using the value returned by the hash functions
 * to indicate which bin the entry should be placed into, provided the key
 * is unique.
 *
 * Because the hash table does not store the key of each entry, this requires
 * that the key function be called for each entry in the destination bin, so
 * should only be used where the uniqueness constraint is required and not
 * already enforced by other code.
 *
 * If @entry is already in another list it is removed so there is no need
 * to call nih_list_remove() before this function.
 *
 * Returns: @entry which is now a member of one of @hash's bins or NULL if
 * an entry already existed with the same key.
 **/
NihList *
nih_hash_add_unique (NihHash *hash,
		     NihList *entry)
{
	const void *key;
	uint32_t    hashval;
	NihList    *bin;

	nih_assert (hash != NULL);
	nih_assert (entry != NULL);

	key = hash->key_function (entry);
	hashval = hash->hash_function (key) % hash->size;
	bin = &hash->bins[hashval];

	NIH_LIST_FOREACH (bin, iter) {
		if (! hash->cmp_function (key, hash->key_function (iter)))
			return NULL;
	}

	return nih_list_add (bin, entry);
}

/**
 * nih_hash_replace:
 * @hash: destination hash table,
 * @entry: entry to be added.
 *
 * Adds @entry to @hash using the value returned by the hash functions
 * to indicate which bin the entry should be placed into, replacing any
 * existing entry with the same key.
 *
 * Because the hash table does not store the key of each entry, this requires
 * that the key function be called for each entry in the destination bin, so
 * should only be used where the uniqueness constraint is required and not
 * already enforced by other code.
 *
 * The replaced entry is returned, it is up to the caller to free it and
 * ensure this does not come as a surprise to other code.
 *
 * If @entry is already in another list it is removed so there is no need
 * to call nih_list_remove() before this function.
 *
 * Returns: existing entry with the same key replaced in the table, or NULL
 * if no such entry existed.
 **/
NihList *
nih_hash_replace (NihHash *hash,
		  NihList *entry)
{
	const void *key;
	uint32_t    hashval;
	NihList    *bin, *ret = NULL;

	nih_assert (hash != NULL);
	nih_assert (entry != NULL);

	key = hash->key_function (entry);
	hashval = hash->hash_function (key) % hash->size;
	bin = &hash->bins[hashval];

	NIH_LIST_FOREACH (bin, iter) {
		if (! hash->cmp_function (key, hash->key_function (iter))) {
			ret = nih_list_remove (iter);
			break;
		}
	}

	nih_list_add (bin, entry);

	return ret;
}


/**
 * nih_hash_search:
 * @hash: hash table to search,
 * @key: key to look for,
 * @entry: previous entry found.
 *
 * Finds all entries in @hash with a key of @key by calling the hash's
 * key function on each entry in the appropriate bin, starting with @entry,
 * until one is found.
 *
 * The initial @entry can be found by passing NULL or using nih_hash_lookup().
 *
 * Returns: next entry in the hash or NULL if there are no more entries.
 **/
NihList *
nih_hash_search (NihHash    *hash,
		 const void *key,
		 NihList    *entry)
{
	uint32_t  hashval;
	NihList  *bin;

	nih_assert (hash != NULL);
	nih_assert (key != NULL);

	hashval = hash->hash_function (key) % hash->size;
	bin = &hash->bins[hashval];

	NIH_LIST_FOREACH (bin, iter) {
		if (iter == entry) {
			entry = NULL;
			continue;
		} else if (entry) {
			continue;
		} else if (! hash->cmp_function (key, hash->key_function (iter))) {
			return iter;
		}
	}

	return NULL;
}

/**
 * nih_hash_lookup:
 * @hash: hash table to search.
 * @key: key to look for.
 *
 * Finds the first entry in @hash with a key of @key by calling the hash's
 * NihKeyFunction on each entry in the appropriate bin until one is found.
 *
 * If multiple entries are expected, use nih_hash_search() instead.
 *
 * Returns: entry found or NULL if no entry existed.
 **/
NihList *
nih_hash_lookup (NihHash    *hash,
		 const void *key)
{
	return nih_hash_search (hash, key, NULL);
}


/**
 * nih_hash_string_key:
 * @entry: entry to create key for.
 *
 * Key function that can be used for any list entry where the first member
 * immediately after the list header is a pointer to the string containing
 * the name.
 *
 * Returns: pointer to that string.
 **/
const char *
nih_hash_string_key (NihList *entry)
{
	nih_assert (entry != NULL);

	return *((const char **)((char *)entry + sizeof (NihList)));
}

/**
 * nih_hash_string_hash:
 * @key: string key to hash.
 *
 * Generates and returns a 32-bit hash for the given string key using the
 * FNV-1 algorithm as documented at http://www.isthe.com/chongo/tech/comp/fnv/
 *
 * The returned key will need to be bounded within the number of bins
 * used in the hash table.
 *
 * Returns: 32-bit hash.
 **/
uint32_t
nih_hash_string_hash (const char *key)
{
	register uint32_t hash = FNV_OFFSET_BASIS;

	nih_assert (key != NULL);

	while (*key) {
		hash *= FNV_PRIME;
		hash ^= *(key++);
	}

	return hash;
}

/**
 * nih_hash_string_cmp:
 * @key1: key to compare,
 * @key2: key to compare against.
 *
 * Compares @key1 to @key2 case-sensitively.
 *
 * Returns: integer less than, equal to or greater than zero if @key1 is
 * respectively less then, equal to or greater than @key2.
 **/
int
nih_hash_string_cmp (const char *key1,
		     const char *key2)
{
	nih_assert (key1 != NULL);
	nih_assert (key2 != NULL);

	return strcmp (key1, key2);
}
