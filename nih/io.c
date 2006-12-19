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
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/ip.h>

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
#include <nih/errors.h>

#include "io.h"


/* Prototypes for static functions */
static inline void nih_io_buffer_shrink  (NihIoBuffer *buffer, size_t len);
static void        nih_io_stream_watcher (NihIo *io, NihIoWatch *watch,
					  NihIoEvents events);
static void        nih_io_closed         (NihIo *io);
static void        nih_io_error          (NihIo *io);


/**
 * io_watches;
 *
 * This is the list of current watches on file descriptors and sockets,
 * not sorted into any particular order.  Each item is an NihIoWatch
 * structure.
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
		NIH_MUST (io_watches = nih_list_new (NULL));
}

/**
 * nih_io_add_watch:
 * @parent: parent of watch,
 * @fd: file descriptor or socket to watch,
 * @events: events to watch for,
 * @watcher: function to call when @events occur on @fd,
 * @data: pointer to pass to @watcher.
 *
 * Adds @fd to the list of file descriptors and sockets to watch, when any
 * of @events occur @watcher will be called.  @events is a bit mask
 * of the different events we care about.
 *
 * This is the simplest form of watch and satisfies most purposes.
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
 * Returns: the watch structure, or NULL if insufficient memory.
 **/
NihIoWatch *
nih_io_add_watch (const void   *parent,
		  int           fd,
		  NihIoEvents   events,
		  NihIoWatcher  watcher,
		  void         *data)
{
	NihIoWatch *watch;

	nih_assert (fd >= 0);
	nih_assert (watcher != NULL);

	nih_io_init ();

	watch = nih_new (parent, NihIoWatch);
	if (! watch)
		return NULL;

	nih_list_init (&watch->entry);
	nih_alloc_set_destructor (watch, (NihDestructor)nih_list_destructor);

	watch->fd = fd;
	watch->events = events;

	watch->watcher = watcher;
	watch->data = data;

	nih_list_add (io_watches, &watch->entry);

	return watch;
}


/**
 * nih_io_select_fds:
 * @nfds: pointer to store highest number in,
 * @readfds: pointer to set of descriptors to check for read,
 * @writefds: pointer to set of descriptors to check for write,
 * @exceptfds: pointer to set of descriptors to check for exceptions.
 *
 * Fills the given fd_set arrays based on the list of I/O watches.
 **/
void
nih_io_select_fds (int    *nfds,
		   fd_set *readfds,
		   fd_set *writefds,
		   fd_set *exceptfds)
{
	nih_assert (nfds != NULL);
	nih_assert (readfds != NULL);
	nih_assert (writefds != NULL);
	nih_assert (exceptfds != NULL);

	nih_io_init ();

	NIH_LIST_FOREACH (io_watches, iter) {
		NihIoWatch    *watch = (NihIoWatch *)iter;

		if (watch->events & NIH_IO_READ) {
			FD_SET (watch->fd, readfds);
			*nfds = MAX (*nfds, watch->fd + 1);
		}

		if (watch->events & NIH_IO_WRITE) {
			FD_SET (watch->fd, writefds);
			*nfds = MAX (*nfds, watch->fd + 1);
		}

		if (watch->events & NIH_IO_EXCEPT) {
			FD_SET (watch->fd, exceptfds);
			*nfds = MAX (*nfds, watch->fd + 1);
		}
	}
}

/**
 * nih_io_handle_fds:
 * @readfds: pointer to set of descriptors ready for read,
 * @writefds: pointer to set of descriptors ready for write,
 * @exceptfds: pointer to set of descriptors with exceptions.
 *
 * Receives arrays of fd_set structures which have been cleared of any
 * descriptors which haven't changed and iterates the watch list calling
 * the appropriate functions.
 *
 * It is safe for watches to remove the watch during their call.
 **/
void
nih_io_handle_fds (fd_set *readfds,
		   fd_set *writefds,
		   fd_set *exceptfds)
{
	nih_assert (readfds != NULL);
	nih_assert (writefds != NULL);
	nih_assert (exceptfds != NULL);

	nih_io_init ();

	NIH_LIST_FOREACH_SAFE (io_watches, iter) {
		NihIoWatch  *watch = (NihIoWatch *)iter;
		NihIoEvents  events;

		events = NIH_IO_NONE;

		if ((watch->events & NIH_IO_READ)
		    && FD_ISSET (watch->fd, readfds))
			events |= NIH_IO_READ;

		if ((watch->events & NIH_IO_WRITE)
		    && FD_ISSET (watch->fd, writefds))
			events |= NIH_IO_WRITE;

		if ((watch->events & NIH_IO_EXCEPT)
		    && FD_ISSET (watch->fd, exceptfds))
			events |= NIH_IO_EXCEPT;

		if (events)
			watch->watcher (watch->data, watch, events);
	}
}


/**
 * nih_io_buffer_new:
 * @parent: parent of new buffer.
 *
 * Allocates a new NihIoBuffer structure containing an empty buffer,
 * all functions that use the buffer ensure that the internal data is
 * an nih_alloc() child of the buffer, so this may be freed using
 * nih_free().
 *
 * The buffer is allocated using nih_alloc(), it can be freed using
 * nih_free().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: new buffer, or NULL if insufficient memory.
 **/
NihIoBuffer *
nih_io_buffer_new (const void *parent)
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
 * Returns: zero on success, NULL if insufficient memory.
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
 * necessary, and returns them in a new string allocated with nih_alloc().
 *
 * The returned string is always NULL terminated, even if there was
 * not a NULL in the buffer.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * It is illegal to request more bytes than are available in the buffer.
 *
 * Returns: newly allocated data pointer, or NULL if insufficient memory.
 **/
char *
nih_io_buffer_pop (const void  *parent,
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
 * Returns: zero on success, NULL if insufficient memory.
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
 * nih_io_message_new:
 * @parent: parent of new message.
 *
 * Allocates a new empty NihIoMessage structure.  All functions that use
 * the message structure ensure that the internal data is an nih_alloc()
 * child of the message, so this may be freed using nih_list_free().
 *
 * The message structure is allocated using nih_alloc() and normally
 * stored in a linked list, a default destructor is set that removes the
 * message from the list.  Removal of the message can be performed by
 * freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: new message, or NULL if insufficient memory.
 **/
NihIoMessage *
nih_io_message_new (const void *parent)
{
	NihIoMessage *message;

	message = nih_new (parent, NihIoMessage);
	if (! message)
		return NULL;

	nih_list_init (&message->entry);
	nih_alloc_set_destructor (message, (NihDestructor)nih_list_destructor);

	message->addr = NULL;
	message->addrlen = 0;

	message->msg_buf = nih_io_buffer_new (message);
	message->ctrl_buf = nih_io_buffer_new (message);

	return message;
}


/**
 * nih_io_message_recv:
 * @parent: parent of new message,
 * @fd: file descriptor to read from,
 * @len: maximum message size.
 *
 * Allocates a new NihIoMessage structure and fills it with a message
 * received on the @fd given, which has an upper limit of @len bytes.
 * The child buffers are allocated with nih_alloc as children of the
 * message itself, so are freed when the message is.
 *
 * If the message received is larger than @len bytes, the
 * NIH_TRUNCATED_MESSAGE error will be raised; likewise if there is more
 * control information than expected, NIH_TRUNCATED_CONTROL will be raised.
 *
 * The message structure is allocated using nih_alloc() and normally
 * stored in a linked list, a default destructor is set that removes the
 * message from the list.  Removal of the message can be performed by
 * freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: new message, or NULL on raised error.
 */
NihIoMessage *
nih_io_message_recv  (const void *parent,
		      int         fd,
		      size_t      len)
{
	NihIoMessage  *message;
	struct msghdr  msghdr;
	struct iovec   iov[1];
	ssize_t        recv_len;

	nih_assert (fd >= 0);

	message = nih_io_message_new (parent);
	if (! message)
		nih_return_system_error (NULL);

	/* Reserve enough space to hold the name based on the socket type */
	switch (nih_io_get_family (fd)) {
	case PF_UNIX:
		message->addrlen = sizeof (struct sockaddr_un);
		break;
	case PF_INET:
		message->addrlen = sizeof (struct sockaddr_in);
		break;
	case PF_INET6:
		message->addrlen = sizeof (struct sockaddr_in6);
		break;
	default:
		message->addrlen = 0;
	}

	if (message->addrlen) {
		message->addr = nih_alloc (message, message->addrlen);
		if (! message->addr)
			goto error;

		msghdr.msg_name = message->addr;
		msghdr.msg_namelen = message->addrlen;
	} else {
		msghdr.msg_name = NULL;
		msghdr.msg_namelen = 0;
	}

	/* Allocate the message buffer and resize it so it'll fit at least
	 * the number of bytes expected, receive the data directly into it.
	 */
	message->msg_buf = nih_io_buffer_new (message);
	if (! message->msg_buf)
		goto error;

	if (nih_io_buffer_resize (message->msg_buf, len) < 0)
		goto error;

	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 1;
	iov[0].iov_base = message->msg_buf->buf;
	iov[0].iov_len = message->msg_buf->size;

	/* Allocate the control buffer with ample space to receive any
	 * control information that we might get, and receive the data
	 * directly into it as well.
	 */
	message->ctrl_buf = nih_io_buffer_new (message);
	if (! message->ctrl_buf)
		goto error;

	if (nih_io_buffer_resize (message->ctrl_buf, BUFSIZ) < 0)
		goto error;

	msghdr.msg_control = message->ctrl_buf->buf;
	msghdr.msg_controllen = message->ctrl_buf->size;

	msghdr.msg_flags = 0;

	/* Receive the message, update the length of the message and control
	 * buffers and check for truncated messages */
	recv_len = recvmsg (fd, &msghdr, 0);
	if (recv_len < 0)
		goto error;

	/* Copy the lengths back out of the msghdr structure into the buffers
	 * so they're right.
	 */
	message->msg_buf->len = recv_len;
	message->ctrl_buf->len = msghdr.msg_controllen;
	message->addrlen = msghdr.msg_namelen;

	if ((msghdr.msg_flags & MSG_TRUNC)
	    || (msghdr.msg_flags & MSG_CTRUNC)) {
		nih_error_raise (NIH_TRUNCATED_MESSAGE,
				 _(NIH_TRUNCATED_MESSAGE_STR));
		nih_free (message);
		return NULL;
	}

	return message;

error:
	nih_error_raise_system ();
	nih_free (message);
	return NULL;
}

/**
 * nih_io_message_send:
 * @message: message to be sent,
 * @fd: file descriptor to send to.
 *
 * Send a @message as a single message to the file descriptor or socket @fd.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_io_message_send (NihIoMessage *message,
		     int           fd)
{
	struct msghdr msghdr;
	struct iovec  iov[1];

	nih_assert (message != NULL);
	nih_assert (fd >= 0);

	msghdr.msg_name = message->addr;
	msghdr.msg_namelen = message->addrlen;

	if (message->msg_buf) {
		msghdr.msg_iov = iov;
		msghdr.msg_iovlen = 1;
		iov[0].iov_base = message->msg_buf->buf;
		iov[0].iov_len = message->msg_buf->len;
	} else {
		msghdr.msg_iov = NULL;
		msghdr.msg_iovlen = 0;
	}

	if (message->ctrl_buf) {
		msghdr.msg_control = message->ctrl_buf->buf;
		msghdr.msg_controllen = message->ctrl_buf->len;
	} else {
		msghdr.msg_control = NULL;
		msghdr.msg_controllen = 0;
	}

	msghdr.msg_flags = 0;

	if (sendmsg (fd, &msghdr, 0) < 0)
		nih_return_system_error (-1);

	return 0;
}


/**
 * nih_io_reopen:
 * @parent: parent pointer of new structure,
 * @fd: file descriptor to manage,
 * @reader: function to call when new data available,
 * @close_handler: function to call on close,
 * @error_handler: function to call on error,
 * @data: data to pass to functions.
 *
 * This allocates a new NihIo structure using nih_alloc, used to manage an
 * already opened file descriptor.  The descriptor is set to be non-blocking
 * if it hasn't already been and the SIGPIPE signal is set to be ignored.
 *
 * If @reader is given then all data is automatically read from the
 * file descriptor and stored in a buffer and this function is called
 * whenever there is new data available.  The function is under no
 * obligation to remove any data, it's perfectly valid to leave it in the
 * buffer until next time.
 *
 * If @close_handler is given then it is called whenever the remote end of the
 * file descriptor is closed, otherwise the local end is closed and the
 * entire structure freed (which may be surprising to you).
 *
 * If @error_handler is given then any it is called whenever any errors are
 * raised, otherwise the @close_handler is called or the same action taken
 * if that is not given either.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated structure, or NULL if insufficient memory.
 **/
NihIo *
nih_io_reopen (const void        *parent,
	       int                fd,
	       NihIoReader        reader,
	       NihIoCloseHandler  close_handler,
	       NihIoErrorHandler  error_handler,
	       void              *data)
{
	NihIo *io;

	nih_assert (fd >= 0);

	io = nih_new (parent, NihIo);
	if (! io)
		return NULL;

	io->type = NIH_IO_STREAM;
	io->reader = reader;
	io->close_handler = close_handler;
	io->error_handler = error_handler;
	io->data = data;
	io->shutdown = FALSE;

	io->watch = nih_io_add_watch (io, fd, NIH_IO_READ,
				      (NihIoWatcher) nih_io_stream_watcher,
				      io);
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
	if (nih_io_set_nonblock (fd) < 0) {
		NihError *err;

		err = nih_error_get ();
		nih_error ("%s: %s",
			   _("Unable to set descriptor non-blocking"),
			   err->message);
		nih_free (err);
	}

	return io;
error:
	nih_free (io);
	return NULL;
}


/**
 * nih_io_stream_watcher:
 * @io: NihIo structure,
 * @watch: NihIoWatch for which an event occurred,
 * @events: events that occurred.
 *
 * This is the watcher function associated with all file descriptors
 * in the stream mode being managed by NihIo.  It ensures that data is
 * read from the file descriptor into the recv buffer and the reader
 * called, any data in the send buffer is written to the socket
 * and any errors are handled
 **/
static void
nih_io_stream_watcher (NihIo       *io,
		       NihIoWatch  *watch,
		       NihIoEvents  events)
{
	nih_assert (io != NULL);
	nih_assert (io->type == NIH_IO_STREAM);
	nih_assert (watch != NULL);

	/* There's data to be read */
	if (events & NIH_IO_READ) {
		ssize_t len;

		/* Read directly into the buffer to save hauling temporary
		 * blocks around; always make sure there's room for at
		 * least 80 bytes (random minimum read).  Make sure we call
		 * read as many times as necessary to exhaust the socket
		 * so we can get maximum throughput.
		 */
		do {
			if (nih_io_buffer_resize (io->recv_buf, 80) < 0)
				return;

			len = read (watch->fd,
				    io->recv_buf->buf + io->recv_buf->len,
				    io->recv_buf->size - io->recv_buf->len);
			if (len < 0) {
				nih_error_raise_system ();
			} else if (len > 0) {
				io->recv_buf->len += len;
			}
		} while (len > 0);

		/* Call the reader if we have any data in the buffer.
		 * This could be called simply because we're about to error,
		 * but it means we give it one last chance to process.
		 */
		nih_error_push_context();
		if (io->recv_buf->len) {
			if (io->reader) {
				io->reader (io->data, io, io->recv_buf->buf,
					     io->recv_buf->len);
			} else {
				/* No reader, just discard */
				nih_io_buffer_shrink (io->recv_buf,
						      io->recv_buf->len);
			}
		}
		nih_error_pop_context();

		/* Deal with errors */
		if (len < 0) {
			NihError *err;

			err = nih_error_get ();
			if ((err->number != EAGAIN)
			    && (err->number != EINTR)) {
				nih_error_raise_again (err);
				nih_io_error (io);
				return;
			} else {
				nih_free (err);
			}
		}

		/* Deal with socket being closed */
		if (! len) {
			nih_io_closed (io);
			return;
		}
	}

	/* There's room to write data, send as much as we can */
	if (events & NIH_IO_WRITE) {
		ssize_t len;

		/* Write directly from the buffer to save hauling temporary
		 * blocks around, and call write() as many times as necessary
		 * to exhaust the buffer so we can get maximum throughput.
		 */
		while (io->send_buf->len) {
			len = write (watch->fd, io->send_buf->buf,
				     io->send_buf->len);

			/* Don't bother checking errors, we catch them
			 * using read
			 */
			if (len <= 0)
				break;

			nih_io_buffer_shrink (io->send_buf, len);
		}

		/* Don't check for writability if we have nothing to write */
		if (! io->send_buf->len)
			watch->events &= ~NIH_IO_WRITE;

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
 * file descriptor being managed by NihIo.
 *
 * Normally this just calls the error handler, or if not available, it
 * behaves as if the remote end was closed.
 **/
static void
nih_io_error (NihIo *io)
{
	nih_assert (io != NULL);

	if (io->error_handler) {
		io->error_handler (io->data, io);
	} else {
		NihError *err;

		err = nih_error_get ();
		nih_error ("%s: %s", _("Error while reading from descriptor"),
			   err->message);
		nih_free (err);

		nih_io_closed (io);
	}
}

/**
 * nih_io_closed:
 * @io: structure to be closed.
 *
 * This function is called when the local end of a file descriptor being
 * managed by NihIo should be closed.  Usually this is because the remote
 * end has been closed (without error) but it can also be because no
 * error handler was given
 *
 * Normally this just calls the close handler, or if not available, it
 * closes the file descriptor and frees the structure (which may be
 * surprising if you were hanging on to a pointer of it).
 *
 * Note that this can result in the error handler getting called if
 * there's an error caught by closing the socket.
 **/
static void
nih_io_closed (NihIo *io)
{
	nih_assert (io != NULL);

	if (io->close_handler) {
		io->close_handler (io->data, io);
	} else {
		nih_io_close (io);
	}
}

/**
 * nih_io_shutdown:
 * @io: structure to be closed.
 *
 * Marks the NihIo structure to be closed once the buffers or queue have
 * been emptied, rather than immediately.  Closure is performed by calling
 * the close handler if given or nih_io_close().
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
 * Closes the file descriptor associated with an NihIo structure and
 * frees the structure.  If an error is caught by closing the descriptor,
 * it is the error handler is called instead of the error being raised;
 * this allows you to group your error handling in one place rather than
 * special-case close.
 **/
void
nih_io_close (NihIo *io)
{
	nih_assert (io != NULL);

	if ((close (io->watch->fd) < 0) && io->error_handler) {
		nih_error_raise_system ();
		io->error_handler (io->data, io);
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
 * in a new string allocated with nih_alloc() that is always NULL terminated
 * even if there was not a NULL in the buffer.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * It is illegal to request more bytes than exist in the bufferr.
 *
 * Returns: newly allocated string, or NULL if insufficient memory.
 **/
char *
nih_io_read (const void *parent,
	     NihIo      *io,
	     size_t      len)
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
 * Care should be taken to ensure @len does not include the NULL
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

	/* If we have data to write, ensure we watch for writability */
	if (io->send_buf->len)
		io->watch->events |= NIH_IO_WRITE;

	return ret;
}


/**
 * nih_io_get:
 * @parent: parent of new string,
 * @io: structure to read from,
 * @delim: character to read until.
 *
 * Reads from the receive buffer of @io until a character in @delim or
 * the NULL terminator is found, and returns a new string allocated with
 * nih_alloc() that contains a copy of the buffer up to, but not including,
 * the delimiter.
 *
 * @delim may be the empty string if only the NULL terminator is considered
 * a delimiter.
 *
 * The string and the delimiter are removed from the buffer.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: newly allocated string or NULL if delimiter not found or
 * insufficient memory.
 **/
char *
nih_io_get (const void *parent,
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

/**
 * nih_io_get_family:
 * @fd: file descriptor to check.
 *
 * Queries the socket so that the family it belongs to (PF_UNIX, PF_INET,
 * PF_INET6) can be found.
 *
 * Returns: the family of the socket, or -1 on error.
 **/
ssize_t
nih_io_get_family (int fd)
{
	socklen_t socklen;
	union {
		sa_family_t         family;
		struct sockaddr_un  un;
		struct sockaddr_in  in;
		struct sockaddr_in6 in6;
	} addr;

	nih_assert (fd >= 0);

	socklen = sizeof (addr);
	if (getsockname (fd, (struct sockaddr *)&addr, &socklen) < 0)
		return -1;

	return addr.family;
}
