/* libnih
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

#ifndef NIH_HASH_H
#define NIH_HASH_H

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihKeyFunction:
 * @entry: entry to key.
 *
 * This function is used to obtain a string key for a given hash table
 * entry; the key is not freed so should be a constant, or at least static,
 * string.
 *
 * Keys are always compared case-sensitively, code should take care to
 * present keys in a canon form.
 **/
typedef const char *(*NihKeyFunction) (NihList *entry);

/**
 * NihHash:
 * @bins: array of bins,
 * @size: size of bins array,
 * @key_function: function used to obtain keys for entries.
 *
 * This structure represents a hash table which is more efficient for
 * looking up members than an ordinary list.
 *
 * Individual members of the hash table are NihList members as are the
 * bins themselves, so to remove an entry from the table you can just
 * use nih_list_remove().
 **/
typedef struct nih_hash {
	NihList        *bins;
	size_t          size;

	NihKeyFunction  key_function;
} NihHash;


NIH_BEGIN_EXTERN

NihHash *nih_hash_new        (const void *parent, size_t entries,
			      NihKeyFunction key_function)
	__attribute__ ((warn_unused_result, malloc));

NihList *nih_hash_add        (NihHash *hash, NihList *entry);
NihList *nih_hash_add_unique (NihHash *hash, NihList *entry);
NihList *nih_hash_replace    (NihHash *hash, NihList *entry);

NihList *nih_hash_search     (NihHash *hash, const char *key, NihList *entry);
NihList *nih_hash_lookup     (NihHash *hash, const char *key);

NIH_END_EXTERN

#endif /* NIH_HASH_H */
