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

#ifndef NIH_CHILD_H
#define NIH_CHILD_H

#include <sys/types.h>
#include <sys/wait.h>

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihReaper:
 *
 * A reaper is a function called once a child process has terminated
 * and its status obtained.  It is called with the process id of the
 * terminated child, whether the child was killed and its exit status,
 * if the third argument is true then the last argument is the signal that
 * killed it rather than the exit status.
 **/
typedef void (*NihReaper) (void *, pid_t, int, int);

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
 * The watch can be cancelled by calling #nih_list_remove on the structure
 * as they are held in a list internally.
 **/
typedef struct nih_child_watch {
	NihList    entry;
	pid_t      pid;

	NihReaper  reaper;
	void      *data;
} NihChildWatch;


NIH_BEGIN_EXTERN

NihChildWatch *nih_child_add_watch (void *parent, pid_t pid,
				    NihReaper reaper, void *data);

void           nih_child_poll      (void);

NIH_END_EXTERN

#endif /* NIH_CHILD_H */
