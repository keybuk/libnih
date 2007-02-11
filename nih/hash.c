/* libnih
 *
 * hash.c - Fuller/Noll/Vo hash table implementation
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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


/* Prototypes for static functions */
static uint32_t fnv_hash (const char *key);


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
 * fnv_hash:
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
static uint32_t
fnv_hash (const char *key)
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
 * nih_hash_new:
 * @parent: parent of new hash,
 * @entries: rough number of entries expected,
 * @key_function: function used to obtain keys for entries.
 *
 * Allocates a new hash table, the number of buckets selected is a prime
 * number that is no larger than @entries; this should be set to a rough
 * number of expected entries to ensure optimum distribution.
 *
 * Individual members of the hash table are NihList members, so to
 * associate them with a string key @key_function must be provided; this
 * would ordinarily just return a static string within the entry itself.
 *
 * The structure is allocated using nih_alloc() so it can be used as a
 * context to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the new hash table or NULL if the allocation failed.
 **/
NihHash *
nih_hash_new (const void     *parent,
	      size_t          entries,
	      NihKeyFunction  key_function)
{
	NihHash *hash;
	size_t   i;

	nih_assert (key_function != NULL);

	hash = nih_new (parent, NihHash);
	if (! hash)
		return NULL;

	/* Pick a prime number larger than the number of entries */
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

	return hash;
}


/**
 * nih_hash_add:
 * @hash: destination hash table,
 * @entry: entry to be added.
 *
 * Adds @entry to @hash using the value returned by the hash NihKeyFunction
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
	const char *key;
	NihList    *bin;

	nih_assert (hash != NULL);
	nih_assert (entry != NULL);

	key = hash->key_function (entry);
	bin = &hash->bins[fnv_hash (key) % hash->size];

	return nih_list_add (bin, entry);
}

/**
 * nih_hash_add_unique:
 * @hash: destination hash table,
 * @entry: entry to be added.
 *
 * Adds @entry to @hash using the value returned by the hash NihKeyFunction
 * to indicate which bin the entry should be placed into, provided the key
 * is unique.
 *
 * Because the hash table does not store the key of each entry, this requires
 * that NihKeyFunction be called for each entry in the destination bin, so
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
	const char *key;
	NihList    *bin;

	nih_assert (hash != NULL);
	nih_assert (entry != NULL);

	key = hash->key_function (entry);
	bin = &hash->bins[fnv_hash (key) % hash->size];

	NIH_LIST_FOREACH (bin, iter) {
		if (! strcmp (key, hash->key_function (iter)))
			return NULL;
	}

	return nih_list_add (bin, entry);
}

/**
 * nih_hash_replace:
 * @hash: destination hash table,
 * @entry: entry to be added.
 *
 * Adds @entry to @hash using the value returned by the hash NihKeyFunction
 * to indicate which bin the entry should be placed into, replacing any
 * existing entry with the same key.
 *
 * Because the hash table does not store the key of each entry, this requires
 * that NihKeyFunction be called for each entry in the destination bin, so
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
	const char *key;
	NihList    *bin, *ret = NULL;

	nih_assert (hash != NULL);
	nih_assert (entry != NULL);

	key = hash->key_function (entry);
	bin = &hash->bins[fnv_hash (key) % hash->size];

	NIH_LIST_FOREACH (bin, iter) {
		if (! strcmp (key, hash->key_function (iter))) {
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
 * NihKeyFunction on each entry in the appropriate bin, starting with @entry,
 * until one is found.
 *
 * The initial @entry can be found by passing NULL or using nih_hash_lookup().
 *
 * Returns: next entry in the hash or NULL if there are no more entries.
 **/
NihList *
nih_hash_search (NihHash    *hash,
		 const char *key,
		 NihList    *entry)
{
	NihList *bin;

	nih_assert (hash != NULL);
	nih_assert (key != NULL);

	bin = &hash->bins[fnv_hash (key) % hash->size];
	NIH_LIST_FOREACH (bin, iter) {
		if (iter == entry) {
			entry = NULL;
			continue;
		} else if (entry) {
			continue;
		} else if (! strcmp (key, hash->key_function (iter))) {
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
		 const char *key)
{
	return nih_hash_search (hash, key, NULL);
}


/**
 * nih_hash_string_key:
 * @entry: entry to be checked.
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
