/* libnih
 *
 * io.c - file and socket input/output handling
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


#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/logging.h>

#include "io.h"


/**
 * io_watches;
 *
 * This is the list of current watches on file descriptors and sockets,
 * not sorted into any particular order.
 **/
static NihList *io_watches = NULL;


/**
 * nih_io_init:
 *
 * Initialise the list of I/O watches.
 **/
static inline void
nih_io_init (void)
{
	if (! io_watches)
		NIH_MUST (io_watches = nih_list_new ());
}

/**
 * nih_io_add_watch:
 * @fd: file descriptor or socket to watch,
 * @events: events to watch for,
 * @callback: function to call when @events occur on @fd,
 * @data: pointer to pass to @callback.
 *
 * Adds @fd to the list of file descriptors to watch, when any of @events
 * occur on the socket, @callback will be called.  @events is the standard
 * list of #poll events.
 *
 * This is the simplest form of watch and satisfies most purposes.  The watch
 * may be removed by calling #nih_list_remove on the structure returned.
 *
 * Returns: the watch structure, or %NULL if insufficient memory.
 **/
NihIoWatch *
nih_io_add_watch (int      fd,
		  short    events,
		  NihIoCb  callback,
		  void    *data)
{
	NihIoWatch *watch;

	nih_assert (fd >= 0);
	nih_assert (events != 0);
	nih_assert (callback != NULL);

	nih_io_init ();

	watch = nih_new (io_watches, NihIoWatch);
	if (! watch)
		return NULL;

	nih_list_init (&watch->entry);

	watch->fd = fd;
	watch->events = events;

	watch->callback = callback;
	watch->data = data;

	nih_list_add (io_watches, &watch->entry);

	return watch;
}


/**
 * nih_io_poll_fds:
 * @ufds: pointer to store location of array in,
 * @nfds: pointer to store length of array in.
 *
 * Builds an array of pollfd structures suitable for giving to poll
 * containing the list of I/O watches.  The array is allocated using
 * #nih_alloc.
 *
 * Returns: zero on success, negative value on insufficient memory.
 **/
int
nih_io_poll_fds (struct pollfd **ufds,
		 nfds_t         *nfds)
{
	nih_assert (ufds != NULL);
	nih_assert (nfds != NULL);

	nih_io_init ();

	*ufds = NULL;
	*nfds = 0;

	NIH_LIST_FOREACH (io_watches, iter) {
		NihIoWatch    *watch = (NihIoWatch *)iter;
		struct pollfd *new_ufds, *ufd;

		new_ufds = nih_realloc (*ufds, NULL,
					sizeof (struct pollfd) * ++(*nfds));
		if (! new_ufds) {
			nih_free (*ufds);
			return -1;
		}
		*ufds = new_ufds;

		ufd = &(*ufds)[*nfds - 1];
		ufd->fd = watch->fd;
		ufd->events = watch->events;
		ufd->revents = 0;
	}

	return 0;
}

/**
 * nih_io_handle_fds:
 * @ufds: array of poll information,
 * @nfds: length of array.
 *
 * Recieves an array of pollfd structures which has had the revents
 * members filled in by a call to #poll and iterates the watch list
 * calling the appropriate callbacks.
 *
 * It is safe for callbacks to remove the watch during their call.
 **/
void
nih_io_handle_fds (const struct pollfd *ufds,
		   nfds_t               nfds)
{
	nih_assert (ufds != NULL);
	nih_assert (nfds > 0);

	nih_io_init ();

	NIH_LIST_FOREACH_SAFE (io_watches, iter) {
		NihIoWatch *watch = (NihIoWatch *)iter;
		nfds_t              i;

		for (i = 0; i < nfds; i++) {
			if (ufds[i].fd != watch->fd)
				continue;

			if (! (ufds[i].revents & watch->events))
				continue;

			watch->callback (watch->data, watch, ufds[i].revents);
		}
	}
}
