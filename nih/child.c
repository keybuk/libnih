/* libnih
 *
 * child.c - child process termination handling
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


#include <sys/types.h>
#include <sys/wait.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>

#include "child.h"


/**
 * child_watches:
 *
 * This is the list of current child watches, not sorted into any
 * particular order.
 **/
static NihList *child_watches = NULL;


/**
 * nih_child_init:
 *
 * Initialise the list of child watches.
 **/
static inline void
nih_child_init (void)
{
	if (! child_watches)
		NIH_MUST (child_watches = nih_list_new ());
}


/**
 * nih_child_add_watch:
 * @pid: process id to watch or -1,
 * @reaper: function to call on termination,
 * @data: pointer to pass to @reaper.
 *
 * Adds @reaper to the list of functions that should be called by
 * #nih_child_poll if the process with id @pid terminates.  If @pid is -1
 * then @reaper is called for all children.
 *
 * The watch may be removed by calling #nih_list_remove on the returned
 * structure.
 *
 * Returns: the watch information, or %NULL if insufficient memory.
 **/
NihChildWatch *
nih_child_add_watch (pid_t      pid,
		     NihReaper  reaper,
		     void      *data)
{
	NihChildWatch *watch;

	nih_assert (pid != 0);
	nih_assert (reaper != NULL);

	nih_child_init ();

	watch = nih_new (child_watches, NihChildWatch);
	if (! watch)
		return NULL;

	nih_list_init (&watch->entry);

	watch->pid = pid;

	watch->reaper = reaper;
	watch->data = data;

	nih_list_add (child_watches, &watch->entry);

	return watch;
}


/**
 * nih_child_poll:
 *
 * Repeatedly call #waitpid until there are no children waiting to be
 * reaped.  For each child that has terminated, the list of child watches
 * is iterated and the reaper function for appropriate entries called.
 *
 * It is safe for the reaper to remove itself.
 **/
void
nih_child_poll (void)
{
	pid_t pid;
	int   status;

	while ((pid = waitpid (-1, &status, WNOHANG)) > 0) {
		NIH_LIST_FOREACH_SAFE (child_watches, iter) {
			NihChildWatch *watch = (NihChildWatch *)iter;

			if ((watch->pid != pid) && (watch->pid != -1))
				continue;

			watch->reaper (watch->data, pid, status);

			if (watch->pid != -1)
				nih_list_free (&watch->entry);
		}
	}
}
