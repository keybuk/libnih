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


#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/signal.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "io.h"


/* Prototypes for static functions */
static inline void nih_io_buffer_shrink (NihIoBuffer *buffer, size_t len);
static void        nih_io_cb            (NihIo *io, NihIoWatch *watch,
					 short events);
static void        nih_io_closed        (NihIo *io);
static void        nih_io_error         (NihIo *io);


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
 * @parent: parent of watch,
 * @fd: file descriptor or socket to watch,
 * @events: events to watch for,
 * @callback: function to call when @events occur on @fd,
 * @data: pointer to pass to @callback.
 *
 * Adds @fd to the list of file descriptors to watch, when any of @events
 * occur on the socket, @callback will be called.  @events is the standard
 * list of #poll events.
 *
 * This is the simplest form of watch and satisfies most purposes.
 *
 * The watch structure is allocated using #nih_alloc and stored in a linked
 * list, a default destructor is set that removes the watch from the list.
 * Removal of the watch can be performed by freeing it.
 *
 * Returns: the watch structure, or %NULL if insufficient memory.
 **/
NihIoWatch *
nih_io_add_watch (void    *parent,
		  int      fd,
		  short    events,
		  NihIoCb  callback,
		  void    *data)
{
	NihIoWatch *watch;

	nih_assert (fd >= 0);
	nih_assert (callback != NULL);

	nih_io_init ();

	watch = nih_new (parent, NihIoWatch);
	if (! watch)
		return NULL;

	nih_list_init (&watch->entry);
	nih_alloc_set_destructor (watch, (NihDestructor)nih_list_destructor);

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

			if (! ufds[i].revents)
				continue;

			watch->callback (watch->data, watch, ufds[i].revents);
		}
	}
}


/**
 * nih_io_buffer_new:
 * @parent: parent of new buffer.
 *
 * Allocates a new #NihIoBuffer structure containing an empty buffer,
 * all functions that use the buffer ensure that the internal data is
 * an #nih_alloc child of the buffer, so this may be freed using #nih_free.
 *
 * Returns: new buffer, or %NULL if insufficient memory.
 **/
NihIoBuffer *
nih_io_buffer_new (void *parent)
{
	NihIoBuffer *buffer;

	buffer = nih_new (parent, NihIoBuffer);
	if (! buffer)
		return NULL;

	buffer->buf = NULL;
	buffer->size = 0;
	buffer->len = 0;

	return buffer;
}

/**
 * nih_io_buffer_resize:
 * @buffer: buffer to be resized,
 * @grow: number of bytes to grow by.
 *
 * This function resizes the given @buffer so there is enough space for
 * both the current data and @grow additional bytes (which may be zero).
 * If there is more room than there needs to be, the buffer may actually
 * be decreased in size.
 *
 * Returns: zero on success, %NULL if insufficient memory.
 **/
int
nih_io_buffer_resize (NihIoBuffer *buffer,
		      size_t       grow)
{
	char   *new_buf;
	size_t  new_len, new_size;

	nih_assert (buffer != NULL);
	nih_assert (grow >= 0);

	new_len = buffer->len + grow;
	if (! new_len) {
		/* No bytes to store, so clean up the buffer */
		if (buffer->buf)
			nih_free (buffer->buf);

		buffer->buf = NULL;
		buffer->size = 0;

		return 0;
	}

	/* Round buffer to next largest multiple of BUFSIZ */
	new_size = ((new_len - 1) / BUFSIZ) * BUFSIZ + BUFSIZ;
	if (new_size == buffer->size)
		return 0;

	/* Adjust buffer memory */
	new_buf = nih_realloc (buffer->buf, buffer, new_size);
	if (! new_buf)
		return -1;

	/* Note: don't adjust the length */
	buffer->buf = new_buf;
	buffer->size = new_size;

	return 0;
}

/**
 * nih_io_buffer_pop:
 * @parent: parent of new pointer,
 * @buffer: buffer to shrink,
 * @len: bytes to take.
 *
 * Takes @len bytes from the start of @buffer, reducing the size if
 * necessary, and returns them in a newly allocated string.  It is
 * illegal to request more bytes than are available in the buffer.
 *
 * The returned string is always %NULL terminated, even if there was
 * not a %NULL in the buffer.
 *
 * Returns: newly allocated data pointer, or %NULL if insufficient memory.
 **/
char *
nih_io_buffer_pop (void        *parent,
		   NihIoBuffer *buffer,
		   size_t       len)
{
	char *str;

	nih_assert (buffer != NULL);
	nih_assert (len <= buffer->len);

	str = nih_alloc (parent, len + 1);
	if (! str)
		return NULL;

	/* Copy the data into the new string and add NULL */
	memcpy (str, buffer->buf, len);
	str[len] = '\0';

	/* Move the buffer up */
	nih_io_buffer_shrink (buffer, len);

	/* Don't worry if this fails, it just means the buffer is larger
	 * than it needs to be.
	 */
	nih_io_buffer_resize (buffer, 0);

	return str;
}

/**
 * nih_io_buffer_shrink:
 * @buffer: buffer to shrink,
 * @len: bytes to remove from the front.
 *
 * Removes @len bytes from the beginning of @buffer and moves the rest
 * of the data up to begin there.
 **/
static inline void
nih_io_buffer_shrink (NihIoBuffer *buffer,
		      size_t       len)
{
	nih_assert (buffer != NULL);
	nih_assert (len <= buffer->len);

	memmove (buffer->buf, buffer->buf + len, buffer->len - len);
	buffer->len -= len;

}

/**
 * nih_io_buffer_push:
 * @buffer: buffer to extend,
 * @str: data to push,
 * @len: length of @str.
 *
 * Pushes @len bytes from @str onto the end of @buffer, increasing the size
 * if necessary.
 *
 * Returns: zero on success, %NULL if insufficient memory.
 **/
int
nih_io_buffer_push (NihIoBuffer *buffer,
		    const char  *str,
		    size_t       len)
{
	nih_assert (buffer != NULL);
	nih_assert (str != NULL);

	if (nih_io_buffer_resize (buffer, len) < 0)
		return -1;

	/* Copy the data into the buffer */
	memcpy (buffer->buf + buffer->len, str, len);
	buffer->len += len;

	return 0;
}


/**
 * nih_io_reopen:
 * @parent: parent pointer of new structure,
 * @fd: file descriptor to manage,
 * @read_cb: function to call when new data available,
 * @close_cb: function to call on close,
 * @error_cb: function to call on error,
 * @data: data to pass to callbacks.
 *
 * This allocates and returns an #NihIo structure for managing an already
 * opened file descriptor.  The file descriptor is set to be non-blocking
 * if it hasn't already been and the SIGPIPE signal is set to be ignored.
 *
 * If @read_cb is given then all data is automatically read from the
 * file descriptor and stored in a buffer and this function is called
 * whenever there is new data available.  The function is under no
 * obligation to remove any data, it's perfectly valid to leave it in the
 * buffer until next time.
 *
 * If @close_cb is given then it is called whenever the remote end of the
 * file descriptor is closed, otherwise the local end is closed and the
 * entire structure freed (which may be surprising to you).
 *
 * If @error_cb is given then any it is called whenever any errors are
 * raised, otherwise the same action as for @close_cb is taken (including
 * that callback).
 *
 * Returns: newly allocated structure, or %NULL if insufficient memory.
 **/
NihIo *
nih_io_reopen (void         *parent,
	       int           fd,
	       NihIoReadCb   read_cb,
	       NihIoCloseCb  close_cb,
	       NihIoErrorCb  error_cb,
	       void         *data)
{
	NihIo *io;

	nih_assert (fd >= 0);

	io = nih_new (parent, NihIo);
	if (! io)
		return NULL;

	io->read_cb = read_cb;
	io->close_cb = close_cb;
	io->error_cb = error_cb;
	io->data = data;
	io->shutdown = FALSE;

	/* Only poll for input if there's actually a callback */
	io->watch = nih_io_add_watch (io, fd, (read_cb ? POLLIN : 0),
				      (NihIoCb) nih_io_cb, io);
	if (! io->watch)
		goto error;

	io->send_buf = nih_io_buffer_new (io);
	if (! io->send_buf)
		goto error;

	io->recv_buf = nih_io_buffer_new (io);
	if (! io->recv_buf)
		goto error;

	/* Irritating signal, means we terminate if the remote end
	 * disconnects between a read() and a write() ... far better to
	 * just get an errno!
	 */
	nih_signal_set_ignore (SIGPIPE);

	/* We want to be able to repeatedly call read and write on the
	 * file descriptor so we always get maximum throughput, and we
	 * don't want to end up blocking; so set the socket so that
	 * doesn't happen.
	 */
	if (nih_io_set_nonblock (fd) < 0)
		nih_free (nih_error_get ());

	return io;
error:
	nih_free (io);
	return NULL;
}


/**
 * nih_io_cb:
 * @io: structure with callbacks,
 * @watch: #NihIoWatch for which an event occurred,
 * @events: events that occurred.
 *
 * This is the callback function associated with all file descriptors
 * being managed by #NihIo.  It ensures that data is read from the file
 * descriptor into the recv buffer and the read_cb called, any data in
 * the send buffer is written to the socket and any errors are handled
 **/
static void
nih_io_cb (NihIo      *io,
	   NihIoWatch *watch,
	   short       events)
{
	/* There's data to be read */
	if (events & ~POLLOUT) {
		ssize_t len;

		/* Read directly into the buffer to save hauling temporary
		 * blocks around; always make sure there's room for at
		 * least 80 bytes (random minimum read).  Make sure we call
		 * read as many times as necessary to exhaust the socket
		 * so we can get maximum throughput.
		 */
		do {
			if (nih_io_buffer_resize (io->recv_buf, 80) < 0)
				/* Out of memory; just handle what we have */
				break;

			len = read (watch->fd,
				    io->recv_buf->buf + io->recv_buf->len,
				    io->recv_buf->size - io->recv_buf->len);
			if (len > 0)
				io->recv_buf->len += len;
		} while (len > 0);

		/* Call the callback if we have any data in the buffer.
		 * This could be called simply because we're about to error,
		 * but it means we give it one last chance to process.
		 */
		if (io->recv_buf->len) {
			if (io->read_cb) {
				io->read_cb (io->data, io, io->recv_buf->buf,
					     io->recv_buf->len);
			} else {
				/* No read callback, just discard */
				nih_io_buffer_shrink (io->recv_buf,
						      io->recv_buf->len);
			}
		}

		/* Deal with errors */
		if ((len < 0) && (errno != EAGAIN) && (errno != EINTR)) {
			nih_error_raise_system ();
			nih_io_error (io);
			return;
		}

		/* Deal with socket being closed */
		if (! len) {
			nih_io_closed (io);
			return;
		}
	}

	/* There's room to write data, send as much as we can */
	if (events & POLLOUT) {
		ssize_t len;

		/* Write directly from the buffer to save hauling temporary
		 * blocks around, and call write() as many times as necessary
		 * to exhaust the buffer so we can get maximum throughput.
		 */
		while (io->send_buf->len) {
			len = write (watch->fd, io->send_buf->buf,
				     io->send_buf->len);

			/* Don't bother checking for errors, it'll poll for
			 * ERR, HUP on IN next time round and we can get
			 * them that way.
			 */
			if (len <= 0)
				break;

			nih_io_buffer_shrink (io->send_buf, len);
		}

		/* Don't poll for write if we have nothing to write */
		if (! io->send_buf->len)
			watch->events &= ~POLLOUT;

		/* Resize the buffer to avoid memory wastage */
		nih_io_buffer_resize (io->send_buf, 0);
	}

	/* Shut down the socket if it has empty buffers */
	if (io->shutdown && (! io->send_buf->len) && (! io->recv_buf->len))
		nih_io_closed (io);
}

/**
 * nih_io_error:
 * @io: structure error occurred for.
 *
 * This function is called to deal with errors that have occurred on a
 * file descriptor being managed by #NihIo.
 *
 * Normally this just calls the error callback, or if not available, it
 * behaves as if the remote end was closed.
 **/
static void
nih_io_error (NihIo *io)
{
	nih_assert (io != NULL);

	if (io->error_cb) {
		io->error_cb (io->data, io);
	} else {
		nih_free (nih_error_get ());
		nih_io_closed (io);
	}
}

/**
 * nih_io_closed:
 * @io: structure to be closed.
 *
 * This function is called when the local end of a file descriptor being
 * managed by #NihIo should be closed.  Usually this is because the remote
 * end has been closed (without error) but it can also be because no
 * error callback was given
 *
 * Normally this just calls the close callback, or if not available, it
 * closes the file descriptor and frees the structure (which may be
 * surprising if you were hanging on to a pointer of it).
 *
 * Note that this can result in the error callback getting called if
 * there's an error caught by closing the socket.
 **/
static void
nih_io_closed (NihIo *io)
{
	nih_assert (io != NULL);

	if (io->close_cb) {
		io->close_cb (io->data, io);
	} else {
		nih_io_close (io);
	}
}

/**
 * nih_io_shutdown:
 * @io: structure to be closed.
 *
 * Marks the #NihIo structure to be closed once the buffers have been
 * emptied, rather than immediately.  Closure is performed by calling
 * the close callback if given or #nih_io_close.
 *
 * This is most useful to send a burst of data and discard the structure
 * once the data has been sent, without worrying about keeping track of
 * the structure in the mean time.
 **/
void
nih_io_shutdown (NihIo *io)
{
	nih_assert (io != NULL);

	io->shutdown = TRUE;
}

/**
 * nih_io_close:
 * @io: structure to be closed.
 *
 * Closes the file descriptor associated with an #NihIo structure and
 * frees the structure.  If an error is caught by closing the descriptor,
 * it is handled through the error callback rather than raised.
 **/
void
nih_io_close (NihIo *io)
{
	nih_assert (io != NULL);

	if ((close (io->watch->fd) < 0) && io->error_cb) {
		nih_error_raise_system ();
		io->error_cb (io->data, io);
	}

	nih_free (io);
}


/**
 * nih_io_read:
 * @parent: parent of new string,
 * @io: structure to read from,
 * @len: number of bytes to read.
 *
 * Reads @len bytes from the receive buffer of @io and returns the data
 * in a newly allocated string which is always %NULL terminated even
 * if there was not a %NULL in the buffer.
 *
 * It is illegal to request more bytes than exist in the bufferr.
 *
 * Returns: newly allocated string, or %NULL if insufficient memory.
 **/
char *
nih_io_read (void   *parent,
	     NihIo  *io,
	     size_t  len)
{
	nih_assert (io != NULL);

	return nih_io_buffer_pop (parent, io->recv_buf, len);
}

/**
 * nih_io_write:
 * @io: structure to write to,
 * @str: data to write,
 * @len: length of @str.
 *
 * Writes @len bytes from @str into the send buffer of @io, the data will
 * not be sent immediately but whenever possible.
 *
 * Care should be taken to ensure @len does not include the %NULL
 * terminator unless you really want that sent.
 *
 * Returns: zero on success, negative value if insufficient memory.
 **/
int
nih_io_write (NihIo      *io,
	      const char *str,
	      size_t      len)
{
	int ret;

	nih_assert (io != NULL);
	nih_assert (str != NULL);

	ret = nih_io_buffer_push (io->send_buf, str, len);

	/* If we have data to write, ensure we poll for POLLOUT */
	if (io->send_buf->len)
		io->watch->events |= POLLOUT;

	return ret;
}


/**
 * nih_io_get:
 * @parent: parent of new string,
 * @io: structure to read from,
 * @delim: character to read until.
 *
 * Reads from the receive buffer of @io until a character in @delim or
 * the %NULL terminator is found, and returns the string up to, but not
 * including, the delimiter as a newly allocated string.
 *
 * @delim may be the empty string if only the %NULL terminator is considered
 * a delimiter.
 *
 * The string and the delimiter are removed from the buffer.
 *
 * Returns: newly allocated string or %NULL if delimiter not found or
 * insufficient memory.
 **/
char *
nih_io_get (void       *parent,
	    NihIo      *io,
	    const char *delim)
{
	size_t i;

	nih_assert (io != NULL);
	nih_assert (delim != NULL);

	for (i = 0; i < io->recv_buf->len; i++) {
		/* Found end of string */
		if (strchr (delim, io->recv_buf->buf[i])
		    || (io->recv_buf->buf[i] == '\0')) {
			char *str;

			/* Remove the string, and then the delimiter */
			str = nih_io_buffer_pop (parent, io->recv_buf, i);
			nih_io_buffer_shrink (io->recv_buf, 1);
			return str;
		}
	}

	return NULL;
}

/**
 * nih_io_printf:
 * @io: structure to write to,
 * @format: printf format string.
 *
 * Writes data formatted according to the printf-style @format string to
 * the send buffer of @io, the data will not be sent immediately but
 * whenever possible.
 *
 * Returns: number of bytes written, negative value if insufficient memory.
 **/
ssize_t
nih_io_printf (NihIo      *io,
	       const char *format,
	       ...)
{
	char    *str;
	va_list  args;
	ssize_t  len;

	nih_assert (io != NULL);
	nih_assert (format != NULL);

	va_start (args, format);
	str = nih_vsprintf (NULL, format, args);
	va_end (args);

	if (! str)
		return -1;

	len = nih_io_write (io, str, strlen (str));
	nih_free (str);

	return len;
}


/**
 * nih_io_set_nonblock:
 * @fd: file descriptor to change.
 *
 * Change the flags of @fd so that all operations become non-blocking.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_io_set_nonblock (int fd)
{
	int flags;

	nih_assert (fd >= 0);

	flags = fcntl (fd, F_GETFL);
	if (flags < 0)
		nih_return_system_error (-1);

	flags |= O_NONBLOCK;

	if (fcntl (fd, F_SETFL, flags) < 0)
		nih_return_system_error (-1);

	return 0;
}

/**
 * nih_io_set_cloexec:
 * @fd: file descriptor to change.
 *
 * Change the flags of @fd so that the file descriptor is closed on exec().
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_io_set_cloexec (int fd)
{
	int flags;

	nih_assert (fd >= 0);

	flags = fcntl (fd, F_GETFD);
	if (flags < 0)
		nih_return_system_error (-1);

	flags |= FD_CLOEXEC;

	if (fcntl (fd, F_SETFD, flags) < 0)
		nih_return_system_error (-1);

	return 0;
}
