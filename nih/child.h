/* libnih
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

#ifndef NIH_CHILD_H
#define NIH_CHILD_H

#include <sys/types.h>
#include <sys/wait.h>

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihReaper:
 * @data: data pointer given with callback,
 * @pid: process that died,
 * @killed: TRUE if the process was killed by a signal, FALSE otherwise,
 * @status: exit status of process or signal that killed it.
 *
 * A reaper is a function called once a child process has terminated
 * and its status obtained.  The child is not actually cleaned up fully
 * until these functions have been called, so it still exists in the process
 * table.
 **/
typedef void (*NihReaper) (void *data, pid_t pid, int killed, int status);

/**
 * NihChildWatch:
 * @entry: list header,
 * @pid: process id to watch or -1,
 * @reaper: function called when child terminates,
 * @data: pointer passed to @reaper.
 *
 * This structure represents a watch on a particular child, the @reaper
 * function is called when the child with process id @pid terminates.
 * If @pid is -1 then this function is called when all processes
 * terminate.
 *
 * The watch can be cancelled by calling nih_list_remove() on the structure
 * as they are held in a list internally.
 **/
typedef struct nih_child_watch {
	NihList    entry;
	pid_t      pid;

	NihReaper  reaper;
	void      *data;
} NihChildWatch;


NIH_BEGIN_EXTERN

NihChildWatch *nih_child_add_watch (const void *parent, pid_t pid,
				    NihReaper reaper, void *data)
	__attribute__ ((warn_unused_result, malloc));

void           nih_child_poll      (void);

NIH_END_EXTERN

#endif /* NIH_CHILD_H */
