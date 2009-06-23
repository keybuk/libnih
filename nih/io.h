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

#ifndef NIH_IO_H
#define NIH_IO_H

#include <sys/types.h>
#include <sys/socket.h>

#include <nih/macros.h>
#include <nih/list.h>


/**
 * NihIoType:
 *
 * Whether an NihIo structure is used for a buffered stream of data, or
 * a queue of discreet messages.
 **/
typedef enum {
	NIH_IO_STREAM,
	NIH_IO_MESSAGE
} NihIoType;

/**
 * NihIoEvents:
 *
 * Events that we can watch for, generally used as a bit mask of the events
 * that have occurred.
 **/
typedef enum {
	NIH_IO_NONE   = 00,
	NIH_IO_READ   = 01,
	NIH_IO_WRITE  = 02,
	NIH_IO_EXCEPT = 04,
} NihIoEvents;


/* Predefine the typedefs as we use them in the callbacks */
typedef struct nih_io_watch NihIoWatch;
typedef struct nih_io       NihIo;

/**
 * NihIoWatcher:
 * @data: data pointer given when registered,
 * @watch: #NihIoWatch for which an event occurred,
 * @events: events that occurred.
 *
 * An I/O watcher is a function that is called whenever an event occurs
 * on a file descriptor or socket being watched.  It is safe for the
 * watcher to remove the watch during the call.
 **/
typedef void (*NihIoWatcher) (void *data, NihIoWatch *watch,
			      NihIoEvents events);

/**
 * NihIoReader:
 * @data: data pointer given when registered,
 * @io: NihIo with data to be read,
 * @buf: buffer data is available in,
 * @len: bytes in @buf.
 *
 * An I/O reader is a function that is called whenever new data or a new
 * message has been received on a file descriptor or socket and placed
 * into the receive buffer or queue.
 *
 * In stream mode, @buf and @len will point to the entire receive buffer
 * and this function need not clear the buffer, it is entirely permitted
 * for the data to be left there.  When further data arrives, the buffer
 * will be extended and the reader called again.
 *
 * In message mode, @buf and @len will point to the contents of the oldest
 * message in the receive queue.  You'll almost certainly want to remove
 * this message from the queue, otherwise when a new message arrives, the
 * function will still be called with the same oldest message.
 *
 * It is safe to call nih_io_close() from within the reader function, this
 * results in the structure being flagged to be closed when the watcher
 * that invokes it has finished.  You must not nih_free() @io or cause it
 * to be freed from within this function, except by nih_io_close().
 **/
typedef void (*NihIoReader) (void *data, NihIo *io,
			     const char *buf, size_t len);

/**
 * NihIoCloseHandler:
 * @data: data pointer given when registered.
 * @io: NihIo that closed.
 *
 * An I/O close handler is a function that is called when the remote end
 * of a file descriptor or socket is closed and data can no longer be
 * read from it.
 *
 * It should take appropriate action, which may include closing the
 * file descriptor with nih_io_close().  You must not nih_free() @io or
 * cause it to be freed from within this function, except by nih_io_close().
 **/
typedef void (*NihIoCloseHandler) (void *data, NihIo *io);

/**
 * NihIoErrorHandler:
 * @data: data pointer given when registered,
 * @io: NihIo that caused the error.
 *
 * An I/O error handler is a function that is called to handle an error
 * raise while reading from the file descriptor or socket.  The error
 * itself can be obtained using nih_error_get().
 *
 * It should take appropriate action, which may include closing the
 * file descriptor with nih_io_close().  You must not nih_free() @io or
 * cause it to be freed from within this function, except by nih_io_close().
 **/
typedef void (*NihIoErrorHandler) (void *data, NihIo *io);


/**
 * NihIoWatch:
 * @entry: list header,
 * @fd: file descriptor,
 * @events: events to watch for,
 * @watcher: function called when @events occur on @fd,
 * @data: pointer passed to @watcher.
 *
 * This structure represents the most basic kind of I/O handling, a watch
 * on a file descriptor or socket that causes a function to be called
 * when listed events occur.
 *
 * The watch can be cancelled by calling nih_list_remove() on the structure
 * as they are held in a list internally.
 **/
struct nih_io_watch {
	NihList       entry;
	int           fd;
	NihIoEvents   events;

	NihIoWatcher  watcher;
	void         *data;
};

/**
 * NihIoBuffer:
 * @buf: memory allocated for buffer,
 * @size: allocated size of @buf,
 * @len: number of bytes of @buf used.
 *
 * This structure is used to represent a buffer holding data that is
 * waiting to be sent or processed.
 **/
typedef struct nih_io_buffer {
	char   *buf;
	size_t  size;
	size_t  len;
} NihIoBuffer;

/**
 * NihIoMessage:
 * @entry: list header,
 * @addr: address received from or to be sent to,
 * @addrlen: length of @addr,
 * @data: buffer for message data,
 * @control: NULL-terminated array of control messages,
 * @int_data: user-supplied integer data,
 * @ptr_data: user-supplied pointer data.
 *
 * This structure is used to represent an individual message waiting in
 * a queue to be sent or processed.
 *
 * When a message is in the queue, it is sometimes useful to be able to
 * associate it with the source or destination of the message, for example
 * when handling errors.  You may use the @int_data or @ptr_member to store
 * an integer or pointer value that is useful to you.  These are not usually
 * initialised.
 **/
typedef struct nih_io_message {
	NihList           entry;

	struct sockaddr  *addr;
	socklen_t         addrlen;

	NihIoBuffer      *data;
	struct cmsghdr  **control;

	union {
		int      int_data;
		void    *ptr_data;
	};
} NihIoMessage;

/**
 * NihIo:
 * @type: type of structure,
 * @watch: associated file descriptor watch,
 * @send_buf: buffer that pools data to be sent (NIH_IO_STREAM),
 * @send_q: queue of messages to be sent (NIH_IO_MESSAGE),
 * @recv_buf: buffer that pools data received (NIH_IO_STREAM),
 * @recv_q: queue of messages received (NIH_IO_MESSAGE),
 * @reader: function called when new data in @recv_buf or @recv_q,
 * @close_handler: function called when socket closes,
 * @error_handler: function called when an error occurs,
 * @data: pointer passed to functions,
 * @shutdown: TRUE if the structure should be freed once the buffers are empty,
 * @free: pointer to variable to set to TRUE if freed during the watcher.
 *
 * This structure implements more featureful I/O handling than provided by
 * an NihIoWatch alone.
 *
 * When used in the stream mode (@type is NIH_IO_STREAM), it combines an
 * NihIoWatch and two NihIoBuffer structures to implement a high-throughput
 * alternative to the traditional stdio functions.
 *
 * Those functions are optimised to reduce the number of read() or write()
 * calls made on a file descriptor, and cannot be used to pool large
 * amounts of data for processing.
 *
 * The NihIo functions are instead optimised for being able to queue and
 * receive much data as possible, and have the data sent in the background
 * or processed at your leisure.
 *
 * When used in the message mode (@type is NIH_IO_MESSAGE), it combines the
 * NihIoWatch with an NihList of NihIoMessage structures to implement
 * asynchronous handling of datagram sockets.
 **/
struct nih_io {
	NihIoType            type;

	NihIoWatch          *watch;
	union {
		NihIoBuffer *send_buf;
		NihList     *send_q;
	};
	union {
		NihIoBuffer *recv_buf;
		NihList     *recv_q;
	};

	NihIoReader          reader;
	NihIoCloseHandler    close_handler;
	NihIoErrorHandler    error_handler;
	void                *data;

	int                  shutdown;
	int                 *free;
};


NIH_BEGIN_EXTERN

extern NihList *nih_io_watches;


void          nih_io_init                (void);

NihIoWatch *  nih_io_add_watch           (const void *parent, int fd,
					  NihIoEvents events,
					  NihIoWatcher watcher, void *data)
	__attribute__ ((warn_unused_result, malloc));

void          nih_io_select_fds          (int *nfds, fd_set *readfds,
					  fd_set *writefds, fd_set *exceptfds);
void          nih_io_handle_fds          (fd_set *readfds, fd_set *writewfds,
					  fd_set *exceptfds);


NihIoBuffer * nih_io_buffer_new          (const void *parent)
	__attribute__ ((warn_unused_result, malloc));

int           nih_io_buffer_resize       (NihIoBuffer *buffer, size_t grow);
char *        nih_io_buffer_pop          (const void *parent,
					  NihIoBuffer *buffer, size_t *len)
	__attribute__ ((warn_unused_result, malloc));
void          nih_io_buffer_shrink       (NihIoBuffer *buffer, size_t len);
int           nih_io_buffer_push         (NihIoBuffer *buffer,
					  const char *str, size_t len)
	__attribute__ ((warn_unused_result));


NihIoMessage *nih_io_message_new         (const void *parent)
	__attribute__ ((warn_unused_result, malloc));

int           nih_io_message_add_control (NihIoMessage *message, int level,
					  int type, socklen_t len,
					  const void *data)
	__attribute__ ((warn_unused_result));

NihIoMessage *nih_io_message_recv        (const void *parent, int fd,
					  size_t *len)
	__attribute__ ((warn_unused_result, malloc));
ssize_t       nih_io_message_send        (NihIoMessage *message, int fd)
	__attribute__ ((warn_unused_result));


NihIo *       nih_io_reopen              (const void *parent, int fd,
					  NihIoType type, NihIoReader reader,
					  NihIoCloseHandler close_handler,
					  NihIoErrorHandler error_handler,
					  void *data)
	__attribute__ ((warn_unused_result, malloc));
void          nih_io_shutdown            (NihIo *io);
int           nih_io_destroy             (NihIo *io);

NihIoMessage *nih_io_read_message        (const void *parent, NihIo *io);
void          nih_io_send_message        (NihIo *io, NihIoMessage *message);

char *        nih_io_read                (const void *parent, NihIo *io,
					  size_t *len)
	__attribute__ ((warn_unused_result, malloc));
int           nih_io_write               (NihIo *io, const char *str,
					  size_t len)
	__attribute__ ((warn_unused_result));

char *        nih_io_get                 (const void *parent, NihIo *io,
					  const char *delim)
	__attribute__ ((warn_unused_result, malloc));

int           nih_io_printf              (NihIo *io, const char *format, ...)
	__attribute__ ((warn_unused_result, format (printf, 2, 3)));


int           nih_io_set_nonblock        (int fd);
int           nih_io_set_cloexec         (int fd);

ssize_t       nih_io_get_family          (int fd);

NIH_END_EXTERN

#endif /* NIH_IO_H */
