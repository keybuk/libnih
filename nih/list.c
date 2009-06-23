/* libnih
 *
 * list.c - generic circular doubly-linked list implementation
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


#include <nih/macros.h>
#include <nih/logging.h>
#include <nih/alloc.h>

#include "list.h"


/* Prototypes for static functions */
static inline NihList *nih_list_cut (NihList *entry);


/**
 * nih_list_init:
 * @entry: entry to be initialised.
 *
 * Initialise an already allocated list entry, once done it can be used
 * as the start of a new list or added to an existing list.
 **/
void
nih_list_init (NihList *entry)
{
	nih_assert (entry != NULL);

	entry->prev = entry->next = entry;
}

/**
 * nih_list_new:
 * @parent: parent object for new list.
 *
 * Allocates a new list structure, usually used as the start of a new
 * list.  You may prefer to allocate the NihList structure statically and
 * use nih_list_init() to initialise it instead.
 *
 * The structure is allocated using nih_alloc() so can be used as a context
 * to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned list.  When all parents
 * of the returned list are freed, the returned list will also be
 * freed.
 *
 * Returns: the new list or NULL if the allocation failed.
 **/
NihList *
nih_list_new (const void *parent)
{
	NihList *list;

	list = nih_new (parent, NihList);
	if (! list)
		return NULL;

	nih_list_init (list);

	nih_alloc_set_destructor (list, nih_list_destroy);

	return list;
}

/**
 * nih_list_entry_new:
 * @parent: parent object for new list entry.
 *
 * Allocates a new list entry structure, leaving the caller to set the
 * data of the entry.
 *
 * The structure is allocated using nih_alloc() so can be used as a context
 * to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned list entry.  When all parents
 * of the returned list entry are freed, the returned list entry will also be
 * freed.
 *
 * Returns: the new list entry or NULL if the allocation failed.
 **/
NihListEntry *
nih_list_entry_new (const void *parent)
{
	NihListEntry *list;

	list = nih_new (parent, NihListEntry);
	if (! list)
		return NULL;

	nih_list_init (&list->entry);

	nih_alloc_set_destructor (list, nih_list_destroy);

	list->data = NULL;

	return list;
}


/**
 * nih_list_add:
 * @list: entry in the destination list,
 * @entry: entry to be added to the list.
 *
 * Adds @entry to a new list immediately before the @list entry.  If @list
 * is the pointer you are using to refer to the list itself, this results
 * in @entry being appended to the list.
 *
 * If @entry is already in another list it is removed so there is no need
 * to call nih_list_remove() before this function.  There is also no
 * requirement that the lists be different, so this can be used to reorder
 * a list.
 *
 * Returns: @entry which is now a member of the same list as @list.
 **/
NihList *
nih_list_add (NihList *list,
	      NihList *entry)
{
	nih_assert (list != NULL);
	nih_assert (entry != NULL);

	nih_list_cut (entry);

	entry->prev = list->prev;
	list->prev->next = entry;
	list->prev = entry;
	entry->next = list;

	return entry;
}

/**
 * nih_list_add_after:
 * @list: entry in the destination list,
 * @entry: entry to be added to the list.
 *
 * Adds @entry to a new list immediately after the @list entry.  If @list
 * is the pointer you are using to refer to the list itself and that entry
 * has no data, this results in @entry being pushed onto a stack under it.
 *
 * If @entry is already in another list it is removed so there is no need
 * to call nih_list_remove() before this function.  There is also no
 * requirement that the lists be different, so this can be used to reorder
 * a list.
 *
 * Returns: @entry which is now a member of the same list as @list.
 **/
NihList *
nih_list_add_after (NihList *list,
		    NihList *entry)
{
	nih_assert (list != NULL);
	nih_assert (entry != NULL);

	nih_list_cut (entry);

	entry->next = list->next;
	list->next->prev = entry;
	list->next = entry;
	entry->prev = list;

	return entry;
}


/**
 * nih_list_cut:
 * @entry: entry to be removed.
 *
 * Removes @entry from its containing list, but does not alter @entry
 * itself; care should be taken to set the pointers immediately after.
 *
 * Returns: @entry unmodified.
 **/
static inline NihList *
nih_list_cut (NihList *entry)
{
	nih_assert (entry != NULL);

	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;

	return entry;
}

/**
 * nih_list_remove:
 * @entry: entry to be removed.
 *
 * Removes @entry from its containing list.  The entry is not freed, but
 * is instead returned so that it can be added to another list (though
 * there's no need to call nih_list_remove() first if you wanted to do
 * that) or used as the start of a new list.
 *
 * Returns: @entry as a lone entry.
 **/
NihList *
nih_list_remove (NihList *entry)
{
	nih_assert (entry != NULL);

	nih_list_cut (entry);
	nih_list_init (entry);

	return entry;
}

/**
 * nih_list_destroy:
 * @entry: entry to be removed.
 *
 * Removes @entry from its containing list.
 *
 * Normally used or called from an nih_alloc() destructor so that the list
 * item is automatically removed from its containing list when freed.
 *
 * Returns: zero.
 **/
int
nih_list_destroy (NihList *entry)
{
	nih_assert (entry != NULL);

	nih_list_cut (entry);

	return 0;
}
