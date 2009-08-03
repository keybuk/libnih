/* libnih
 *
 * io.c - file and socket input/output handling
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
static void           nih_io_watcher        (NihIo *io, NihIoWatch *watch,
					     NihIoEvents events);
static inline ssize_t nih_io_watcher_read   (NihIo *io, NihIoWatch *watch)
	__attribute__ ((warn_unused_result));
static inline ssize_t nih_io_watcher_write  (NihIo *io, NihIoWatch *watch)
	__attribute__ ((warn_unused_result));
static void           nih_io_closed         (NihIo *io);
static void           nih_io_error          (NihIo *io);
static void           nih_io_shutdown_check (NihIo *io);
static NihIoMessage * nih_io_first_message  (NihIo *io);


/**
 * nih_io_watches;
 *
 * This is the list of current watches on file descriptors and sockets,
 * not sorted into any particular order.  Each item is an NihIoWatch
 * structure.
 **/
NihList *nih_io_watches = NULL;


/**
 * nih_io_init:
 *
 * Initialise the list of I/O watches.
 **/
void
nih_io_init (void)
{
	if (! nih_io_watches)
		nih_io_watches = NIH_MUST (nih_list_new (NULL));
}

/**
 * nih_io_add_watch:
 * @parent: parent object for new watch,
 * @fd: file descriptor or socket to watch,
 * @events: events to watch for,
 * @watcher: function to call when @events occur on @fd,
 * @data: pointer to pass to @watcher.
 *
 * Adds @fd to the list of file descriptors and sockets to watch, when any
 * of @events occur @watcher will be called.  @events is a bit mask
 * of the different events we care about.
 *
 * This is the simplest form of watch and satisfies most basic purposes.
 *
 * The watch structure is allocated using nih_alloc() and stored in a linked
 * list; there is no non-allocated version because of this.
 *
 * Removal of the watch can be performed by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned watch.  When all parents
 * of the returned watch are freed, the returned watch will also be
 * freed.
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

	nih_alloc_set_destructor (watch, nih_list_destroy);

	watch->fd = fd;
	watch->events = events;

	watch->watcher = watcher;
	watch->data = data;

	nih_list_add (nih_io_watches, &watch->entry);

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

	NIH_LIST_FOREACH (nih_io_watches, iter) {
		NihIoWatch    *watch = (NihIoWatch *)iter;

		if (watch->events & NIH_IO_READ) {
			FD_SET (watch->fd, readfds);
			*nfds = nih_max (*nfds, watch->fd + 1);
		}

		if (watch->events & NIH_IO_WRITE) {
			FD_SET (watch->fd, writefds);
			*nfds = nih_max (*nfds, watch->fd + 1);
		}

		if (watch->events & NIH_IO_EXCEPT) {
			FD_SET (watch->fd, exceptfds);
			*nfds = nih_max (*nfds, watch->fd + 1);
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

	NIH_LIST_FOREACH_SAFE (nih_io_watches, iter) {
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
 * @parent: parent object for new buffer.
 *
 * Allocates a new NihIoBuffer structure containing an empty buffer.
 *
 * The buffer is allocated using nih_alloc() and all functions that use the
 * buffer ensure that the internal data is an nih_alloc() child of the buffer
 * itself, so this can be freed using nih_free(); there is no non-allocated
 * version because of this,
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned buffer.  When all parents
 * of the returned buffer are freed, the returned buffer will also be
 * freed.
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
 * Returns: zero on success, negative value if insufficient memory.
 **/
int
nih_io_buffer_resize (NihIoBuffer *buffer,
		      size_t       grow)
{
	char   *new_buf;
	size_t  new_len, new_size;

	nih_assert (buffer != NULL);

	new_len = buffer->len + grow;
	if (! new_len) {
		/* No bytes to store, so clean up the buffer */
		if (buffer->buf)
			nih_unref (buffer->buf, buffer);

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

	/* Clear the area between the old and new size; this is because we
	 * tend to pass these buffers to syscalls, and passing around
	 * unintialised data upsets people.
	 */
	if (new_size > buffer->size)
		memset (new_buf + buffer->size, '\0', new_size - buffer->size);

	/* Note: don't adjust the length */
	buffer->buf = new_buf;
	buffer->size = new_size;

	return 0;
}

/**
 * nih_io_buffer_pop:
 * @parent: parent object for new object,
 * @buffer: buffer to shrink,
 * @len: bytes to take.
 *
 * Takes @len bytes from the start of @buffer, reducing the size if
 * necessary, and returns them in a new string allocated with nih_alloc().
 * @len is updated to contain the actual number of bytes returned.
 *
 * The returned string is always NULL terminated, even if there was
 * not a NULL in the buffer.
 *
 * If there are not @len bytes in the buffer, the maximum amount there is
 * will be returned, if there is nothing you'll get a zero-length string.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned object.  When all parents
 * of the returned watch are freed, the returned object will also be
 * object.
 *
 * Returns: newly allocated data pointer, or NULL if insufficient memory.
 **/
char *
nih_io_buffer_pop (const void  *parent,
		   NihIoBuffer *buffer,
		   size_t      *len)
{
	char *str;

	nih_assert (buffer != NULL);
	nih_assert (len != NULL);

	*len = nih_min (*len, buffer->len);

	str = nih_alloc (parent, *len + 1);
	if (! str)
		return NULL;

	/* Copy the data into the new string and add NULL */
	memcpy (str, buffer->buf, *len);
	str[*len] = '\0';

	/* Move the buffer up */
	nih_io_buffer_shrink (buffer, *len);

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
void
nih_io_buffer_shrink (NihIoBuffer *buffer,
		      size_t       len)
{
	nih_assert (buffer != NULL);

	len = nih_min (len, buffer->len);

	memmove (buffer->buf, buffer->buf + len, buffer->len - len);
	buffer->len -= len;

	/* Don't worry if this fails, it just means the buffer is larger
	 * than it needs to be.
	 */
	nih_io_buffer_resize (buffer, 0);
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
 * Returns: zero on success, negative value if insufficient memory.
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
 * @parent: parent object for new message.
 *
 * Allocates a new NihIoMessage structure with empty buffers.
 *
 * All functions that use the message structure ensure that the internal
 * data is an nih_alloc() child of the message or its buffers, so the entire
 * message freed using nih_free(); there is no non-allocated version because
 * of this.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned message.  When all parents
 * of the returned message are freed, the returned message will also be
 * freed.
 *
 * @parent should not be the NihIo object this is to be sent with, that
 * will take a reference when you send the message.
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

	nih_alloc_set_destructor (message, nih_list_destroy);

	message->addr = NULL;
	message->addrlen = 0;

	message->data = nih_io_buffer_new (message);
	if (! message->data)
		goto error;

	message->control = nih_new (message, struct cmsghdr *);
	if (! message->control)
		goto error;

	message->control[0] = NULL;

	return message;

error:
	nih_free (message);
	return NULL;
}

/**
 * nih_io_message_add_control:
 * @message: message to add control message to,
 * @level: level of control message,
 * @type: protocol-specific type,
 * @len: length of control data,
 * @data: control data.
 *
 * Adds a new control message with the @level and @type given to @message.
 * The control data is copied from @data into the message and should be @len
 * bytes long.
 *
 * Returns: zero on success, negative value if insufficient memory.
 **/
int
nih_io_message_add_control (NihIoMessage *message,
			    int           level,
			    int           type,
			    socklen_t     len,
			    const void   *data)
{
	struct cmsghdr *cmsg, **ptr;
	size_t          cmsglen = 0;

	nih_assert (message != NULL);
	nih_assert (data != NULL);

	/* Allocate the control message first */
	cmsg = nih_alloc (message->control, CMSG_SPACE (len));
	if (! cmsg)
		return -1;

	/* Then increase the size of the array, if this fails then we can
	 * still leave things in a consistent state.
	 */
	for (ptr = message->control; *ptr; ptr++)
		cmsglen++;

	ptr = nih_realloc (message->control, message,
			   sizeof (struct cmsghdr *) * (cmsglen + 2));
	if (! ptr) {
		nih_free (cmsg);
		return -1;
	}

	message->control = ptr;
	message->control[cmsglen++] = cmsg;
	message->control[cmsglen] = NULL;

	cmsg->cmsg_level = level;
	cmsg->cmsg_type = type;
	cmsg->cmsg_len = CMSG_LEN (len);

	memcpy (CMSG_DATA (cmsg), data, len);

	return 0;
}


/**
 * nih_io_message_recv:
 * @parent: parent object for new message,
 * @fd: file descriptor to read from,
 * @len: number of bytes read.
 *
 * Allocates a new NihIoMessage structure and fills it with a message
 * received on @fd with recvmsg().  The buffer is increased in size until
 * all of the message can fit within it.  @len is set to contain the actual
 * number of bytes read.
 *
 * The message structure is allocated using nih_alloc() and normally
 * stored in a linked list.  Removal of the message can be performed by
 * freeing it.
 *
 * All functions that use the message structure ensure that the internal
 * data is an nih_alloc() child of the message or its buffers, so the entire
 * message freed using nih_free().
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned message.  When all parents
 * of the returned message are freed, the returned message will also be
 * freed.
 *
 * Returns: new message, or NULL on raised error.
 */
NihIoMessage *
nih_io_message_recv  (const void *parent,
		      int         fd,
		      size_t     *len)
{
	NihIoMessage          *message;
	nih_local NihIoBuffer *ctrl_buf = NULL;
	struct msghdr          msghdr;
	struct iovec           iov[1];
	struct cmsghdr        *cmsg;
	ssize_t                recv_len;

	nih_assert (fd >= 0);
	nih_assert (len != NULL);

	message = nih_io_message_new (parent);
	if (! message)
		goto error;

	/* Allocate a buffer to store received control messages in */
	ctrl_buf = nih_io_buffer_new (NULL);
	if (! ctrl_buf)
		goto error;

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

	/* Set the flags to indicate a truncated message so that we allocate
	 * some initial buffer space.
	 */
	msghdr.msg_flags = MSG_TRUNC | MSG_CTRUNC;

	do {
		/* Increase the size of the message buffer */
		if ((msghdr.msg_flags & MSG_TRUNC)
		    && (nih_io_buffer_resize (message->data,
					      (message->data->size
					       + BUFSIZ)) < 0))
			goto error;

		msghdr.msg_iov = iov;
		msghdr.msg_iovlen = 1;
		iov[0].iov_base = message->data->buf;
		iov[0].iov_len = message->data->size;

		/* Increase the size of the control buffer */
		if ((msghdr.msg_flags & MSG_CTRUNC)
		    && (nih_io_buffer_resize (ctrl_buf, (ctrl_buf->size
							 + BUFSIZ)) < 0))
			goto error;

		msghdr.msg_control = ctrl_buf->buf;
		msghdr.msg_controllen = ctrl_buf->size;

		msghdr.msg_flags = 0;

		/* Peek at the message to determine whether or not it has
		 * been truncated.
		 */
		recv_len = recvmsg (fd, &msghdr, MSG_PEEK);
		if (recv_len < 0)
			goto error;
	} while ((msghdr.msg_flags & MSG_TRUNC)
		 || (msghdr.msg_flags & MSG_CTRUNC));

	/* Receive properly this time */
	recv_len = recvmsg (fd, &msghdr, 0);
	if (recv_len < 0)
		goto error;

	/* Update the lengths, both to the caller and of the message structure
	 * buffers based on what was actually received.
	 */
	*len = recv_len;
	message->data->len = recv_len;
	message->addrlen = msghdr.msg_namelen;

	/* Copy control messages out of the buffer and add them to the
	 * message.
	 */
	for (cmsg = CMSG_FIRSTHDR (&msghdr); cmsg;
	     cmsg = CMSG_NXTHDR (&msghdr, cmsg)) {
		size_t len;

		len = cmsg->cmsg_len - CMSG_ALIGN (sizeof (struct cmsghdr));
		NIH_ZERO (nih_io_message_add_control (message,
						      cmsg->cmsg_level,
						      cmsg->cmsg_type,
						      len,
						      CMSG_DATA (cmsg)));
	}

	return message;

error:
	nih_error_raise_system ();
	if (message)
		nih_free (message);
	return NULL;
}

/**
 * nih_io_message_send:
 * @message: message to be sent,
 * @fd: file descriptor to send to.
 *
 * Send @message, an already allocated and filled NihIoMessage structure
 * to the file descriptor or socket @fd using sendmsg().
 *
 * If @fd is not connected, the destination for the message can be specified
 * in the addr and addrlen members of the structure.  The message data itself
 * should be pushed into the data member, and any control data added to the
 * control member (usually using nih_io_message_add_control()).
 *
 * Returns: length of message sent, negative value on raised error.
 **/
ssize_t
nih_io_message_send (NihIoMessage *message,
		     int           fd)
{
	nih_local NihIoBuffer  *ctrl_buf = NULL;
	struct msghdr           msghdr;
	struct iovec            iov[1];
	struct cmsghdr        **ptr;
	ssize_t                 len;

	nih_assert (message != NULL);
	nih_assert (fd >= 0);

	msghdr.msg_name = message->addr;
	msghdr.msg_namelen = message->addrlen;

	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 1;
	iov[0].iov_base = message->data->buf;
	iov[0].iov_len = message->data->len;

	/* Allocate a buffer in which we store the control messages that we
	 * need to send.
	 */
	ctrl_buf = nih_io_buffer_new (NULL);
	if (! ctrl_buf)
		nih_return_system_error (-1);

	for (ptr = message->control; *ptr; ptr++) {
		size_t len;

		len = CMSG_SPACE ((*ptr)->cmsg_len
				  - CMSG_ALIGN (sizeof (struct cmsghdr)));
		if (nih_io_buffer_resize (ctrl_buf, len) < 0)
			nih_return_system_error (-1);

		memcpy (ctrl_buf->buf + ctrl_buf->len, *ptr, (*ptr)->cmsg_len);
		ctrl_buf->len += len;
	}

	msghdr.msg_control = ctrl_buf->buf;
	msghdr.msg_controllen = ctrl_buf->len;

	msghdr.msg_flags = 0;

	len = sendmsg (fd, &msghdr, 0);
	if (len < 0)
		nih_return_system_error (-1);

	return len;
}


/**
 * nih_io_reopen:
 * @parent: parent object for new structure,
 * @fd: file descriptor to manage,
 * @type: handling mode,
 * @reader: function to call when new data available,
 * @close_handler: function to call on close,
 * @error_handler: function to call on error,
 * @data: data to pass to functions.
 *
 * This allocates a new NihIo structure using nih_alloc(), used to manage an
 * already opened file descriptor.  The descriptor is set to be non-blocking
 * if it hasn't already been and the SIGPIPE signal is set to be ignored.
 * The file descriptor will be closed when the structure is freed.
 *
 * If @type is NIH_IO_STREAM, the descriptor is managed in stream mode; data
 * to be sent and data received are held in a single buffer that is expanded
 * and shrunk as required.  If @type is NIH_IO_MESSAGE, the descriptor is
 * managed in message mode; individual messages are queued to be sent and
 * are received into a queue as discreet messages.
 *
 * Data is automatically read from the file descriptor whenever it is
 * available, and stored in the receive buffer or queue.  If @reader is
 * given, this function is called whenever new data has been received.
 * In stream mode, this function is under no obligation to remove any data,
 * it's perfectly valid to leave it in the buffer until more data is added.
 * In message mode, the message must be processed as even if more messages
 * are read, the reader will only be called with the first one in the queue.
 *
 * If @close_handler is given then it is called whenever the remote end of the
 * file descriptor is closed, otherwise the local end is closed and the
 * entire structure freed (which may be surprising to you).
 *
 * If @error_handler is given then it is called whenever any errors are
 * raised, otherwise the @close_handler is called or the same action taken
 * if that is not given either.
 *
 * The returned structure is allocated with nih_alloc() and children buffers
 * and watches are allocated as children so will be automatically freed;
 * there is no non-allocated version because of this.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned structure.  When all parents
 * of the returned structure are freed, the returned structure will also be
 * freed.
 *
 * Returns: newly allocated structure, or NULL on raised error.
 **/
NihIo *
nih_io_reopen (const void        *parent,
	       int                fd,
	       NihIoType          type,
	       NihIoReader        reader,
	       NihIoCloseHandler  close_handler,
	       NihIoErrorHandler  error_handler,
	       void              *data)
{
	NihIo *io;

	nih_assert (fd >= 0);

	io = nih_new (parent, NihIo);
	if (! io)
		nih_return_system_error (NULL);

	io->type = type;
	io->reader = reader;
	io->close_handler = close_handler;
	io->error_handler = error_handler;
	io->data = data;
	io->shutdown = FALSE;
	io->free = NULL;

	switch (io->type) {
	case NIH_IO_STREAM:
		io->send_buf = nih_io_buffer_new (io);
		if (! io->send_buf)
			goto error;

		io->recv_buf = nih_io_buffer_new (io);
		if (! io->recv_buf)
			goto error;

		break;
	case NIH_IO_MESSAGE:
		io->send_q = nih_list_new (io);
		if (! io->send_q)
			goto error;

		io->recv_q = nih_list_new (io);
		if (! io->recv_q)
			goto error;

		break;
	default:
		nih_assert_not_reached ();
	}

	/* Watcher function is called at first only if we have data to read */
	io->watch = nih_io_add_watch (io, fd, NIH_IO_READ,
				      (NihIoWatcher)nih_io_watcher, io);
	if (! io->watch)
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
		goto error;

	nih_alloc_set_destructor (io, nih_io_destroy);

	return io;
error:
	nih_error_raise_system ();
	nih_free (io);
	return NULL;
}


/**
 * nih_io_watcher:
 * @io: NihIo structure,
 * @watch: NihIoWatch for which an event occurred,
 * @events: events that occurred.
 *
 * This is the watcher function associated with all file descriptors being
 * managed by NihIo.  It ensures that any data or messages available are
 * read and placed into the receive buffer or queue, and the reader function
 * is called if set.
 *
 * Any data or messaages in the send buffer or queue are written out if the
 * @events includes NIH_IO_WRITE.
 *
 * Errors are handled when data is read, and result in the error handled
 * function being called if set, otherwise the close handler is called if
 * set and the socket closed.
 **/
static void
nih_io_watcher (NihIo       *io,
		NihIoWatch  *watch,
		NihIoEvents  events)
{
	int caught_free;

	nih_assert (io != NULL);
	nih_assert (watch != NULL);

	caught_free = FALSE;
	if (! io->free)
		io->free = &caught_free;

	/* There's data to be read */
	if (events & NIH_IO_READ) {
		ssize_t len;

		len = nih_io_watcher_read (io, watch);

		/* We want to call the reader if we have any data in the
		 * buffer or messages in the receive queue, even if the
		 * remote end is closed or there was an error.  In the
		 * latter case, it means we give it once last chance to
		 * process the messages.
		 */
		if (io->reader) {
			nih_error_push_context();

			switch (io->type) {
			case NIH_IO_STREAM:
				if (io->recv_buf->len)
					io->reader (io->data, io,
						    io->recv_buf->buf,
						    io->recv_buf->len);

				break;
			case NIH_IO_MESSAGE: {
				NihIoMessage *last = NULL, *message;

				/* Call reader until the first message on the
				 * queue does not change.
				 */
				last = NULL;
				while (((message = nih_io_first_message (io))
					!= last) && message) {
					io->reader (io->data, io,
						    message->data->buf,
						    message->data->len);
					if (caught_free)
						break;
					last = message;
				}
				break;
			}
			default:
				nih_assert_not_reached();
			}

			nih_error_pop_context();
		}

		/* Deal with errors */
		if (len < 0) {
			NihError *err;

			err = nih_error_get ();
			switch (err->number) {
			case EAGAIN:
			case EINTR:
			case ENOMEM:
				nih_free (err);
				break;
			default:
				if (caught_free)
					return;

				nih_io_error (io);
				if (caught_free)
					return;
				goto finish;
			}
		}

		/* We might have been freed */
		if (caught_free)
			return;

		/* Deal with socket being closed */
		if ((io->type == NIH_IO_STREAM) && (! len)) {
			nih_io_closed (io);
			if (caught_free)
				return;
			goto finish;
		}
	}

	/* There's room to write data, send as much as we can */
	if (events & NIH_IO_WRITE) {
		ssize_t len;

		len = nih_io_watcher_write (io, watch);

		/* Deal with errors */
		if (len < 0) {
			NihError *err;

			err = nih_error_get ();
			switch (err->number) {
			case EAGAIN:
			case EINTR:
			case ENOMEM:
				nih_free (err);
				break;
			default:
				nih_io_error (io);
				if (caught_free)
					return;
				goto finish;
			}
		}
	}

finish:
	if (io->free == &caught_free)
		io->free = NULL;

	/* Shut down the socket if it is empty */
	nih_io_shutdown_check (io);
}

/**
 * nih_io_watcher_read:
 * @io: NihIo structure,
 * @watch: NihIoWatch for which an event occurred.
 *
 * Read data from the socket directly into the buffer or receive queue to
 * save hauling temporary blocks around.  This function will call read()
 * or recvmsg() as many times as possible to keep the kernel-side buffers
 * small.
 *
 * It returns once a call errors or returns zero to indicate that the
 * remote end closed.
 *
 * Returns: size of last read, zero if remote end closed and negative
 * value on raised error.
 **/
static inline ssize_t
nih_io_watcher_read (NihIo      *io,
		     NihIoWatch *watch)
{
	ssize_t len = 0;

	nih_assert (io != NULL);
	nih_assert (watch != NULL);

	for (;;) {
		NihIoMessage *message;

		switch (io->type) {
		case NIH_IO_STREAM:
			/* Make sure there's room for at least 80 bytes
			 * (random minimum read).
			 */
			if (nih_io_buffer_resize (io->recv_buf, 80) < 0)
				nih_return_system_error (-1);

			len = read (watch->fd,
				    io->recv_buf->buf + io->recv_buf->len,
				    io->recv_buf->size - io->recv_buf->len);
			if (len < 0) {
				nih_return_system_error (-1);
			} else if (len > 0) {
				io->recv_buf->len += len;
			} else {
				return 0;
			}

			break;
		case NIH_IO_MESSAGE:
			/* Use BUFSIZ as the maximum message size. */
			len = BUFSIZ;
			message = nih_io_message_recv (io, watch->fd,
						       (size_t *)&len);
			if (! message) {
				return -1;
			} else {
				nih_list_add (io->recv_q, &message->entry);
			}

			break;
		default:
			nih_assert_not_reached ();
		}
	}

	return len;
}

/**
 * nih_io_watcher_write:
 * @io: NihIo structure,
 * @watch: NihIoWatch for which an event occurred.
 *
 * Write data directly from the buffer or receive queue into the socket to
 * save hauling temporary blocks around.  This function will call write()
 * or sendmsg() as many times as possible to keep the buffer or queue
 * small.
 *
 * It returns once a call errors or returns zero to indicate that the
 * remote end closed.
 *
 * Returns: size of last write, zero if remote end closed and negative
 * value on raised error.
 **/
static inline ssize_t
nih_io_watcher_write (NihIo      *io,
		      NihIoWatch *watch)
{
	ssize_t len = 0;

	nih_assert (io != NULL);
	nih_assert (watch != NULL);

	switch (io->type) {
	case NIH_IO_STREAM:
		while (io->send_buf->len) {
			len = write (watch->fd, io->send_buf->buf,
				     io->send_buf->len);

			if (len < 0)
				nih_return_system_error (-1);

			nih_io_buffer_shrink (io->send_buf, len);
		}

		/* Don't check for writability if we have nothing to write */
		if (! io->send_buf->len)
			watch->events &= ~NIH_IO_WRITE;

		break;
	case NIH_IO_MESSAGE:
		while (! NIH_LIST_EMPTY (io->send_q)) {
			NihIoMessage *message;

			message = (NihIoMessage *)io->send_q->next;
			len = nih_io_message_send (message, watch->fd);

			if (len < 0)
				return -1;

			nih_unref (message, io);
		}

		/* Don't check for writability if we have nothing to write */
		if (NIH_LIST_EMPTY (io->send_q))
			watch->events &= ~NIH_IO_WRITE;

		break;
	default:
		nih_assert_not_reached ();
	}

	return len;
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
		nih_free (io);
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

	nih_io_shutdown_check (io);
}

/**
 * nih_io_shutdown_check:
 * @io: structure to be closed.
 *
 * Checks whether the NihIo structure is set to be shutdown, and has now
 * reached the point to be closed.  Call whenever you remove data from a
 * buffer.
 **/
static void
nih_io_shutdown_check (NihIo *io)
{
	nih_assert (io != NULL);

	if (! io->shutdown)
		return;

	switch (io->type) {
	case NIH_IO_STREAM:
		if ((! io->send_buf->len) && (! io->recv_buf->len))
			nih_io_closed (io);

		break;
	case NIH_IO_MESSAGE:
		if (NIH_LIST_EMPTY (io->send_q) && NIH_LIST_EMPTY (io->recv_q))
			nih_io_closed (io);

		break;
	default:
		nih_assert_not_reached ();
	}
}

/**
 * nih_io_destroy:
 * @io: structure to be destroyed.
 *
 * Closes the file descriptor associated with an NihIo structure so that
 * the structure can be freed.  IF an error is caught by closing the
 * descriptor, the error handler is called instead of the error being raised;
 * this allows you to group your error handling in one place rather than
 * special-case close.
 *
 * Normally used or called from an nih_alloc() destructor.
 *
 * Returns: zero.
 **/
int
nih_io_destroy (NihIo *io)
{
	nih_assert (io != NULL);

	if (io->free)
		*(io->free) = TRUE;

	if ((close (io->watch->fd) < 0) && io->error_handler) {
		nih_error_raise_system ();
		io->error_handler (io->data, io);
	}

	return 0;
}


/**
 * nih_io_first_message:
 * @io: structure to read from.
 *
 * Used to obtain the oldest message in the receive queue of @io, the
 * returned message is not removed from the queue, so this can be used
 * to "peek" at the message or manipulate it.
 *
 * The message can be removed from the queue with nih_list_remove() or
 * nih_free().
 *
 * This may only be used when @io is in message mode.
 *
 * Returns: message in queue, or NULL if the queue is empty.
 **/
static NihIoMessage *
nih_io_first_message (NihIo *io)
{
	nih_assert (io != NULL);
	nih_assert (io->type == NIH_IO_MESSAGE);

	if (NIH_LIST_EMPTY (io->recv_q))
		return NULL;

	return (NihIoMessage *)io->recv_q->next;
}

/**
 * nih_io_read_message:
 * @parent: parent object for new message,
 * @io: structure to read from.
 *
 * Obtains the oldest message in the receive queue of @io, removes it
 * from the queue and returns it reparented.
 *
 * This may only be used when @io is in message mode.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned message.  When all parents
 * of the returned message are freed, the returned message will also be
 * freed.
 *
 * Returns: message from queue, or NULL if the queue is empty.
 **/
NihIoMessage *
nih_io_read_message (const void *parent,
		     NihIo      *io)
{
	NihIoMessage *message;

	nih_assert (io != NULL);
	nih_assert (io->type == NIH_IO_MESSAGE);

	message = nih_io_first_message (io);
	if (message) {
		nih_list_remove (&message->entry);

		nih_ref (message, parent);
		nih_unref (message, io);
	}

	nih_io_shutdown_check (io);

	return message;
}

/**
 * nih_io_send_message:
 * @io: structure to write to,
 * @message: message to write.
 *
 * Appends message to the send queue of @io so that it will be sent in
 * turn when possible.
 *
 * @message itself is added to the queue, and freed once written.
 *
 * When called on a message already in the send queue, this moves it to
 * the end of the queue.  It's entirely permitted to call this on messages
 * taken from the receive queue (usually of another NihIo).
 *
 * This may only be used when @io is in message mode.
 **/
void
nih_io_send_message (NihIo        *io,
		     NihIoMessage *message)
{
	nih_assert (io != NULL);
	nih_assert (io->type == NIH_IO_MESSAGE);
	nih_assert (message != NULL);

	nih_list_add (io->send_q, &message->entry);
	nih_ref (message, io);

	io->watch->events |= NIH_IO_WRITE;
}


/**
 * nih_io_read:
 * @parent: parent object for new string,
 * @io: structure to read from,
 * @len: number of bytes to read.
 *
 * Reads @len bytes from the receive buffer of @io or the oldest message
 * in the receive queue and returns the data in a new string allocated with
 * nih_alloc() that is always NULL terminated even if there was not a NULL
 * in the buffer.
 *
 * @len is updated to contain the actual number of bytes returned.
 *
 * If there are not @len bytes in the buffer or message, the maximum amount
 * there is will be returned, if there is nothing you'll get a zero-length
 * string.
 *
 * If the message has no more data in the buffer, it is removed from the
 * receive queue, and the next call to this function will operate on the
 * next oldest message in the queue.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string, or NULL if insufficient memory.
 **/
char *
nih_io_read (const void *parent,
	     NihIo      *io,
	     size_t     *len)
{
	NihIoMessage *message;
	NihIoBuffer  *buf;
	char         *str;

	nih_assert (io != NULL);
	nih_assert (len != NULL);

	switch (io->type) {
	case NIH_IO_STREAM:
		message = NULL;
		buf = io->recv_buf;
		break;
	case NIH_IO_MESSAGE:
		/* Need to return a zero-length string if there's no message
		 * in the queue.
		 */
		message = nih_io_first_message (io);
		if (! message) {
			*len = 0;
			str = nih_strdup (parent, "");
			goto finish;
		}

		buf = message->data;
		break;
	default:
		nih_assert_not_reached ();
	}

	str = nih_io_buffer_pop (parent, buf, len);

	if (message && (! message->data->len))
		nih_unref (message, io);

finish:
	nih_io_shutdown_check (io);

	return str;
}

/**
 * nih_io_write:
 * @io: structure to write to,
 * @str: data to write,
 * @len: length of @str.
 *
 * Writes @len bytes from @str into the send buffer of @io, or into a new
 * message placed in the send queue.  The data will not be sent immediately
 * but whenever possible.
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
	nih_local NihIoMessage *message = NULL;
	NihIoBuffer *           buf;

	nih_assert (io != NULL);
	nih_assert (str != NULL);

	switch (io->type) {
	case NIH_IO_STREAM:
		message = NULL;
		buf = io->send_buf;
		break;
	case NIH_IO_MESSAGE:
		message = nih_io_message_new (NULL);
		if (! message)
			return -1;

		buf = message->data;
		break;
	default:
		nih_assert_not_reached ();
	}

	if (nih_io_buffer_push (buf, str, len) < 0)
		return -1;

	if (message) {
		nih_io_send_message (io, message);
	} else if (buf->len) {
		io->watch->events |= NIH_IO_WRITE;
	}

	return 0;
}


/**
 * nih_io_get:
 * @parent: parent object for new string,
 * @io: structure to read from,
 * @delim: character to read until.
 *
 * Reads from the receive buffer of @io or the oldest message in the
 * receive queue until a character in @delim or the NULL terminator is
 * found, and returns a new string allocated with nih_alloc() that contains
 * a copy of the string up to, but not including, the delimiter.
 *
 * @delim may be the empty string if only the NULL terminator is considered
 * a delimiter.
 *
 * The string and the delimiter are removed from the buffer or message.
 *
 * If the message has no more data in the buffer, it is removed from the
 * receive queue, and the next call to this function will operate on the
 * next oldest message in the queue.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if delimiter not found or
 * insufficient memory.
 **/
char *
nih_io_get (const void *parent,
	    NihIo      *io,
	    const char *delim)
{
	NihIoMessage *message;
	NihIoBuffer  *buf;
	char         *str;
	size_t        i;

	nih_assert (io != NULL);
	nih_assert (delim != NULL);

	str = NULL;

	switch (io->type) {
	case NIH_IO_STREAM:
		message = NULL;
		buf = io->recv_buf;
		break;
	case NIH_IO_MESSAGE:
		message = nih_io_first_message (io);
		if (! message)
			goto finish;

		buf = message->data;
		break;
	default:
		nih_assert_not_reached ();
	}

	/* Find the end of the string */
	for (i = 0; i < buf->len; i++) {
		if (strchr (delim, buf->buf[i]) || (buf->buf[i] == '\0')) {
			/* Remove the string, and then the delimiter */
			str = nih_io_buffer_pop (parent, buf, &i);
			if (! str)
				return NULL;

			nih_io_buffer_shrink (buf, 1);
			break;
		}
	}

	if (message && (! message->data->len))
		nih_unref (message, io);

finish:
	nih_io_shutdown_check (io);

	return str;
}

/**
 * nih_io_printf:
 * @io: structure to write to,
 * @format: printf format string.
 *
 * Writes data formatted according to the printf-style @format string to
 * the send buffer of @io, or into a new message placed in the send queue.
 * The data will not be sent immediately but whenever possible.
 *
 * Returns: zero on success, negative value if insufficient memory.
 **/
int
nih_io_printf (NihIo      *io,
	       const char *format,
	       ...)
{
	nih_local char *str = NULL;
	va_list         args;
	int             ret;

	nih_assert (io != NULL);
	nih_assert (format != NULL);

	va_start (args, format);
	str = nih_vsprintf (NULL, format, args);
	va_end (args);

	if (! str)
		return -1;

	ret = nih_io_write (io, str, strlen (str));

	return ret;
}


/**
 * nih_io_set_nonblock:
 * @fd: file descriptor to change.
 *
 * Change the flags of @fd so that all operations become non-blocking.
 *
 * Returns: zero on success, negative value on invalid file descriptor.
 **/
int
nih_io_set_nonblock (int fd)
{
	int flags;

	nih_assert (fd >= 0);

	flags = fcntl (fd, F_GETFL);
	if (flags < 0)
		return -1;

	flags |= O_NONBLOCK;

	if (fcntl (fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

/**
 * nih_io_set_cloexec:
 * @fd: file descriptor to change.
 *
 * Change the flags of @fd so that the file descriptor is closed on exec().
 *
 * Returns: zero on success, negative value on invalid file descriptor.
 **/
int
nih_io_set_cloexec (int fd)
{
	int flags;

	nih_assert (fd >= 0);

	flags = fcntl (fd, F_GETFD);
	if (flags < 0)
		return -1;

	flags |= FD_CLOEXEC;

	if (fcntl (fd, F_SETFD, flags) < 0)
		return -1;

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
