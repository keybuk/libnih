/* libnih
 *
 * child.c - child process termination handling
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


#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>

#include "child.h"


/**
 * WAITOPTS:
 *
 * Options to pass to waitid().
 **/
#define WAITOPTS (WEXITED | WSTOPPED | WCONTINUED)


/**
 * nih_child_watches:
 *
 * This is the list of current child watches, not sorted into any
 * particular order.  Each item is an NihChildWatch structure.
 **/
NihList *nih_child_watches = NULL;


/**
 * nih_child_init:
 *
 * Initialise the list of child watches.
 **/
void
nih_child_init (void)
{
	if (! nih_child_watches)
		nih_child_watches = NIH_MUST (nih_list_new (NULL));
}


/**
 * nih_child_add_watch:
 * @parent: parent object for new watch,
 * @pid: process id to watch or -1,
 * @events: events to watch for,
 * @handler: function to call on @events,
 * @data: pointer to pass to @handler.
 *
 * Adds @handler to the list of functions that should be called by
 * nih_child_poll() if any of the events listed in @events occurs to the
 * process with id @pid.  If @pid is -1 then @handler is called for all
 * children.
 *
 * The watch structure is allocated using nih_alloc() and stored in a linked
 * list; there is no non-allocated version because of this and because it
 * will be automatically freed once called if @pid is not -1 and the event
 * indicates that the process has terminated.
 *
 * Removal of the watch can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned watch.  When all parents
 * of the returned watch are freed, the returned watch will also be
 * freed.
 *
 * Returns: the watch information, or NULL if insufficient memory.
 **/
NihChildWatch *
nih_child_add_watch (const void      *parent,
		     pid_t            pid,
		     NihChildEvents   events,
		     NihChildHandler  handler,
		     void            *data)
{
	NihChildWatch *watch;

	nih_assert (pid != 0);
	nih_assert (handler != NULL);

	nih_child_init ();

	watch = nih_new (parent, NihChildWatch);
	if (! watch)
		return NULL;

	nih_list_init (&watch->entry);

	nih_alloc_set_destructor (watch, nih_list_destroy);

	watch->pid = pid;
	watch->events = events;

	watch->handler = handler;
	watch->data = data;

	nih_list_add (nih_child_watches, &watch->entry);

	return watch;
}


/**
 * nih_child_poll:
 *
 * Repeatedly call waitid() until there are no children waiting to be
 * reaped.  For each child that an event occurs for, the list of child
 * watches is iterated and the handler function for appropriate entries
 * is called.
 *
 * It is safe for the handler to remove itself.
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

	while (waitid (P_ALL, 0, &info, WAITOPTS | WNOHANG) == 0) {
		pid_t          pid;
		NihChildEvents event;
		int            status, free_watch = TRUE;

		pid = info.si_pid;
		if (! pid)
			break;

		/* Convert siginfo information to handler function arguments;
		 * in practice this is mostly just copying, with a few bits
		 * of lore.
		 */
		switch (info.si_code) {
		case CLD_EXITED:
			event = NIH_CHILD_EXITED;
			status = info.si_status;
			break;
		case CLD_KILLED:
			event = NIH_CHILD_KILLED;
			status = info.si_status;
			break;
		case CLD_DUMPED:
			event = NIH_CHILD_DUMPED;
			status = info.si_status;
			break;
		case CLD_TRAPPED:
			if (((info.si_status & 0x7f) == SIGTRAP)
			    && (info.si_status & ~0x7f)) {
				event = NIH_CHILD_PTRACE;
				status = info.si_status >> 8;
			} else {
				event = NIH_CHILD_TRAPPED;
				status = info.si_status;
			}
			free_watch = FALSE;
			break;
		case CLD_STOPPED:
			event = NIH_CHILD_STOPPED;
			status = info.si_status;
			free_watch = FALSE;
			break;
		case CLD_CONTINUED:
			event = NIH_CHILD_CONTINUED;
			status = info.si_status;
			free_watch = FALSE;
			break;
		default:
			nih_assert_not_reached ();
		}

		NIH_LIST_FOREACH_SAFE (nih_child_watches, iter) {
			NihChildWatch *watch = (NihChildWatch *)iter;

			if ((watch->pid != pid) && (watch->pid != -1))
				continue;

			if (! (watch->events & event))
				continue;

			watch->handler (watch->data, pid, event, status);

			if (free_watch && (watch->pid != -1))
				nih_free (watch);
		}

		/* For next waitid call */
		memset (&info, 0, sizeof (info));
	}
}
