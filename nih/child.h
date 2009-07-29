/* libnih
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

#ifndef NIH_CHILD_H
#define NIH_CHILD_H

#include <sys/types.h>
#include <sys/wait.h>

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihChildEvents:
 *
 * Events that can occur for child processes, and used to determine the
 * content of the translated status field.  For NIH_CHILD_EXITED this will
 * contain the exit status of the program; for NIH_CHILD_PTRACE this will
 * contain one of the PTRACE_EVENT_* constants; otherwise this will
 * contain the signal that killed, dumped, stopped or continued the process
 * or was trapped through ptrace.
 **/
typedef enum {
	NIH_CHILD_NONE      = 0000,
	NIH_CHILD_EXITED    = 0001,
	NIH_CHILD_KILLED    = 0002,
	NIH_CHILD_DUMPED    = 0004,
	NIH_CHILD_STOPPED   = 0010,
	NIH_CHILD_CONTINUED = 0020,
	NIH_CHILD_TRAPPED   = 0040,
	NIH_CHILD_PTRACE    = 0100,
	NIH_CHILD_ALL       = 0177
} NihChildEvents;

/**
 * NihChildHandler:
 * @data: data pointer given with callback,
 * @pid: process that changed,
 * @event: event that occurred on the child,
 * @status: exit status of process, signal that killed it or ptrace event.
 *
 * A child handler is a function called for events on the child process
 * obtained through waitid().
 **/
typedef void (*NihChildHandler) (void *data, pid_t pid,
				 NihChildEvents event, int status);

/**
 * NihChildWatch:
 * @entry: list header,
 * @pid: process id to watch or -1,
 * @events: events to watch for,
 * @handler: function called when events occur to child,
 * @data: pointer passed to @reaper.
 *
 * This structure represents a watch on a particular child, the @reaper
 * function is called when an event in @events occurs to a child with
 * process id @pid.  If @pid is -1 then this function is called when @events
 * occur for all processes.
 *
 * The watch can be cancelled by calling nih_list_remove() on the structure
 * as they are held in a list internally.
 **/
typedef struct nih_child_watch {
	NihList          entry;
	pid_t            pid;
	NihChildEvents   events;

	NihChildHandler  handler;
	void            *data;
} NihChildWatch;


NIH_BEGIN_EXTERN

extern NihList *nih_child_watches;


void           nih_child_init      (void);

NihChildWatch *nih_child_add_watch (const void *parent, pid_t pid,
				    NihChildEvents events,
				    NihChildHandler handler, void *data)
	__attribute__ ((warn_unused_result, malloc));

void           nih_child_poll      (void);

NIH_END_EXTERN

#endif /* NIH_CHILD_H */
