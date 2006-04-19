/* libnih
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

#ifndef NIH_LIST_H
#define NIH_LIST_H

#include <nih/macros.h>


/**
 * NihList:
 * @prev: previous entry in the list,
 * @next: next entry in the list.
 *
 * This structure can be used both to refer to a linked list and can be
 * placed in your own structures to use them as list entries.
 *
 * The list is circular so the @next pointer of the last entry points to
 * the first, and the @prev pointer of the first entry points to the last.
 * An empty list simply has the @prev and @next pointers pointing to itself.
 **/
typedef struct nih_list {
	struct nih_list *prev, *next;
} NihList;


/**
 * NIH_LIST_EMPTY:
 * @list: entry in the list to check.
 *
 * Checks whether the given list is empty.
 *
 * Returns: TRUE if empty, FALSE otherwise.
 **/
#define NIH_LIST_EMPTY(list) ((list)->prev == (list)->next)


NIH_BEGIN_EXTERN

void          nih_list_init      (NihList *entry);
NihList *     nih_list_new       (void);

NihList *     nih_list_remove    (NihList *entry);
void          nih_list_free      (NihList *entry);

NihList *     nih_list_add       (NihList *list, NihList *entry);
NihList *     nih_list_add_after (NihList *list, NihList *entry);

NIH_END_EXTERN

#endif /* NIH_LIST_H */
