/* libnih
 *
 * list.c - generic circular doubly-linked list implementation
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


#include <stdlib.h>

#include <nih/macros.h>

#include "list.h"


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
	/* FIXME check entry is not NULL */

	entry->prev = entry->next = entry;
}

/**
 * nih_list_new:
 *
 * Allocates a new list structure, usually used as the start of a new
 * list.  You may prefer to allocate the #NihList structure statically and
 * use #nih_list_init to initialise it instead.
 *
 * Returns: the new list entry.
 **/
NihList *
nih_list_new (void)
{
	NihList *list;

	/* FIXME use nih_alloc */
	list = malloc (sizeof (NihList));
	nih_list_init (list);

	return list;
}

/**
 * nih_list_entry_new:
 * @data: data to attach to the new entry.
 *
 * Allocates a new list entry and sets the data member to @data, the new
 * entry can be added to any existing list.
 *
 * Returns: the new list entry..
 **/
NihListEntry *
nih_list_entry_new (void *data)
{
	NihListEntry *entry;

	/* FIXME use nih_alloc */
	entry = malloc (sizeof (NihListEntry));
	nih_list_init ((NihList *)entry);

	entry->data = data;

	return entry;
}


/**
 * nih_list_remove:
 * @entry: entry to be removed.
 *
 * Removes @entry from its containing list.  The entry is not freed, but
 * is instead returned so that it can be added to another list (though
 * there's no need to call #nih_list_remove first if you wanted to do that)
 * or used as the start of a new list.
 *
 * Returns: @entry as a lone entry.
 **/
NihList *
nih_list_remove (NihList *entry)
{
	/* FIXME assert entry is not NULL */

	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;

	nih_list_init (entry);

	return entry;
}

/**
 * nih_list_free:
 * @entry: entry to be removed and freed.
 *
 * Removes @entry from its containing list and frees the memory allocated
 * for it.
 *
 * You must take care of freeing the data attached to the entry yourself
 * by either freeing it before calling this function or allocating it using
 * the list entry as the context.
 **/
void
nih_list_free (NihList *entry)
{
	/* FIXME assert entry is not NULL */

	nih_list_remove (entry);

	/* FIXME nih_free */
	free (entry);
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
 * to call #nih_list_remove before this function.  There is also no
 * requirement that the lists be different, so this can be used to reorder
 * a list.
 *
 * Returns: @entry which is now a member of the same list as @list.
 **/
NihList *
nih_list_add (NihList *list,
	      NihList *entry)
{
	/* FIXME assert list and entry are not NULL */

	nih_list_remove (entry);

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
 * to call #nih_list_remove before this function.  There is also no
 * requirement that the lists be different, so this can be used to reorder
 * a list.
 *
 * Returns: @entry which is now a member of the same list as @list.
 **/
NihList *
nih_list_add_after (NihList *list,
		    NihList *entry)
{
	/* FIXME assert list and entry are not NULL */

	nih_list_remove (entry);

	entry->next = list->next;
	list->next->prev = entry;
	list->next = entry;
	entry->prev = list;

	return entry;
}
