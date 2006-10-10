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

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>

#include "child.h"


/**
 * child_watches:
 *
 * This is the list of current child watches, not sorted into any
 * particular order.  Each item is an NihChildWatch structure.
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
		NIH_MUST (child_watches = nih_list_new (NULL));
}


/**
 * nih_child_add_watch:
 * @parent: parent of watch,
 * @pid: process id to watch or -1,
 * @reaper: function to call on termination,
 * @data: pointer to pass to @reaper.
 *
 * Adds @reaper to the list of functions that should be called by
 * nih_child_poll() if the process with id @pid terminates.  If @pid is -1
 * then @reaper is called for all children.
 *
 * The watch structure is allocated using nih_alloc() and stored in a linked
 * list, a default destructor is set that removes the watch from the list.
 * Removal of the watch can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: the watch information, or NULL if insufficient memory.
 **/
NihChildWatch *
nih_child_add_watch (const void *parent,
		     pid_t       pid,
		     NihReaper   reaper,
		     void       *data)
{
	NihChildWatch *watch;

	nih_assert (pid != 0);
	nih_assert (reaper != NULL);

	nih_child_init ();

	watch = nih_new (parent, NihChildWatch);
	if (! watch)
		return NULL;

	nih_list_init (&watch->entry);
	nih_alloc_set_destructor (watch, (NihDestructor)nih_list_destructor);

	watch->pid = pid;

	watch->reaper = reaper;
	watch->data = data;

	nih_list_add (child_watches, &watch->entry);

	return watch;
}


/**
 * nih_child_poll:
 *
 * Repeatedly call waitid() until there are no children waiting to be
 * reaped.  For each child that has terminated, the list of child watches
 * is iterated and the reaper function for appropriate entries called.
 *
 * It is safe for the reaper to remove itself.
 **/
void
nih_child_poll (void)
{
	siginfo_t info;

	nih_child_init ();

	/* NOTE: there's a strange kernel inconsistency, when the waitid()
	 * syscall is native, it takes special care to zero this struct
	 * before returning ... but when it's a compat syscall, it
	 * specifically *doesn't* zero the struct.
	 *
	 * So we have to take care to do it ourselves before every call.
	 */
	memset (&info, 0, sizeof (info));

	while (waitid (P_ALL, 0, &info, WEXITED | WNOHANG | WNOWAIT) == 0) {
		pid_t pid;
		int   killed, status;

		pid = info.si_pid;
		if (! pid)
			break;

		killed = info.si_code == CLD_KILLED ? TRUE : FALSE;
		status = info.si_status;

		NIH_LIST_FOREACH_SAFE (child_watches, iter) {
			NihChildWatch *watch = (NihChildWatch *)iter;

			if ((watch->pid != pid) && (watch->pid != -1))
				continue;

			watch->reaper (watch->data, pid, killed, status);

			if (watch->pid != -1)
				nih_list_free (&watch->entry);
		}

		/* Reap the child */
		memset (&info, 0, sizeof (info));
		waitid (P_PID, pid, &info, WEXITED);

		/* For next waitid call */
		memset (&info, 0, sizeof (info));
	}
}
