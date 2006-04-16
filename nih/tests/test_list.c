/* libnih
 *
 * test_list.c - test suite for nih/list.c
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
#include <string.h>

#include <nih/alloc.h>
#include <nih/list.h>


int
test_init (void)
{
	NihList entry;
	int     ret = 0;

	printf ("Testing nih_list_init()\n");
	nih_list_init (&entry);

	/* Previous pointer should point back to itself */
	if (entry.prev != &entry) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Next pointer should point back to itself */
	if (entry.next != &entry) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

int
test_new (void)
{
	NihList *list;
	int      ret = 0;

	printf ("Testing nih_list_new()\n");
	list = nih_list_new ();

	/* Previous pointer should point back to itself */
	if (list->prev != list) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Next pointer should point back to itself */
	if (list->next != list) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (! strstr (nih_alloc_name (list), "list.c")) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	return ret;
}

int
test_entry_new (void)
{
	static char  *data = "some data";
	NihListEntry *entry;
	int           ret = 0;

	printf ("Testing nih_list_entry_new()\n");
	entry = nih_list_entry_new (data);

	/* Data pointer should be set to what we gave */
	if (entry->data != data) {
		printf ("BAD: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous pointer should point back to itself */
	if (entry->prev != (NihList *)entry) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Next pointer should point back to itself */
	if (entry->next != (NihList *)entry) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (! strstr (nih_alloc_name (entry), "list.c")) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	return ret;
}

int
test_add (void)
{
	static char  *data = "new data";
	NihList      *list, *ptr;
	NihListEntry *entry1, *entry2;
	int           ret = 0;

	printf ("Testing nih_list_add()\n");

	list = nih_list_new ();
	entry1 = nih_list_entry_new ("entry 1");
	entry2 = nih_list_entry_new ("entry 2");

	printf ("...with single-entry list\n");
	ptr = nih_list_add (list, (NihList *)entry1);

	/* The added entry should be returned */
	if (ptr != (NihList *)entry1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != (NihList *)entry1) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry too */
	if (list->next != (NihList *)entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry1->next != list) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head too */
	if (entry1->prev != list) {
		printf ("BAD: entry prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with multi-entry list\n");
	nih_list_add (list, (NihList *)entry2);

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != (NihList *)entry2) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be unchanged. */
	if (list->next != (NihList *)entry1) {
		printf ("BAD: head next pointer changed.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry2->next != list) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the previous tail */
	if (entry2->prev != (NihList *)entry1) {
		printf ("BAD: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous tail's next pointer should be the new entry */
	if (entry1->next != (NihList *)entry2) {
		printf ("BAD: previous tail next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous tail's prev pointer should be unchanged. */
	if (entry1->prev != list) {
		printf ("BAD: previous tail prev pointer changed.\n");
		ret = 1;
	}


	printf ("...with two entries from same list\n");
	nih_list_add ((NihList *)entry1, (NihList *)entry2);

	/* Head entry's next pointer should now be entry2 */
	if (list->next != (NihList *)entry2) {
		printf ("BAD: list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's previous pointer should be the head entry */
	if (entry2->prev != list) {
		printf ("BAD: entry2 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's next pointer should be entry1 */
	if (entry2->next != (NihList *)entry1) {
		printf ("BAD: entry2 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's previous pointer should be entry2 */
	if (entry1->prev != (NihList *)entry2) {
		printf ("BAD: entry1 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's next pointer should be the head entry */
	if (entry1->next != list) {
		printf ("BAD: entry1 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* head entry's previous pointer should be entry1 */
	if (list->prev != (NihList *)entry1) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry from other list\n");
	ptr = nih_list_new ();
	nih_list_add (ptr, (NihList *)entry2);

	/* The entry should be removed from the old list, so the
	 * old list head entry's next pointer should point to the tail. */
	if (list->next != (NihList *)entry1) {
		printf ("BAD: old list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* The entry should be removed from the old list, so the
	 * old list tail entry's prev pointer should point to the head. */
	if (entry1->prev != list) {
		printf ("BAD: old list tail prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("Testing nih_list_add_new()\n");
	entry2 = nih_list_add_new (list, data);

	/* Data pointer should be set to what we gave */
	if (entry2->data != data) {
		printf ("BAD: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != (NihList *)entry2) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the list head */
	if (entry2->next != list) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the other entry */
	if (entry2->prev != (NihList *)entry1) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Other entry's next pointer should be the new entry */
	if (entry1->next != (NihList *)entry2) {
		printf ("BAD: other next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

int
test_add_after (void)
{
	static char  *data = "new entry";
	NihList      *list, *ptr;
	NihListEntry *entry1, *entry2;
	int           ret = 0;

	printf ("Testing nih_list_add_after()\n");

	list = nih_list_new ();
	entry1 = nih_list_entry_new ("entry 1");
	entry2 = nih_list_entry_new ("entry 2");

	printf ("...with single-entry list\n");
	ptr = nih_list_add_after (list, (NihList *)entry1);

	/* The added entry should be returned */
	if (ptr != (NihList *)entry1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry */
	if (list->next != (NihList *)entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry too */
	if (list->prev != (NihList *)entry1) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head */
	if (entry1->prev != list) {
		printf ("BAD: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head too */
	if (entry1->next != list) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with multi-entry list\n");
	nih_list_add_after (list, (NihList *)entry2);

	/* Head entry's next pointer should be the new entry */
	if (list->next != (NihList *)entry2) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be unchanged. */
	if (list->prev != (NihList *)entry1) {
		printf ("BAD: head prev pointer changed.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head */
	if (entry2->prev != list) {
		printf ("BAD: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the tail */
	if (entry2->next != (NihList *)entry1) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Tail entry's prev pointer should be the new entry */
	if (entry1->prev != (NihList *)entry2) {
		printf ("BAD: tail prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Tail entry's next pointer should be unchanged. */
	if (entry1->next != list) {
		printf ("BAD: tail next pointer changed.\n");
		ret = 1;
	}


	printf ("...with two entries from same list\n");
	nih_list_add_after ((NihList *)entry1, (NihList *)entry2);

        /* Head entry's next pointer should now be entry1 */
	if (list->next != (NihList *)entry1) {
		printf ("BAD: list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's previous pointer should be the head entry */
	if (entry1->prev != list) {
		printf ("BAD: entry1 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's next pointer should be entry2 */
	if (entry1->next != (NihList *)entry2) {
		printf ("BAD: entry1 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's previous pointer should be entry1 */
	if (entry2->prev != (NihList *)entry1) {
		printf ("BAD: entry2 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's next pointer should be the head entry */
	if (entry2->next != list) {
		printf ("BAD: entry2 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* head entry's previous pointer should be entry2 */
	if (list->prev != (NihList *)entry2) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry from other list\n");
	ptr = nih_list_new ();
	nih_list_add_after (ptr, (NihList *)entry1);

	/* The entry should be removed from the old list, so the
	 * old list head entry's next pointer should point to the tail. */
	if (list->next != (NihList *)entry2) {
		printf ("BAD: old list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* The entry should be removed from the old list, so the
	 * old list tail entry's prev pointer should point to the head. */
	if (entry2->prev != list) {
		printf ("BAD: old list tail prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("Testing nih_list_add_new_after()\n");
	entry1 = nih_list_add_new_after (list, data);

	/* Data pointer should be set to what we gave */
	if (entry1->data != data) {
		printf ("BAD: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry */
	if (list->next != (NihList *)entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the list head */
	if (entry1->prev != list) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the other entry */
	if (entry1->next != (NihList *)entry2) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Other entry's previous pointer should be the new entry */
	if (entry2->prev != (NihList *)entry1) {
		printf ("BAD: other prev pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

int
test_remove (void)
{
	NihList      *list, *ptr;
	NihListEntry *entry, *tail;
	int           ret = 0;

	list = nih_list_new ();
	entry = nih_list_add_new (list, "entry 1");
	tail = nih_list_add_new (list, "entry 2");

	printf ("Testing nih_list_remove()\n");
	ptr = nih_list_remove ((NihList *)entry);

	/* The entry that was removed should be returned */
	if (ptr != (NihList *)entry) {
		printf ("BAD: return value is not what we expected.\n");
		ret = 1;
	}

	/* The previous pointer should point back to itself */
	if (entry->prev != (NihList *)entry) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* The next pointer should point back to itself */
	if (entry->next != (NihList *)entry) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point to the tail entry */
	if (list->next != (NihList *)tail) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should still point to the tail */
	if (list->prev != (NihList *)tail) {
		printf ("BAD: head prev pointer changed.\n");
		ret = 1;
	}

	/* Tail entry's next pointer should still point to the head */
	if (tail->next != list) {
		printf ("BAD: tail next pointer changed.\n");
		ret = 1;
	}

	/* Tail entry's previous pointer should point to the head entry */
	if (tail->prev != list) {
		printf ("BAD: tail next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

static int was_called;

static int
destructor_called (void *ptr)
{
	was_called++;

	return 0;
}

static int
test_free (void)
{
	NihList      *list;
	NihListEntry *entry, *tail;
	int           ret = 0;

	list = nih_list_new ();
	entry = nih_list_add_new (list, "entry 1");
	tail = nih_list_add_new (list, "entry 2");

	printf ("Testing nih_list_free()\n");
	nih_alloc_set_destructor (entry, destructor_called);
	nih_list_free ((NihList *)entry);

	/* Destructor should have been called */
	if (! was_called) {
		printf ("BAD: destructor was not called.\n");
		ret = 1;
	}

	/* The previous pointer should point back to itself */
	if (entry->prev != (NihList *)entry) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* The next pointer should point back to itself */
	if (entry->next != (NihList *)entry) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point to the tail entry */
	if (list->next != (NihList *)tail) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should still point to the tail */
	if (list->prev != (NihList *)tail) {
		printf ("BAD: head prev pointer changed.\n");
		ret = 1;
	}

	/* Tail entry's next pointer should still point to the head */
	if (tail->next != list) {
		printf ("BAD: tail next pointer changed.\n");
		ret = 1;
	}

	/* Tail entry's previous pointer should point to the head entry */
	if (tail->prev != list) {
		printf ("BAD: tail next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_init ();
	ret |= test_new ();
	ret |= test_entry_new ();
	ret |= test_add ();
	ret |= test_add_after ();
	ret |= test_remove ();
	ret |= test_free ();

	return ret;
}
