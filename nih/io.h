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

#include <sys/types.h>
#include <sys/poll.h>

#include <nih/macros.h>
#include <nih/list.h>


typedef struct nih_io_watch NihIoWatch;
typedef struct nih_io       NihIo;

/**
 * NihIoCb:
 * @data: data pointer given with callback,
 * @watch: #NihIoWatch for which an event occurred,
 * @events: events that occurred.
 *
 * I/O callback functions are called whenever an event occurs on a file
 * descriptor or socket being watched by an #NihIoWatch.
 **/
typedef void (*NihIoCb) (void *, NihIoWatch *, short);

/**
 * NihIoReadCb:
 * @data: data pointer given with callback,
 * @io: #NihIo with data to be read,
 * @buf: buffer data is available in,
 * @len: bytes in @buf.
 *
 * I/O read callback functions are called whenever there is new data in the
 * receive buffer of an #NihIo object.
 **/
typedef void (*NihIoReadCb) (void *, NihIo *, const char *, size_t);

/**
 * NihIoCloseCb:
 * @data: data pointer given with callback,
 * @io: #NihIo that closed.
 *
 * I/O close callback functions are called when the remote end of the
 * associated file descriptor is closed.
 *
 * This function should take appropriate action, which may include freeing
 * the #NihIo structure and closing the socket.
 **/
typedef void (*NihIoCloseCb) (void *, NihIo *);

/**
 * NihIoErrorCb:
 * @data: data pointer given with callback,
 * @io: #NihIo that caused the error.
 *
 * I/O error callback functions are called when an error was raised while
 * processing the file descriptor.  The error can be obtained using
 * #nih_error_get.
 *
 * This function should take appropriate action, which may include freeing
 * the #NihIo structure and closing the socket.
 **/
typedef void (*NihIoErrorCb) (void *, NihIo *);


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
 * NihIo:
 * @watch: associated file descriptor watch,
 * @send_buf: buffer that pools data to be sent,
 * @recv_buf: buffer that pools data received,
 * @read_cb: function called when new data in @recv_buf,
 * @close_cb: function called when socket closes,
 * @error_cb: function called when an error occurs,
 * @data: pointer passed to callbacks.
 *
 * This structure implements more featureful I/O handling than provided by
 * an #NihIoWatch alone.  It combines an #NihIoWatch and two #NihIoBuffer
 * structures to implement a high-throughput alternative to the
 * traditional stdio functions.
 *
 * Those functions are optimised to reduce the number of read() or write()
 * calls made on a file descriptor, and cannot be used to pool large
 * amounts of data for processing.
 *
 * The #NihIo functions are instead optimised for being able to queue and
 * receive much data as possible, and have the data sent in the background
 * or processed at your leisure.
 **/
struct nih_io {
	NihIoWatch   *watch;
	NihIoBuffer  *send_buf;
	NihIoBuffer  *recv_buf;

	NihIoReadCb   read_cb;
	NihIoCloseCb  close_cb;
	NihIoErrorCb  error_cb;
	void         *data;
};


NIH_BEGIN_EXTERN

NihIoWatch * nih_io_add_watch     (void *parent, int fd, short events,
				   NihIoCb callback, void *data);

int          nih_io_poll_fds      (struct pollfd **ufds, nfds_t *nfds);
void         nih_io_handle_fds    (const struct pollfd *ufds, nfds_t nfds);


NihIoBuffer *nih_io_buffer_new    (void *parent)
	__attribute__ ((warn_unused_result, malloc));

int          nih_io_buffer_resize (NihIoBuffer *buffer, size_t grow);
char *       nih_io_buffer_pop    (void *parent, NihIoBuffer *buffer,
				   size_t len)
	__attribute__ ((warn_unused_result, malloc));
int          nih_io_buffer_push   (NihIoBuffer *buffer, const char *str,
				   size_t len);


NihIo *      nih_io_reopen        (void *parent, int fd, NihIoReadCb read_cb,
				   NihIoCloseCb close_cb,
				   NihIoErrorCb error_cb, void *data);
void         nih_io_close         (NihIo *io);

char *       nih_io_read          (void *parent, NihIo *io, size_t len)
	__attribute__ ((warn_unused_result, malloc));
int          nih_io_write         (NihIo *io, const char *str, size_t len);

char *       nih_io_get           (void *parent, NihIo *io, const char *delim)
	__attribute__ ((warn_unused_result, malloc));

ssize_t      nih_io_printf        (NihIo *io, const char *format, ...)
	__attribute__ ((format (printf, 2, 3)));

int          nih_io_set_nonblock  (int fd);
int          nih_io_set_cloexec   (int fd);


NIH_END_EXTERN

#endif /* NIH_IO_H */
