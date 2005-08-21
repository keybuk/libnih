/* libnih
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

#ifndef NIH_LIST_H
#define NIH_LIST_H

#include <nih/macros.h>


/**
 * NihList:
 * @prev: previous entry in the list,
 * @next: next entry in the list,
 * @data: pointer to data attached to the entry.
 *
 * This structure is used for each entry in the list, and as the "first"
 * and "last" entries point to each other, can be used to refer to any
 * entry in the list.
 */
typedef struct nih_list_entry {
	struct nih_list_entry *prev, *next;

	void *data;
} NihList;


/**
 * nih_list_add_new:
 * @list: entry in the destination list,
 * @data: data to attach to the new entry.
 *
 * Allocates a new list entry, sets the data member to @data and then
 * adds it to a new list immediately before the @list entry.  If @list
 * is the pointer you are using to refer to the list itself, this results
 * in @entry being appended to the list.
 *
 * Returns: the new entry.
 */
#define nih_list_add_new(list, data) nih_list_add ((list), nih_list_new (data))

/**
 * nih_list_add_new_after:
 * @list: entry in the destination list,
 * @data: data to attach to the new entry.
 *
 * Allocates a new list entry, sets the data member to @data and then
 * adds it to a new list immediately after the @list entry.  If @list
 * is the pointer you are using to refer to the list itself and that entry
 * has no data, this results in @entry being pushed onto a stack under it.
 *
 * Returns: the new entry.
 */
#define nih_list_add_new_after(list, data) nih_list_add_after ((list), \
							   nih_list_new (data))


/**
 * NihListIter:
 *
 * Opaque type used to iterate a list; must be initialised to %NULL before
 * use.  You should not use this as anything other than an argument to the
 * iteration macros, and should not rely on its content or type.
 *
 * Example:
 * | NihListIter  iter = NULL;
 * | NihList     *e;
 * |
 * | for (e = list; !NIH_LIST_LAST (list, iter); e = NIH_LIST_NEXT (list, iter))
 * |         printf ("%s\n", e->data);
 *
 * Every entry in the list is visited, including the entry given to refer
 * to the list.  If you don't want to visit that entry, you don't need to
 * use the iterator macros and can instead use code like:
 * | for (e = list->next; e != list; e = e->next)
 */
typedef NihList * NihListIter;

/**
 * NIH_LIST_FIRST:
 * @list: list being iterated,
 * @iter: list iterator.
 *
 * Macro to return whether the entry currently being visited by the
 * iterator @iter is the first one in the list.
 *
 * Returns: %TRUE if the entry is the first, %FALSE otherwise.
 */
#define NIH_LIST_FIRST(list, iter) ((iter) == NULL)

/**
 * NIH_LIST_LAST:
 * @list: list being iterated,
 * @iter: list iterator.
 *
 * Macro to return whether every entry in the list has been visited by
 * the iterator @iter.
 *
 * If this macro returns true, the iterator is reset to %NULL so it can be
 * used again without requiring reinitialisation.
 *
 * Returns: %TRUE if list has been completely iterated, %FALSE otherwise.
 */
#define NIH_LIST_LAST(list, iter) ((iter) == (list) \
				 ? (iter) = NULL, TRUE : FALSE)

/**
 * NIH_LIST_PREV:
 * @list: list being iterated,
 * @iter: list iterator.
 *
 * Macro to advance an iterator onto the previous entry in the list, used
 * when iterating a list in reverse or just going back on yourself.
 *
 * Returns: the previous entry.
 */
#define NIH_LIST_PREV(list, iter) ((iter) = (iter) ? (iter)->prev : (list)->prev)

/**
 * NIH_LIST_NEXT:
 * @list: list being iterated,
 * @iter: list iterator.
 *
 * Macro to advance an iterator onto the next entry in the list.
 *
 * Returns: the next entry.
 */
#define NIH_LIST_NEXT(list, iter) ((iter) = (iter) ? (iter)->next : (list)->next)


NIH_BEGIN_EXTERN

NihList *nih_list_new       (void *data);
NihList *nih_list_remove    (NihList *entry);
void     nih_list_free      (NihList *entry);
NihList *nih_list_add       (NihList *list, NihList *entry);
NihList *nih_list_add_after (NihList *list, NihList *entry);

NIH_END_EXTERN

#endif /* NIH_LIST_H */
