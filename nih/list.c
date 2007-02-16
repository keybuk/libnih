/* libnih
 *
 * list.c - generic circular doubly-linked list implementation
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
 * @parent: parent of new list.
 *
 * Allocates a new list structure, usually used as the start of a new
 * list.  You may prefer to allocate the NihList structure statically and
 * use nih_list_init() to initialise it instead.
 *
 * The structure is allocated using nih_alloc() so can be used as a context
 * to other allocations.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the new list entry or NULL if the allocation failed.
 **/
NihList *
nih_list_new (const void *parent)
{
	NihList *list;

	list = nih_new (parent, NihList);
	if (! list)
		return NULL;

	nih_list_init (list);

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
 * nih_list_destructor:
 * @entry: entry to be removed.
 *
 * Removes @entry from its containing list, intended to be used as an
 * nih_alloc() destructor so that the list item is automatically removed if
 * it is freed.
 *
 * Returns: zero.
 **/
int
nih_list_destructor (NihList *entry)
{
	nih_assert (entry != NULL);

	nih_list_cut (entry);

	return 0;
}

/**
 * nih_list_free:
 * @entry: entry to be removed and freed.
 *
 * Removes @entry from its containing list and frees the memory allocated
 * for it.  @entry must have been previously allocated using nih_alloc().
 *
 * You must take care of freeing the data attached to the entry yourself
 * by either freeing it before calling this function or allocating it using
 * the list entry as the context.
 *
 * Returns: return value from destructor, or 0.
 **/
int
nih_list_free (NihList *entry)
{
	nih_assert (entry != NULL);

	nih_list_cut (entry);
	return nih_free (entry);
}
