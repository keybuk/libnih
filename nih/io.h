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

#ifndef NIH_IO_H
#define NIH_IO_H

#include <sys/poll.h>

#include <nih/macros.h>
#include <nih/list.h>


typedef struct nih_io_watch NihIoWatch;

/**
 * NihIoCb:
 *
 * An I/O callback is a function called when an event occurs on a file
 * descriptor or socket.  It is passed the #NihIoWatch associated with it
 * and the events that caused it to be triggered.
 **/
typedef void (*NihIoCb) (void *, NihIoWatch *, short);


/**
 * NihIoWatch:
 * @entry: list header,
 * @fd: file descriptor,
 * @events: events to watch for,
 * @callback: function called when @events occur on @fd,
 * @data: pointer passed to @callback.
 *
 * This structure represents the most basic kind of I/O handling, a watch
 * on a file descriptor or socket that causes a function to be called
 * when listed events occur.
 *
 * The watch can be cancelled by calling #nih_list_remove on the structure
 * as they are held in a list internally.
 **/
struct nih_io_watch {
	NihList  entry;
	int      fd;
	short    events;

	NihIoCb  callback;
	void    *data;
};


NIH_BEGIN_EXTERN

NihIoWatch *nih_io_add_watch  (int fd, short events,
			       NihIoCb callback, void *data);

int         nih_io_poll_fds   (struct pollfd **ufds, nfds_t *nfds);
void        nih_io_handle_fds (const struct pollfd *ufds, nfds_t nfds);

NIH_END_EXTERN

#endif /* NIH_IO_H */
