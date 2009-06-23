/* libnih
 *
 * test_list.c - test suite for nih/list.c
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

#include <nih/test.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>


void
test_init (void)
{
	NihList entry;

	/* Check that nih_list_init correctly initialises an empty list,
	 * with both pointers pointing back to the entry itself.
	 */
	TEST_FUNCTION ("nih_list_init");
	nih_list_init (&entry);

	TEST_EQ_P (entry.prev, &entry);
	TEST_EQ_P (entry.next, &entry);
}

void
test_new (void)
{
	NihList *list;

	/* Check that nih_list_new allocates a new empty list with nih_alloc
	 * and that it is initialised with pointers pointing to itself.  If
	 * allocation fails, we should get NULL returned.
	 */
	TEST_FUNCTION ("nih_list_new");
	TEST_ALLOC_FAIL {
		list = nih_list_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (list, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (list, sizeof (NihList));
		TEST_EQ_P (list->prev, list);
		TEST_EQ_P (list->next, list);

		nih_free (list);
	}
}

void
test_entry_new (void)
{
	NihListEntry *list;

	/* Check that nih_list_entry_new allocates a new empty list entry with
	 * nih_alloc and that it is initialised with pointers pointing to
	 * itself.  If allocation fails, we should get NULL returned.
	 */
	TEST_FUNCTION ("nih_list_entry_new");
	TEST_ALLOC_FAIL {
		list = nih_list_entry_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (list, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (list, sizeof (NihListEntry));
		TEST_EQ_P (list->entry.prev, &list->entry);
		TEST_EQ_P (list->entry.next, &list->entry);
		TEST_EQ_P (list->data, NULL);

		nih_free (list);
	}
}

void
test_add (void)
{
	NihList *list, *entry1, *entry2, *ptr;

	TEST_FUNCTION ("nih_list_add");

	list = nih_list_new (NULL);
	entry1 = nih_list_new (NULL);
	entry2 = nih_list_new (NULL);

	/* Check that nih_list_add can add a single entry to an empty list;
	 * the added entry should be returned and the pointers should all
	 * chain up.
	 */
	TEST_FEATURE ("with single-entry list");
	ptr = nih_list_add (list, entry1);

	TEST_EQ_P (ptr, entry1);
	TEST_EQ_P (list->next, entry1);
	TEST_EQ_P (entry1->next, list);
	TEST_EQ_P (list->prev, entry1);
	TEST_EQ_P (entry1->prev, list);


	/* Check that we can now add another entry to that two entry list,
	 * and the pointers are still all right.
	 */
	TEST_FEATURE ("with multi-entry list");
	nih_list_add (list, entry2);

	TEST_EQ_P (list->next, entry1);
	TEST_EQ_P (entry1->next, entry2);
	TEST_EQ_P (entry2->next, list);
	TEST_EQ_P (list->prev, entry2);
	TEST_EQ_P (entry2->prev, entry1);
	TEST_EQ_P (entry1->prev, list);


	/* Check that we can use nih_list_add to swap two entries that are
	 * in the same list.
	 */
	TEST_FEATURE ("with two entries from same list");
	nih_list_add (entry1, entry2);

	TEST_EQ_P (list->next, entry2);
	TEST_EQ_P (entry2->next, entry1);
	TEST_EQ_P (entry1->next, list);
	TEST_EQ_P (list->prev, entry1);
	TEST_EQ_P (entry1->prev, entry2);
	TEST_EQ_P (entry2->prev, list);


	/* Check that we can rip an entry out of its list and place it in
	 * a new empty one.
	 */
	TEST_FEATURE ("with entry from other list");
	ptr = nih_list_new (NULL);
	nih_list_add (ptr, entry2);

	TEST_EQ_P (list->next, entry1);
	TEST_EQ_P (entry1->next, list);
	TEST_EQ_P (list->prev, entry1);
	TEST_EQ_P (entry1->prev, list);

	TEST_EQ_P (ptr->next, entry2);
	TEST_EQ_P (entry2->next, ptr);
	TEST_EQ_P (ptr->prev, entry2);
	TEST_EQ_P (entry2->prev, ptr);

	nih_free (list);
	nih_free (entry1);
	nih_free (entry2);
	nih_free (ptr);
}

void
test_add_after (void)
{
	NihList *list, *entry1, *entry2, *ptr;

	TEST_FUNCTION ("nih_list_add_after");

	list = nih_list_new (NULL);
	entry1 = nih_list_new (NULL);
	entry2 = nih_list_new (NULL);

	/* Check that nih_list_add_after can add a single entry to an empty
	 * list, the result should be the same as nih_list_add.
	 */
	TEST_FEATURE ("with single-entry list");
	ptr = nih_list_add_after (list, entry1);

	TEST_EQ_P (ptr, entry1);
	TEST_EQ_P (list->next, entry1);
	TEST_EQ_P (entry1->next, list);
	TEST_EQ_P (list->prev, entry1);
	TEST_EQ_P (entry1->prev, list);


	/* Check that when adding an entry to a list with multiple entries,
	 * nih_list_add_after adds the entry immediately after the entry
	 * given, not before (as nih_list_add does)
	 */
	TEST_FEATURE ("with multi-entry list");
	nih_list_add_after (list, entry2);

	TEST_EQ_P (list->next, entry2);
	TEST_EQ_P (entry2->next, entry1);
	TEST_EQ_P (entry1->next, list);
	TEST_EQ_P (list->prev, entry1);
	TEST_EQ_P (entry1->prev, entry2);
	TEST_EQ_P (entry2->prev, list);


	/* Check that nih_list_add_after can be used to swap two entries
	 * around.
	 */
	TEST_FEATURE ("with two entries from same list");
	nih_list_add_after (entry1, entry2);

	TEST_EQ_P (list->next, entry1);
	TEST_EQ_P (entry1->next, entry2);
	TEST_EQ_P (entry2->next, list);
	TEST_EQ_P (list->prev, entry2);
	TEST_EQ_P (entry2->prev, entry1);
	TEST_EQ_P (entry1->prev, list);


	/* Check that nih_list_add_after can rip an entry out of its
	 * containing list, and add it to a new one.
	 */
	TEST_FEATURE ("with entry from other list");
	ptr = nih_list_new (NULL);
	nih_list_add_after (ptr, entry1);

	TEST_EQ_P (list->next, entry2);
	TEST_EQ_P (entry2->next, list);
	TEST_EQ_P (list->prev, entry2);
	TEST_EQ_P (entry2->prev, list);

	TEST_EQ_P (ptr->next, entry1);
	TEST_EQ_P (entry1->next, ptr);
	TEST_EQ_P (ptr->prev, entry1);
	TEST_EQ_P (entry1->prev, ptr);

	nih_free (list);
	nih_free (entry1);
	nih_free (entry2);
	nih_free (ptr);
}

void
test_empty (void)
{
	NihList *list, *entry;

	TEST_FUNCTION ("NIH_LIST_EMPTY");

	/* Check that NIH_LIST_EMPTY is TRUE on an empty list */
	TEST_FEATURE ("with empty list");
	list = nih_list_new (NULL);

	TEST_LIST_EMPTY (list);


	/* Check that NIH_LIST_EMPTY is FALSE on a non-empty list */
	TEST_FEATURE ("with non-empty list");
	entry = nih_list_new (NULL);
	nih_list_add (list, entry);

	TEST_LIST_NOT_EMPTY (list);
	TEST_LIST_NOT_EMPTY (entry);

	nih_free (list);
	nih_free (entry);
}

void
test_foreach (void)
{
	NihList *list, *entry[3];
	int      i;

	/* Check that NIH_LIST_FOREACH iterates the list correctly in
	 * order, visiting each entry.
	 */
	TEST_FUNCTION ("NIH_LIST_FOREACH");
	list = nih_list_new (NULL);
	entry[0] = nih_list_add (list, nih_list_new (NULL));
	entry[1] = nih_list_add (list, nih_list_new (NULL));
	entry[2] = nih_list_add (list, nih_list_new (NULL));

	i = 0;
	NIH_LIST_FOREACH (list, iter) {
		if (i > 2)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != entry[i])
			TEST_FAILED ("wrong list entry, expected %p got %p",
				     entry[i], iter);

		i++;
	}

	nih_free (list);
	nih_free (entry[0]);
	nih_free (entry[1]);
	nih_free (entry[2]);
}

void
test_foreach_safe (void)
{
	NihList *list, *entry[3];
	int      i;

	TEST_FUNCTION ("NIH_LIST_FOREACH_SAFE");

	/* Check that NIH_LIST_FOREACH_SAFE iterates the list correctly in
	 * order, visiting each entry.
	 */
	TEST_FEATURE ("with ordinary iteration");
	list = nih_list_new (NULL);
	entry[0] = nih_list_add (list, nih_list_new (NULL));
	entry[1] = nih_list_add (list, nih_list_new (NULL));
	entry[2] = nih_list_add (list, nih_list_new (NULL));

	i = 0;
	NIH_LIST_FOREACH_SAFE (list, iter) {
		if (i > 2)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != entry[i])
			TEST_FAILED ("wrong list entry, expected %p got %p",
				     entry[i], iter);

		i++;
	}

	nih_free (list);
	nih_free (entry[0]);
	nih_free (entry[1]);
	nih_free (entry[2]);


	/* Check that NIH_LIST_FOREACH_SAFE iterates the list correctly in
	 * order, visiting each entry; and that it's safe to remove entries
	 * while doing so.
	 */
	TEST_FEATURE ("with removal of visited node");
	list = nih_list_new (NULL);
	entry[0] = nih_list_add (list, nih_list_new (NULL));
	entry[1] = nih_list_add (list, nih_list_new (NULL));
	entry[2] = nih_list_add (list, nih_list_new (NULL));

	i = 0;
	NIH_LIST_FOREACH_SAFE (list, iter) {
		if (i > 2)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != entry[i])
			TEST_FAILED ("wrong list entry, expected %p got %p",
				     entry[i], iter);

		nih_list_remove (entry[i]);

		i++;
	}


	/* Check that the list is now empty */
	TEST_LIST_EMPTY (list);

	nih_free (list);
	nih_free (entry[0]);
	nih_free (entry[1]);
	nih_free (entry[2]);


	/* Check that NIH_LIST_FOREACH_SAFE iterates the list correctly in
	 * order, visiting each entry; and that it's safe to remove the
	 * next entry while doing so.
	 */
	TEST_FEATURE ("with removal of next node");
	list = nih_list_new (NULL);
	entry[0] = nih_list_add (list, nih_list_new (NULL));
	entry[1] = nih_list_add (list, nih_list_new (NULL));
	entry[2] = nih_list_add (list, nih_list_new (NULL));

	i = 0;
	NIH_LIST_FOREACH_SAFE (list, iter) {
		if (i > 2)
			TEST_FAILED ("wrong number of iterations, expected %d got %d",
				     3, i + 1);

		if (iter != entry[i])
			TEST_FAILED ("wrong list entry, expected %p got %p",
				     entry[i], iter);

		if (i == 0)
			nih_list_remove (entry[1]);

		/* Next entry visited should be 2 */
		i += 2;
	}


	nih_free (list);
	nih_free (entry[0]);
	nih_free (entry[1]);
	nih_free (entry[2]);
}

void
test_remove (void)
{
	NihList *list, *entry, *tail, *ptr;

	TEST_FUNCTION ("nih_list_remove");
	list = nih_list_new (NULL);
	entry = nih_list_add (list, nih_list_new (NULL));
	tail = nih_list_add (list, nih_list_new (NULL));

	/* Check that nih_list_remove works, returning the entry that was
	 * removed and adjusting both sets of pointers in the lists.
	 */
	TEST_FEATURE ("with two-entry list");
	ptr = nih_list_remove (entry);

	TEST_EQ_P (ptr, entry);
	TEST_EQ_P (list->next, tail);
	TEST_EQ_P (tail->next, list);
	TEST_EQ_P (list->prev, tail);
	TEST_EQ_P (tail->prev, list);

	TEST_EQ_P (entry->next, entry);
	TEST_EQ_P (entry->prev, entry);


	/* Check that nih_list_remove works if there is only one entry in the
	 * list that's not the head, the pointers should both curl in.
	 */
	TEST_FEATURE ("with one-entry list");
	ptr = nih_list_remove (tail);

	TEST_EQ_P (list->next, list);
	TEST_EQ_P (list->prev, list);

	TEST_EQ_P (tail->next, tail);
	TEST_EQ_P (tail->prev, tail);


	/* Check that it works on an empty list, this should do nothing. */
	TEST_FEATURE ("with empty list");
	ptr = nih_list_remove (tail);

	TEST_EQ_P (tail->next, tail);
	TEST_EQ_P (tail->prev, tail);

	nih_free (list);
	nih_free (entry);
	nih_free (tail);
}

void
test_destroy (void)
{
	NihList *list, *entry, *tail;
	int      ret;

	/* Check that the function removes the entry from its containing
	 * list, it needn't bother updating the entry itself seeing as it's
	 * being freed anyway.
	 */
	TEST_FUNCTION ("nih_list_destroy");
	list = nih_list_new (NULL);
	entry = nih_list_add (list, nih_list_new (NULL));
	tail = nih_list_add (list, nih_list_new (NULL));
	ret = nih_list_destroy (entry);

	TEST_EQ (ret, 0);

	TEST_EQ_P (list->next, tail);
	TEST_EQ_P (tail->next, list);
	TEST_EQ_P (list->prev, tail);
	TEST_EQ_P (tail->prev, list);

	nih_free (entry);
	nih_free (list);
	nih_free (tail);
}


int
main (int   argc,
      char *argv[])
{
	test_init ();
	test_new ();
	test_entry_new ();
	test_add ();
	test_add_after ();
	test_empty ();
	test_foreach ();
	test_foreach_safe ();
	test_remove ();
	test_destroy ();

	return 0;
}
