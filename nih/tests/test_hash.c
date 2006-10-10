/* libnih
 *
 * test_hash.c - test suite for nih/hash.c
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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


#include <stdio.h>
#include <assert.h>

#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/hash.h>


typedef struct hash_entry {
	NihList     list;
	const char *key;
} HashEntry;

static NihList *
new_entry (void       *parent,
	   const char *key)
{
	HashEntry *entry;

	assert (key != NULL);

	entry = nih_new (parent, HashEntry);
	assert (entry != NULL);

	nih_list_init (&entry->list);
	entry->key = key;

	return (NihList *)entry;
}

static const char *
key_function (NihList *entry)
{
	assert (entry != NULL);

	return ((HashEntry *)entry)->key;
}


int
test_new (void)
{
	NihHash *hash;
	size_t   i;
	int      ret = 0;

	printf ("Testing nih_hash_new()\n");

	printf ("...with zero size\n");
	hash = nih_hash_new (0, key_function);

	/* Size should be smallest prime number */
	if (hash->size != 17) {
		printf ("BAD: size set incorrectly.\n");
		ret = 1;
	}

	/* Bins should be non-NULL */
	if (hash->bins == NULL) {
		printf ("BAD: bins not allocated.\n");
		ret = 1;
	}

	/* Bins should be a child of the hash table */
	if (nih_alloc_parent (hash->bins) != hash) {
		printf ("BAD: bins not child allocation of hash.\n");
		ret = 1;
	}

	/* All bins should be empty */
	for (i = 0; i < hash->size; i++) {
		if (! NIH_LIST_EMPTY (&hash->bins[i])) {
			printf ("BAD: bin not initialised.\n");
			ret = 1;
		}
	}

	/* Key function should be what we gave */
	if (hash->key_function != key_function) {
		printf ("BAD: key_function set incorrectly.\n");
		ret = 1;
	}

	nih_free (hash);


	printf ("...with medium size\n");
	hash = nih_hash_new (650, key_function);

	/* Size should be closest prime number */
	if (hash->size != 331) {
		printf ("BAD: size set incorrectly.\n");
		ret = 1;
	}

	/* Bins should be non-NULL */
	if (hash->bins == NULL) {
		printf ("BAD: bins not allocated.\n");
		ret = 1;
	}

	/* Bins should be a child of the hash table */
	if (nih_alloc_parent (hash->bins) != hash) {
		printf ("BAD: bins not child allocation of hash.\n");
		ret = 1;
	}

	/* All bins should be empty */
	for (i = 0; i < hash->size; i++) {
		if (! NIH_LIST_EMPTY (&hash->bins[i])) {
			printf ("BAD: bin not initialised.\n");
			ret = 1;
		}
	}

	/* Key function should be what we gave */
	if (hash->key_function != key_function) {
		printf ("BAD: key_function set incorrectly.\n");
		ret = 1;
	}

	nih_free (hash);


	printf ("...with large size\n");
	hash = nih_hash_new (40000000, key_function);

	/* Size should be largest prime number */
	if (hash->size != 10250323) {
		printf ("BAD: size set incorrectly.\n");
		ret = 1;
	}

	/* Bins should be non-NULL */
	if (hash->bins == NULL) {
		printf ("BAD: bins not allocated.\n");
		ret = 1;
	}

	/* Bins should be a child of the hash table */
	if (nih_alloc_parent (hash->bins) != hash) {
		printf ("BAD: bins not child allocation of hash.\n");
		ret = 1;
	}

	/* All bins should be empty */
	for (i = 0; i < hash->size; i++) {
		if (! NIH_LIST_EMPTY (&hash->bins[i])) {
			printf ("BAD: bin not initialised.\n");
			ret = 1;
		}
	}

	/* Key function should be what we gave */
	if (hash->key_function != key_function) {
		printf ("BAD: key_function set incorrectly.\n");
		ret = 1;
	}

	nih_free (hash);

	return ret;
}


int
test_add (void)
{
	NihHash *hash;
	NihList *entry1, *entry2, *entry3, *entry4, *ptr;
	int      ret = 0;

	printf ("Testing nih_hash_add()\n");
	hash = nih_hash_new (0, key_function);
	entry1 = new_entry (hash, "entry 1");
	entry2 = new_entry (hash, "entry 2");
	entry3 = new_entry (hash, "entry 1");
	entry4 = new_entry (hash, "entry 4");

	printf ("...with empty hash\n");
	ptr = nih_hash_add (hash, entry1);

	/* The added entry should be returned */
	if (ptr != entry1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* 15th hash bin head's next pointer should be the entry */
	if (hash->bins[15].next != entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 15th hash bin head's previous pointer should be the entry */
	if (hash->bins[15].prev != entry1) {
		printf ("BAD: previous next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 15th hash bin */
	if (entry1->next != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 15th hash bin */
	if (entry1->prev != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with non-empty hash\n");
	nih_hash_add (hash, entry2);

	/* 14th hash bin head's next pointer should be the entry */
	if (hash->bins[14].next != entry2) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 14th hash bin head's previous pointer should be the entry */
	if (hash->bins[14].prev != entry2) {
		printf ("BAD: head previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 14th hash bin */
	if (entry2->next != &hash->bins[14]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 14th hash bin */
	if (entry2->prev != &hash->bins[14]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with duplicate key\n");
	nih_hash_add (hash, entry3);

	/* First entry's next pointer should be the new entry */
	if (entry1->next != entry3) {
		printf ("BAD: first entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* First entry's previous pointer should still be the head */
	if (entry1->prev != &hash->bins[15]) {
		printf ("BAD: first entry previous pointer changed.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the first entry */
	if (entry3->prev != entry1) {
		printf ("BAD: entry previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry3->next != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head's previous pointer should be the new entry */
	if (hash->bins[15].prev != entry3) {
		printf ("BAD: head previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head's next pointer should still be the first entry */
	if (hash->bins[15].next != entry1) {
		printf ("BAD: head next pointer changed.\n");
		ret = 1;
	}


	printf ("...with entry already in a list\n");
	ptr = nih_list_new (NULL);
	nih_list_add (ptr, entry4);
	nih_hash_add (hash, entry4);

	/* List's previous pointer should point back to itself */
	if (ptr->prev != ptr) {
		printf ("BAD: list previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* List's next pointer should point back to itself */
	if (ptr->next != ptr) {
		printf ("BAD: list next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 3rd hash bin head's next pointer should be the entry */
	if (hash->bins[3].next != entry4) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 3rd hash bin head's previous pointer should be the entry */
	if (hash->bins[3].prev != entry4) {
		printf ("BAD: previous next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 3rd hash bin */
	if (entry4->next != &hash->bins[3]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 3rd hash bin */
	if (entry4->prev != &hash->bins[3]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	nih_free (hash);
	nih_free (ptr);

	return ret;
}

int
test_add_unique (void)
{
	NihHash *hash;
	NihList *entry1, *entry2, *entry3, *entry4, *ptr;
	int      ret = 0;

	printf ("Testing nih_hash_add_unique()\n");
	hash = nih_hash_new (0, key_function);
	entry1 = new_entry (hash, "entry 1");
	entry2 = new_entry (hash, "entry 2");
	entry3 = new_entry (hash, "entry 1");
	entry4 = new_entry (hash, "entry 4");

	printf ("...with empty hash\n");
	ptr = nih_hash_add_unique (hash, entry1);

	/* The added entry should be returned */
	if (ptr != entry1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* 15th hash bin head's next pointer should be the entry */
	if (hash->bins[15].next != entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 15th hash bin head's previous pointer should be the entry */
	if (hash->bins[15].prev != entry1) {
		printf ("BAD: previous next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 15th hash bin */
	if (entry1->next != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 15th hash bin */
	if (entry1->prev != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with non-empty hash\n");
	nih_hash_add_unique (hash, entry2);

	/* 14th hash bin head's next pointer should be the entry */
	if (hash->bins[14].next != entry2) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 14th hash bin head's previous pointer should be the entry */
	if (hash->bins[14].prev != entry2) {
		printf ("BAD: head previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 14th hash bin */
	if (entry2->next != &hash->bins[14]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 14th hash bin */
	if (entry2->prev != &hash->bins[14]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with duplicate key\n");
	ptr = nih_hash_add_unique (hash, entry3);

	/* Should have returned NULL */
	if (ptr != NULL) {
		printf ("BAD: return value not correct.\n");
		ret = 1;
	}

	/* First entry's next pointer should still be the head */
	if (entry1->next != &hash->bins[15]) {
		printf ("BAD: first entry next pointer changed.\n");
		ret = 1;
	}

	/* First entry's previous pointer should still be the head */
	if (entry1->prev != &hash->bins[15]) {
		printf ("BAD: first entry previous pointer changed.\n");
		ret = 1;
	}

	/* Head's previous pointer should still be the first entry */
	if (hash->bins[15].prev != entry1) {
		printf ("BAD: head previous pointer changed.\n");
		ret = 1;
	}

	/* Head's next pointer should still be the first entry */
	if (hash->bins[15].next != entry1) {
		printf ("BAD: head next pointer changed.\n");
		ret = 1;
	}

	/* New entry's previous pointer should still be itself */
	if (entry3->prev != entry3) {
		printf ("BAD: entry previous pointer changed.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry3->next != entry3) {
		printf ("BAD: entry next pointer changed.\n");
		ret = 1;
	}


	printf ("...with entry already in a list\n");
	ptr = nih_list_new (NULL);
	nih_list_add (ptr, entry4);
	nih_hash_add_unique (hash, entry4);

	/* List's previous pointer should point back to itself */
	if (ptr->prev != ptr) {
		printf ("BAD: list previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* List's next pointer should point back to itself */
	if (ptr->next != ptr) {
		printf ("BAD: list next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 3rd hash bin head's next pointer should be the entry */
	if (hash->bins[3].next != entry4) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 3rd hash bin head's previous pointer should be the entry */
	if (hash->bins[3].prev != entry4) {
		printf ("BAD: previous next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 3rd hash bin */
	if (entry4->next != &hash->bins[3]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 3rd hash bin */
	if (entry4->prev != &hash->bins[3]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	nih_free (hash);
	nih_free (ptr);

	return ret;
}

int
test_replace (void)
{
	NihHash *hash;
	NihList *entry1, *entry2, *entry3, *entry4, *ptr;
	int      ret = 0;

	printf ("Testing nih_hash_replace()\n");
	hash = nih_hash_new (0, key_function);
	entry1 = new_entry (hash, "entry 1");
	entry2 = new_entry (hash, "entry 2");
	entry3 = new_entry (hash, "entry 1");
	entry4 = new_entry (hash, "entry 4");

	printf ("...with empty hash\n");
	ptr = nih_hash_replace (hash, entry1);

	/* NULL should be returned */
	if (ptr != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* 15th hash bin head's next pointer should be the entry */
	if (hash->bins[15].next != entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 15th hash bin head's previous pointer should be the entry */
	if (hash->bins[15].prev != entry1) {
		printf ("BAD: previous next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 15th hash bin */
	if (entry1->next != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 15th hash bin */
	if (entry1->prev != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with non-empty hash\n");
	nih_hash_replace (hash, entry2);

	/* 14th hash bin head's next pointer should be the entry */
	if (hash->bins[14].next != entry2) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 14th hash bin head's previous pointer should be the entry */
	if (hash->bins[14].prev != entry2) {
		printf ("BAD: head previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 14th hash bin */
	if (entry2->next != &hash->bins[14]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 14th hash bin */
	if (entry2->prev != &hash->bins[14]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with duplicate key\n");
	ptr = nih_hash_replace (hash, entry3);

	/* The replaced entry should be returned */
	if (ptr != entry1) {
		printf ("BAD: return value not correct.\n");
		ret = 1;
	}

	/* Replaced entry's next pointer should point back to itself */
	if (entry1->next != entry1) {
		printf ("BAD: replaced entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Replaced entry's previous pointer should point back to itself */
	if (entry1->prev != entry1) {
		printf ("BAD: replaced entry previous set incorrectly.\n");
		ret = 1;
	}

	/* Head's previous pointer should point to the new entry */
	if (hash->bins[15].prev != entry3) {
		printf ("BAD: head previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head's next pointer should point to the new entry */
	if (hash->bins[15].next != entry3) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should point to the head */
	if (entry3->prev != &hash->bins[15]) {
		printf ("BAD: entry previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should point to the head */
	if (entry3->next != &hash->bins[15]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry already in a list\n");
	ptr = nih_list_new (NULL);
	nih_list_add (ptr, entry4);
	nih_hash_replace (hash, entry4);

	/* List's previous pointer should point back to itself */
	if (ptr->prev != ptr) {
		printf ("BAD: list previous pointer set incorrectly.\n");
		ret = 1;
	}

	/* List's next pointer should point back to itself */
	if (ptr->next != ptr) {
		printf ("BAD: list next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 3rd hash bin head's next pointer should be the entry */
	if (hash->bins[3].next != entry4) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* 3rd hash bin head's previous pointer should be the entry */
	if (hash->bins[3].prev != entry4) {
		printf ("BAD: previous next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's next pointer should be the 3rd hash bin */
	if (entry4->next != &hash->bins[3]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Entry's previous pointer should be the 3rd hash bin */
	if (entry4->prev != &hash->bins[3]) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	nih_free (hash);
	nih_free (ptr);

	return ret;
}

int
test_search (void)
{
	NihHash *hash;
	NihList *entry1, *entry2, *entry3, *ptr;
	int      ret = 0;

	printf ("Testing nih_hash_search()\n");
	hash = nih_hash_new (0, key_function);
	entry1 = nih_hash_add (hash, new_entry (hash, "entry 1"));
	entry2 = nih_hash_add (hash, new_entry (hash, "entry 2"));
	entry3 = nih_hash_add (hash, new_entry (hash, "entry 2"));

	printf ("...with single match\n");
	ptr = nih_hash_search (hash, "entry 1", NULL);

	/* First return value should be the entry */
	if (ptr != entry1) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	ptr = nih_hash_search (hash, "entry 1", ptr);

	/* Second return value should be NULL */
	if (ptr != NULL) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}


	printf ("...with multiple matches\n");
	ptr = nih_hash_search (hash, "entry 2", NULL);

	/* Return value should be first added */
	if (ptr != entry2) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	ptr = nih_hash_search (hash, "entry 2", ptr);

	/* Second return value should be second added */
	if (ptr != entry3) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	ptr = nih_hash_search (hash, "entry 2", ptr);

	/* Third return value should be NULL */
	if (ptr != NULL) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}


	printf ("...with no matches\n");
	ptr = nih_hash_search (hash, "entry 3", NULL);

	/* Return value should be NULL */
	if (ptr != NULL) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	nih_free (hash);

	return ret;
}

int
test_lookup (void)
{
	NihHash *hash;
	NihList *entry1, *entry2, *entry3, *ptr;
	int      ret = 0;

	printf ("Testing nih_hash_lookup()\n");
	hash = nih_hash_new (0, key_function);
	entry1 = nih_hash_add (hash, new_entry (hash, "entry 1"));
	entry2 = nih_hash_add (hash, new_entry (hash, "entry 2"));
	entry3 = nih_hash_add (hash, new_entry (hash, "entry 2"));

	printf ("...with single match\n");
	ptr = nih_hash_lookup (hash, "entry 1");

	/* Return value should be the entry */
	if (ptr != entry1) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}


	printf ("...with multiple matches\n");
	ptr = nih_hash_lookup (hash, "entry 2");

	/* Return value should be first added */
	if (ptr != entry2) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}


	printf ("...with no matches\n");
	ptr = nih_hash_lookup (hash, "entry 3");

	/* Return value should be NULL */
	if (ptr != NULL) {
		printf ("BAD: return value was not correct.\n");
		ret = 1;
	}

	nih_free (hash);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_new ();
	ret |= test_add ();
	ret |= test_add_unique ();
	ret |= test_replace ();
	ret |= test_search ();
	ret |= test_lookup ();

	return ret;
}
