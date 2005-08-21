/* libnih
 *
 * test_list.c - test suite for nih/list.c
 *
 * Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include <nih/list.h>


static int
test_new (void)
{
	static char *data = "some data";
	NihList     *entry;
	int          ret = 0;

	printf ("Testing nih_list_new()\n");
	entry = nih_list_new (data);

	/* Data pointer should be set to what we gave */
	if (entry->data != data) {
		printf ("FAIL: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous pointer should point back to itself */
	if (entry->prev != entry) {
		printf ("FAIL: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Next pointer should point back to itself */
	if (entry->next != entry) {
		printf ("FAIL: next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

static int
test_add (void)
{
	static char *data = "new data";
	NihList     *list, *entry1, *entry2, *ptr;
	int          ret = 0;

	printf ("Testing nih_list_add()\n");

	list = nih_list_new ("list head");
	entry1 = nih_list_new ("entry 1");
	entry2 = nih_list_new ("entry 2");

	printf ("...with single-entry list\n");
	ptr = nih_list_add (list, entry1);

	/* The added entry should be returned */
	if (ptr != entry1) {
		printf ("FAIL: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != entry1) {
		printf ("FAIL: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry too */
	if (list->next != entry1) {
		printf ("FAIL: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry1->next != list) {
		printf ("FAIL: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head too */
	if (entry1->prev != list) {
		printf ("FAIL: entry prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with multi-entry list\n");
	nih_list_add (list, entry2);

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != entry2) {
		printf ("FAIL: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be unchanged. */
	if (list->next != entry1) {
		printf ("FAIL: head next pointer changed.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head */
	if (entry2->next != list) {
		printf ("FAIL: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the previous tail */
	if (entry2->prev != entry1) {
		printf ("FAIL: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous tail's next pointer should be the new entry */
	if (entry1->next != entry2) {
		printf ("FAIL: previous tail next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Previous tail's prev pointer should be unchanged. */
	if (entry1->prev != list) {
		printf ("FAIL: previous tail prev pointer changed.\n");
		ret = 1;
	}


	printf ("...with two entries from same list\n");
	nih_list_add (entry1, entry2);

	/* Head entry's next pointer should now be entry2 */
	if (list->next != entry2) {
		printf ("FAIL: list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's previous pointer should be the head entry */
	if (entry2->prev != list) {
		printf ("FAIL: entry2 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's next pointer should be entry1 */
	if (entry2->next != entry1) {
		printf ("FAIL: entry2 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's previous pointer should be entry2 */
	if (entry1->prev != entry2) {
		printf ("FAIL: entry1 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's next pointer should be the head entry */
	if (entry1->next != list) {
		printf ("FAIL: entry1 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* head entry's previous pointer should be entry1 */
	if (list->prev != entry1) {
		printf ("FAIL: head prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry from other list\n");
	ptr = nih_list_new ("another list");
	nih_list_add (ptr, entry2);

	/* The entry should be removed from the old list, so the
	 * old list head entry's next pointer should point to the tail. */
	if (list->next != entry1) {
		printf ("FAIL: old list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* The entry should be removed from the old list, so the
	 * old list tail entry's prev pointer should point to the head. */
	if (entry1->prev != list) {
		printf ("FAIL: old list tail prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("Testing nih_list_add_new()\n");
	entry2 = nih_list_add_new (list, data);

	/* Data pointer should be set to what we gave */
	if (entry2->data != data) {
		printf ("FAIL: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry */
	if (list->prev != entry2) {
		printf ("FAIL: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the list head */
	if (entry2->next != list) {
		printf ("FAIL: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the other entry */
	if (entry2->prev != entry1) {
		printf ("FAIL: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Other entry's next pointer should be the new entry */
	if (entry1->next != entry2) {
		printf ("FAIL: other next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

static int
test_add_after (void)
{
	static char *data = "new entry";
	NihList     *list, *entry1, *entry2, *ptr;
	int          ret = 0;

	printf ("Testing nih_list_add_after()\n");

	list = nih_list_new ("list head");
	entry1 = nih_list_new ("entry 1");
	entry2 = nih_list_new ("entry 2");

	printf ("...with single-entry list\n");
	ptr = nih_list_add_after (list, entry1);

	/* The added entry should be returned */
	if (ptr != entry1) {
		printf ("FAIL: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry */
	if (list->next != entry1) {
		printf ("FAIL: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be the new entry too */
	if (list->prev != entry1) {
		printf ("FAIL: head prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head */
	if (entry1->prev != list) {
		printf ("FAIL: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the head too */
	if (entry1->next != list) {
		printf ("FAIL: entry next pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with multi-entry list\n");
	nih_list_add_after (list, entry2);

	/* Head entry's next pointer should be the new entry */
	if (list->next != entry2) {
		printf ("FAIL: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should be unchanged. */
	if (list->prev != entry1) {
		printf ("FAIL: head prev pointer changed.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the head */
	if (entry2->prev != list) {
		printf ("FAIL: entry prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the tail */
	if (entry2->next != entry1) {
		printf ("FAIL: entry next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Tail entry's prev pointer should be the new entry */
	if (entry1->prev != entry2) {
		printf ("FAIL: tail prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* Tail entry's next pointer should be unchanged. */
	if (entry1->next != list) {
		printf ("FAIL: tail next pointer changed.\n");
		ret = 1;
	}


	printf ("...with two entries from same list\n");
	nih_list_add_after (entry1, entry2);

        /* Head entry's next pointer should now be entry1 */
	if (list->next != entry1) {
		printf ("FAIL: list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's previous pointer should be the head entry */
	if (entry1->prev != list) {
		printf ("FAIL: entry1 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry1's next pointer should be entry2 */
	if (entry1->next != entry2) {
		printf ("FAIL: entry1 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's previous pointer should be entry1 */
	if (entry2->prev != entry1) {
		printf ("FAIL: entry2 prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* entry2's next pointer should be the head entry */
	if (entry2->next != list) {
		printf ("FAIL: entry2 next pointer set incorrectly.\n");
		ret = 1;
	}

	/* head entry's previous pointer should be entry2 */
	if (list->prev != entry2) {
		printf ("FAIL: head prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("...with entry from other list\n");
	ptr = nih_list_new ("another list");
	nih_list_add_after (ptr, entry1);

	/* The entry should be removed from the old list, so the
	 * old list head entry's next pointer should point to the tail. */
	if (list->next != entry2) {
		printf ("FAIL: old list head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* The entry should be removed from the old list, so the
	 * old list tail entry's prev pointer should point to the head. */
	if (entry2->prev != list) {
		printf ("FAIL: old list tail prev pointer set incorrectly.\n");
		ret = 1;
	}


	printf ("Testing nih_list_add_new_after()\n");
	entry1 = nih_list_add_new_after (list, data);

	/* Data pointer should be set to what we gave */
	if (entry1->data != data) {
		printf ("FAIL: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should be the new entry */
	if (list->next != entry1) {
		printf ("FAIL: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's previous pointer should be the list head */
	if (entry1->prev != list) {
		printf ("FAIL: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* New entry's next pointer should be the other entry */
	if (entry1->next != entry2) {
		printf ("FAIL: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Other entry's previous pointer should be the new entry */
	if (entry2->prev != entry1) {
		printf ("FAIL: other prev pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

static int
test_remove (void)
{
	NihList *list, *entry, *tail, *ptr;
	int      ret = 0;

	list = nih_list_new ("a list");
	entry = nih_list_add_new (list, "entry 1");
	tail = nih_list_add_new (list, "entry 2");

	printf ("Testing nih_list_remove()\n");
	ptr = nih_list_remove (entry);

	/* The entry that was removed should be returned */
	if (ptr != entry) {
		printf ("FAIL: return value is not what we expected.\n");
		ret = 1;
	}

	/* The previous pointer should point back to itself */
	if (entry->prev != entry) {
		printf ("FAIL: prev pointer set incorrectly.\n");
		ret = 1;
	}

	/* The next pointer should point back to itself */
	if (entry->next != entry) {
		printf ("FAIL: next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's next pointer should point to the tail entry */
	if (list->next != tail) {
		printf ("FAIL: head next pointer set incorrectly.\n");
		ret = 1;
	}

	/* Head entry's previous pointer should still point to the tail */
	if (list->prev != tail) {
		printf ("FAIL: head prev pointer changed.\n");
		ret = 1;
	}

	/* Tail entry's next pointer should still point to the head */
	if (tail->next != list) {
		printf ("FAIL: tail next pointer changed.\n");
		ret = 1;
	}

	/* Tail entry's previous pointer should point to the head entry */
	if (tail->prev != list) {
		printf ("FAIL: tail next pointer set incorrectly.\n");
		ret = 1;
	}

	return ret;
}

static int
test_iterator (void)
{
	NihListIter  iter = NULL;
	NihList     *list, *entry1, *entry2, *ptr;
	int          ret = 0;

	printf ("Testing NihList iterators\n");
	list = nih_list_new ("a list");
	entry1 = nih_list_add_new (list, "entry 1");
	entry2 = nih_list_add_new (list, "entry 2");

	printf ("...forwards\n");

	/* Check that NIH_LIST_FIRST is true */
	if (!NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for first entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is false */
	if (NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for first entry.\n");
		ret = 1;
	}

	/* Advancing the list should give us the middle entry */
	ptr = NIH_LIST_NEXT (list, iter);
	if (ptr != entry1) {
		printf ("FAIL: NIH_LIST_NEXT didn't give second entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_FIRST is false */
	if (NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for second entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is false */
	if (NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for second entry.\n");
		ret = 1;
	}

	/* Advancing the list again should give us the third entry */
	ptr = NIH_LIST_NEXT (list, iter);
	if (ptr != entry2) {
		printf ("FAIL: NIH_LIST_NEXT didn't give third entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_FIRST is false */
	if (NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for third entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is false */
	if (NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for third entry.\n");
		ret = 1;
	}

	/* Advancing again should return the first entry again */
	ptr = NIH_LIST_NEXT (list, iter);
	if (ptr != list) {
		printf ("FAIL: NIH_LIST_NEXT didn't give first entry again.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_FIRST is false */
	if (NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for end of list.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is true */
	if (!NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for end of list.\n");
		ret = 1;
	}

	/* Check that the iterator was reset */
	if (iter != NULL) {
		printf ("FAIL: iterator wasn't reset to NULL.\n");
		ret = 1;
	}


	printf ("...backwards\n");

	/* Check that NIH_LIST_FIRST is true */
	if (!NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for first entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is false */
	if (NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for first entry.\n");
		ret = 1;
	}

	/* Advancing the list should give us the third entry */
	ptr = NIH_LIST_PREV (list, iter);
	if (ptr != entry2) {
		printf ("FAIL: NIH_LIST_PREV didn't give third entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_FIRST is false */
	if (NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for third entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is false */
	if (NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for third entry.\n");
		ret = 1;
	}

	/* Advancing the list again should give us the second entry */
	ptr = NIH_LIST_PREV (list, iter);
	if (ptr != entry1) {
		printf ("FAIL: NIH_LIST_PREV didn't give second entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_FIRST is false */
	if (NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for second entry.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is false */
	if (NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for second entry.\n");
		ret = 1;
	}

	/* Advancing again should return the first entry again */
	ptr = NIH_LIST_PREV (list, iter);
	if (ptr != list) {
		printf ("FAIL: NIH_LIST_PREV didn't give first entry again.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_FIRST is false */
	if (NIH_LIST_FIRST (list, iter)) {
		printf ("FAIL: NIH_LIST_FIRST incorrect for end of list.\n");
		ret = 1;
	}

	/* Check that NIH_LIST_LAST is true */
	if (!NIH_LIST_LAST (list, iter)) {
		printf ("FAIL: NIH_LIST_LAST incorrect for end of list.\n");
		ret = 1;
	}

	/* Check that the iterator was reset */
	if (iter != NULL) {
		printf ("FAIL: iterator wasn't reset to NULL.\n");
		ret = 1;
	}

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_new ();
	ret |= test_add ();
	ret |= test_add_after ();
	ret |= test_remove ();
	/* FIXME test_free once we've got nih_alloc destructors */
	ret |= test_iterator ();

	return ret;
}
