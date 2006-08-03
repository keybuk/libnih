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
	if (nih_alloc_size (list) != sizeof (NihList)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	return ret;
}

int
test_add (void)
{
	NihList *list, *entry1, *entry2, *ptr;
	int      ret = 0;

	printf ("Testing nih_list_add()\n");

	list = nih_list_new ();
	entry1 = nih_list_new ();
	entry2 = nih_list_new ();

	printf ("...with single-entry list\n");
	ptr = nih_list_add (list, entry1);

	/* The added entry should be returned */
	if (ptr != entry1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != entry1) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry too */
	if (list->next != entry1) {
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
	nih_list_add (list, entry2);

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != entry2) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be unchanged. */
	if (list->next != entry1) {
		printf ("BAD: head next pointer changed.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry2->next != list) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the previous tail */
	if (entry2->prev != entry1) {
		printf ("BAD: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous tail's next pointer should be the new entry */
	if (entry1->next != entry2) {
		printf ("BAD: previous tail next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous tail's prev pointer should be unchanged. */
	if (entry1->prev != list) {
		printf ("BAD: previous tail prev pointer changed.\n");
		ret = 1;
	}


	printf ("...with two entries from same list\n");
	nih_list_add (entry1, entry2);

	/* Head entry's next pointer should now be entry2 */
	if (list->next != entry2) {
		printf ("BAD: list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's previous pointer should be the head entry */
	if (entry2->prev != list) {
		printf ("BAD: entry2 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's next pointer should be entry1 */
	if (entry2->next != entry1) {
		printf ("BAD: entry2 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's previous pointer should be entry2 */
	if (entry1->prev != entry2) {
		printf ("BAD: entry1 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's next pointer should be the head entry */
	if (entry1->next != list) {
		printf ("BAD: entry1 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* head entry's previous pointer should be entry1 */
	if (list->prev != entry1) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry from other list\n");
	ptr = nih_list_new ();
	nih_list_add (ptr, entry2);

	/* The entry should be removed from the old list, so the
	 * old list head entry's next pointer should point to the tail. */
	if (list->next != entry1) {
		printf ("BAD: old list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* The entry should be removed from the old list, so the
	 * old list tail entry's prev pointer should point to the head. */
	if (entry1->prev != list) {
		printf ("BAD: old list tail prev pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

int
test_add_after (void)
{
	NihList *list, *entry1, *entry2, *ptr;
	int      ret = 0;

	printf ("Testing nih_list_add_after()\n");

	list = nih_list_new ();
	entry1 = nih_list_new ();
	entry2 = nih_list_new ();

	printf ("...with single-entry list\n");
	ptr = nih_list_add_after (list, entry1);

	/* The added entry should be returned */
	if (ptr != entry1) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry */
	if (list->next != entry1) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry too */
	if (list->prev != entry1) {
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
	nih_list_add_after (list, entry2);

	/* Head entry's next pointer should be the new entry */
	if (list->next != entry2) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be unchanged. */
	if (list->prev != entry1) {
		printf ("BAD: head prev pointer changed.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head */
	if (entry2->prev != list) {
		printf ("BAD: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the tail */
	if (entry2->next != entry1) {
		printf ("BAD: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Tail entry's prev pointer should be the new entry */
	if (entry1->prev != entry2) {
		printf ("BAD: tail prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Tail entry's next pointer should be unchanged. */
	if (entry1->next != list) {
		printf ("BAD: tail next pointer changed.\n");
		ret = 1;
	}


	printf ("...with two entries from same list\n");
	nih_list_add_after (entry1, entry2);

        /* Head entry's next pointer should now be entry1 */
	if (list->next != entry1) {
		printf ("BAD: list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's previous pointer should be the head entry */
	if (entry1->prev != list) {
		printf ("BAD: entry1 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's next pointer should be entry2 */
	if (entry1->next != entry2) {
		printf ("BAD: entry1 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's previous pointer should be entry1 */
	if (entry2->prev != entry1) {
		printf ("BAD: entry2 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's next pointer should be the head entry */
	if (entry2->next != list) {
		printf ("BAD: entry2 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* head entry's previous pointer should be entry2 */
	if (list->prev != entry2) {
		printf ("BAD: head prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry from other list\n");
	ptr = nih_list_new ();
	nih_list_add_after (ptr, entry1);

	/* The entry should be removed from the old list, so the
	 * old list head entry's next pointer should point to the tail. */
	if (list->next != entry2) {
		printf ("BAD: old list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* The entry should be removed from the old list, so the
	 * old list tail entry's prev pointer should point to the head. */
	if (entry2->prev != list) {
		printf ("BAD: old list tail prev pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

int
test_empty (void)
{
	NihList *list, *entry;
	int      ret = 0;

	printf ("Testing NIH_LIST_EMPTY()\n");
	list = nih_list_new ();
	entry = nih_list_new ();

	printf ("...with empty list\n");

	/* The list should be empty */
	if (! NIH_LIST_EMPTY (list)) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with non-empty list\n");
	nih_list_add (list, entry);

	/* The list should not be empty */
	if (NIH_LIST_EMPTY (list)) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Check that's true for the entry too */
	if (NIH_LIST_EMPTY (entry)) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	return ret;
}

int
test_foreach (void)
{
	NihList *list, *entry[3];
	int      i, ret = 0;

	printf ("Testing NIH_LIST_FOREACH()\n");
	list = nih_list_new ();
	entry[0] = nih_list_add (list, nih_list_new ());
	entry[1] = nih_list_add (list, nih_list_new ());
	entry[2] = nih_list_add (list, nih_list_new ());

	/* Perform a test iteration */
	i = 0;
	NIH_LIST_FOREACH (list, iter) {
		if (i > 2) {
			printf ("BAD: more iterations than expected.\n");
			ret = 1;
			break;
		}

		if (iter != entry[i]) {
			printf ("BAD: iteration not entry we expected.\n");
			ret = 1;
		}

		i++;
	}

	return ret;
}

int
test_foreach_safe (void)
{
	NihList *list, *entry[3];
	int      i, ret = 0;

	printf ("Testing NIH_LIST_FOREACH_SAFE()\n");
	list = nih_list_new ();
	entry[0] = nih_list_add (list, nih_list_new ());
	entry[1] = nih_list_add (list, nih_list_new ());
	entry[2] = nih_list_add (list, nih_list_new ());

	/* Perform a test iteration */
	i = 0;
	NIH_LIST_FOREACH_SAFE (list, iter) {
		if (i > 2) {
			printf ("BAD: more iterations than expected.\n");
			ret = 1;
			break;
		}

		if (iter != entry[i]) {
			printf ("BAD: iteration not entry we expected.\n");
			ret = 1;
		}

		nih_list_remove (entry[i]);

		i++;
	}

	/* List should be empty */
	if (! NIH_LIST_EMPTY (list)) {
		printf ("BAD: list not empty.\n");
		ret = 1;
	}

	return ret;
}

int
test_remove (void)
{
	NihList *list, *entry, *tail, *ptr;
	int      ret = 0;

	printf ("Testing nih_list_remove()\n");
	list = nih_list_new ();
	entry = nih_list_add (list, nih_list_new ());
	tail = nih_list_add (list, nih_list_new ());

	printf ("...with two-entry list\n");
	ptr = nih_list_remove (entry);

	/* The entry that was removed should be returned */
	if (ptr != entry) {
		printf ("BAD: return value is not what we expected.\n");
		ret = 1;
	}

	/* The previous pointer should point back to itself */
	if (entry->prev != entry) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* The next pointer should point back to itself */
	if (entry->next != entry) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point to the tail entry */
	if (list->next != tail) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should still point to the tail */
	if (list->prev != tail) {
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


	printf ("...with one-entry list\n");
	ptr = nih_list_remove (tail);

	/* The entry that was removed should be returned */
	if (ptr != tail) {
		printf ("BAD: return value is not what we expected.\n");
		ret = 1;
	}

	/* The previous pointer should point back to itself */
	if (tail->prev != tail) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* The next pointer should point back to itself */
	if (tail->next != tail) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point back to itself */
	if (list->next != list) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should point back to itself */
	if (list->prev != list) {
		printf ("BAD: head prev pointer changed.\n");
		ret = 1;
	}


	printf ("...with empty list\n");
	ptr = nih_list_remove (tail);

	/* The entry that was removed should be returned */
	if (ptr != tail) {
		printf ("BAD: return value is not what we expected.\n");
		ret = 1;
	}

	/* The previous pointer should still point back to itself */
	if (tail->prev != tail) {
		printf ("BAD: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* The next pointer should still point back to itself */
	if (tail->next != tail) {
		printf ("BAD: next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

int
test_destructor (void)
{
	NihList *list, *entry, *tail;
	int      ret = 0, retval;

	printf ("Testing nih_list_destructor()\n");
	list = nih_list_new ();
	entry = nih_list_add (list, nih_list_new ());
	tail = nih_list_add (list, nih_list_new ());
	retval = nih_list_destructor (entry);

	/* Zero should be returned */
	if (retval != 0) {
		printf ("BAD: return value is not what we expected.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point to the tail entry */
	if (list->next != tail) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should still point to the tail */
	if (list->prev != tail) {
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
	NihList *list, *entry, *tail;
	int      ret = 0;

	list = nih_list_new ();
	entry = nih_list_add (list, nih_list_new ());
	tail = nih_list_add (list, nih_list_new ());

	printf ("Testing nih_list_free()\n");
	nih_alloc_set_destructor (entry, destructor_called);
	nih_list_free (entry);

	/* Destructor should have been called */
	if (! was_called) {
		printf ("BAD: destructor was not called.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point to the tail entry */
	if (list->next != tail) {
		printf ("BAD: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should still point to the tail */
	if (list->prev != tail) {
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
	ret |= test_add ();
	ret |= test_add_after ();
	ret |= test_empty ();
	ret |= test_foreach ();
	ret |= test_foreach_safe ();
	ret |= test_remove ();
	ret |= test_destructor ();
	ret |= test_free ();

	return ret;
}
