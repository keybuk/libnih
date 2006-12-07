/* libnih
 *
 * test_io.c - test suite for nih/io.c
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

#include <nih/test.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/io.h>
#include <nih/logging.h>
#include <nih/error.h>


static int watcher_called = 0;
static void *last_data = NULL;
static NihIoWatch *last_watch = NULL;
static NihIoEvents last_events = 0;

static void
my_watcher (void *data, NihIoWatch *watch, NihIoEvents events)
{
	watcher_called++;
	last_data = data;
	last_watch = watch;
	last_events = events;
}

void
test_add_watch (void)
{
	NihIoWatch *watch;
	int         fds[2];

	/* Check that we can add a watch on a file descriptor and that the
	 * structure is properly filled in and placed in a list.
	 */
	TEST_FUNCTION ("nih_io_add_watch");
	pipe (fds);
	watch = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				  my_watcher, &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihIoWatch));
	TEST_EQ (watch->fd, fds[0]);
	TEST_EQ (watch->events, NIH_IO_READ);
	TEST_EQ_P (watch->watcher, my_watcher);
	TEST_EQ_P (watch->data, &watch);

	nih_list_free (&watch->entry);

	close (fds[0]);
	close (fds[1]);
}


void
test_select_fds (void)
{
	NihIoWatch *watch1, *watch2, *watch3;
	fd_set      readfds, writefds, exceptfds;
	int         nfds, fds[2];

	/* Check that the select file descriptor sets are correctly filled
	 * based on a set of watches we add.
	 */
	TEST_FUNCTION ("nih_io_select_fds");
	pipe (fds);
	watch1 = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				   my_watcher, &watch1);
	watch2 = nih_io_add_watch (NULL, fds[1], NIH_IO_WRITE,
				   my_watcher, &watch2);
	watch3 = nih_io_add_watch (NULL, fds[0], NIH_IO_EXCEPT,
				   my_watcher, &watch3);

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);

	TEST_EQ (nfds, MAX (fds[0], fds[1]) + 1);
	TEST_TRUE (FD_ISSET (fds[0], &readfds));
	TEST_FALSE (FD_ISSET (fds[0], &writefds));
	TEST_TRUE (FD_ISSET (fds[0], &exceptfds));
	TEST_FALSE (FD_ISSET (fds[1], &readfds));
	TEST_TRUE (FD_ISSET (fds[1], &writefds));
	TEST_FALSE (FD_ISSET (fds[1], &exceptfds));

	nih_list_free (&watch1->entry);
	nih_list_free (&watch2->entry);
	nih_list_free (&watch3->entry);

	close (fds[0]);
	close (fds[1]);
}

void
test_handle_fds (void)
{
	NihIoWatch    *watch1, *watch2, *watch3;
	fd_set         readfds, writefds, exceptfds;
	int            fds[2];

	TEST_FUNCTION ("nih_io_handle_fds");
	pipe (fds);
	watch1 = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				   my_watcher, &watch1);
	watch2 = nih_io_add_watch (NULL, fds[1], NIH_IO_WRITE,
				   my_watcher, &watch2);
	watch3 = nih_io_add_watch (NULL, fds[0], NIH_IO_EXCEPT,
				   my_watcher, &watch3);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	/* Check that something watching a file descriptor for readability
	 * is called, with the right arguments passed; and that another
	 * watch on the same file descriptor for different events is not
	 * called.
	 */
	TEST_FEATURE ("with select for read");
	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 1);
	TEST_EQ (last_events, NIH_IO_READ);
	TEST_EQ_P (last_watch, watch1);
	TEST_EQ_P (last_data, &watch1);


	/* Check that something watching a file descriptor for an exception
	 * is called, and that the watch on the same descriptor for reading
	 * is not called.
	 */
	TEST_FEATURE ("with select for exception");
	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	FD_ZERO (&readfds);
	FD_SET (fds[0], &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 1);
	TEST_EQ (last_events, NIH_IO_EXCEPT);
	TEST_EQ_P (last_watch, watch3);
	TEST_EQ_P (last_data, &watch3);


	/* Check that nothing is called if the file descriptor and events
	 * being polled don't match anything.
	 */
	TEST_FEATURE ("with unwatched select");
	watcher_called = 0;
	FD_ZERO (&exceptfds);
	FD_SET (fds[1], &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 0);


	nih_list_free (&watch1->entry);
	nih_list_free (&watch2->entry);
	nih_list_free (&watch3->entry);

	close (fds[0]);
	close (fds[1]);
}


void
test_buffer_new (void)
{
	NihIoBuffer *buf;

	/* Check that we can create a new empty buffer, and that the
	 * structure members are correct.
	 */
	TEST_FUNCTION ("nih_io_buffer_new");
	buf = nih_io_buffer_new (NULL);

	TEST_ALLOC_SIZE (buf, sizeof (NihIoBuffer));
	TEST_EQ_P (buf->buf, NULL);
	TEST_EQ (buf->size, 0);
	TEST_EQ (buf->len, 0);

	nih_free (buf);
}

void
test_buffer_resize (void)
{
	NihIoBuffer *buf;

	TEST_FUNCTION ("nih_io_buffer_resize");
	buf = nih_io_buffer_new (NULL);

	/* Check that we can resize a NULL buffer; we ask for half a page
	 * and expect to get a full page allocated as a child of the buffer
	 * itself.
	 */
	TEST_FEATURE ("with empty buffer and half increase");
	nih_io_buffer_resize (buf, BUFSIZ / 2);

	TEST_ALLOC_PARENT (buf->buf, buf);
	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);
	TEST_EQ (buf->len, 0);


	/* Check that we can increase the size by a full page, and not
	 * have anything change because there's no space used yet.
	 */
	TEST_FEATURE ("with empty but alloc'd buffer and full increase");
	nih_io_buffer_resize (buf, BUFSIZ);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);


	/* Check that we can increase the size beyond a full page, and
	 * get another page of allocated space.
	 */
	TEST_FEATURE ("with empty but alloc'd buffer and larger increase");
	nih_io_buffer_resize (buf, BUFSIZ + BUFSIZ / 2);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
	TEST_EQ (buf->size, BUFSIZ * 2);


	/* Check that we can drop the size of an allocated but empty buffer
	 * back to zero and have the buffer freed.
	 */
	TEST_FEATURE ("with alloc'd buffer and no data");
	nih_io_buffer_resize (buf, 0);

	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);


	/* Check that asking for a page more space when we claim to be
	 * using half a page gives us a full two pages of space.
	 */
	TEST_FEATURE ("with part-full buffer and increase");
	buf->len = BUFSIZ / 2;
	nih_io_buffer_resize (buf, BUFSIZ);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
	TEST_EQ (buf->size, BUFSIZ * 2);
	TEST_EQ (buf->len, BUFSIZ / 2);


	/* Check that asking for an increase smaller than the difference
	 * between the buffer size and length has no effect.
	 */
	TEST_FEATURE ("with no change");
	buf->len = BUFSIZ + BUFSIZ / 2;
	nih_io_buffer_resize (buf, 80);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
	TEST_EQ (buf->size, BUFSIZ * 2);
	TEST_EQ (buf->len, BUFSIZ + BUFSIZ / 2);

	nih_free (buf);
}

void
test_buffer_pop (void)
{
	NihIoBuffer *buf;
	char        *str;

	TEST_FUNCTION ("nih_io_buffer_pop");
	buf = nih_io_buffer_new (NULL);
	nih_io_buffer_push (buf, "this is a test of the buffer code", 33);


	/* Check that we can pop some bytes out of a buffer, and have a
	 * NULL-terminated string returned that is allocated with nih_alloc.
	 * The buffer should be shrunk appropriately and moved up.
	 */
	TEST_FEATURE ("with full buffer");
	str = nih_io_buffer_pop (NULL, buf, 14);

	TEST_ALLOC_SIZE (str, 15);
	TEST_EQ (str[14], '\0');
	TEST_EQ_STR (str, "this is a test");

	TEST_EQ (buf->len, 19);
	TEST_EQ_MEM (buf->buf, " of the buffer code", 19);

	nih_free (str);


	/* Check that we can empty the buffer and the buffer is freed. */
	TEST_FEATURE ("with request to empty buffer");
	str = nih_io_buffer_pop (NULL, buf, 19);

	TEST_ALLOC_SIZE (str, 20);
	TEST_EQ (str[19], '\0');
	TEST_EQ_STR (str, " of the buffer code");

	TEST_EQ (buf->len, 0);
	TEST_EQ (buf->size, 0);
	TEST_EQ_P (buf->buf, NULL);

	nih_free (str);

	nih_free (buf);
}

void
test_buffer_push (void)
{
	NihIoBuffer *buf;

	TEST_FUNCTION ("nih_io_buffer_push");
	buf = nih_io_buffer_new (NULL);

	/* Check that we can push data into an empty buffer, which will
	 * store it in the buffer.
	 */
	TEST_FEATURE ("with empty buffer");
	nih_io_buffer_push (buf, "test", 4);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);
	TEST_EQ (buf->len, 4);
	TEST_EQ_MEM (buf->buf, "test", 4);


	/* Check that we can push more data into that buffer, which will
	 * append it to the data already there.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_buffer_push (buf, "ing the buffer code", 14);

	TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
	TEST_EQ (buf->size, BUFSIZ);
	TEST_EQ (buf->len, 18);
	TEST_EQ_MEM (buf->buf, "testing the buffer code", 18);

	nih_free (buf);
}


static int read_called = 0;
static int close_called = 0;
static int error_called = 0;
static NihError *last_error = NULL;
static const char *last_str = NULL;
static size_t last_len = 0;

static void
my_reader (void       *data,
	   NihIo      *io,
	   const char *str,
	   size_t      len)
{
	read_called++;
	last_data = data;
	last_str = str;
	last_len = len;
}

static void
my_close_handler (void  *data,
		  NihIo *io)
{
	last_data = data;
	close_called++;
}

static void
my_error_handler (void  *data,
		  NihIo *io)
{
	last_data = data;
	last_error = nih_error_get ();
	error_called++;
}

void
test_reopen (void)
{
	NihIo            *io;
	int               fds[2];
	struct sigaction  oldact;

	/* Check that we can create an NihIo structure from an existing
	 * file descriptor; the structure should be correctly populated
	 * and assigned an NihIoWatch.  The file descriptor should be
	 * altered so that it is non-blocking.
	 */
	TEST_FUNCTION ("nih_io_reopen");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], my_reader, my_close_handler,
			    my_error_handler, &io);

	TEST_ALLOC_SIZE (io, sizeof (NihIo));
	TEST_ALLOC_PARENT (io->send_buf, io);
	TEST_ALLOC_PARENT (io->recv_buf, io);
	TEST_EQ_P (io->reader, my_reader);
	TEST_EQ_P (io->close_handler, my_close_handler);
	TEST_EQ_P (io->error_handler, my_error_handler);
	TEST_EQ_P (io->data, &io);

	TEST_ALLOC_PARENT (io->watch, io);
	TEST_EQ (io->watch->fd, fds[0]);
	TEST_EQ (io->watch->events, NIH_IO_READ);
	TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);


	nih_free (io);

	close (fds[0]);
	close (fds[1]);


	/* Check that the SIGPIPE signal will now be ignored */
	sigaction (SIGPIPE, NULL, &oldact);
	TEST_EQ (oldact.sa_handler, SIG_IGN);
}


static int free_called;

static int
destructor_called (void *ptr)
{
	free_called++;

	return 0;
}

void
test_shutdown (void)
{
	NihIo  *io;
	int     fds[2];
	fd_set  readfds, writefds, exceptfds;

	TEST_FUNCTION ("nih_io_shutdown");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "some data", 9);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	/* Check that shutting down a socket with data in the buffer
	 * merely marks it as shutdown and neither closes the socket or
	 * frees the structure.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_shutdown (io);

	TEST_TRUE (io->shutdown);
	TEST_FALSE (free_called);
	TEST_GE (fcntl (fds[0], F_GETFD), 0);


	/* Check that handling the data in the buffer, emptying it, causes
	 * the shutdown socket to be closed and the structure to be freed.
	 */
	TEST_FEATURE ("with data being handled");
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);
}

void
test_close (void)
{
	NihIo *io;
	int    fds[2];

	TEST_FUNCTION ("nih_io_close");

	/* Check that closing an open file descriptor doesn't call the error
	 * handler, and just closes the fd and frees the structure.
	 */
	TEST_FEATURE ("with open file descriptor");
	pipe (fds);
	error_called = 0;
	io = nih_io_reopen (NULL, fds[0], NULL, NULL, my_error_handler, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	nih_io_close (io);

	TEST_FALSE (error_called);
	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that closing a file descriptor that's already closed
	 * results in the error handler being called with an EBADF system
	 * error and the data pointer, followed by the structure being
	 * freed.
	 */
	TEST_FEATURE ("with closed file descriptor");
	pipe (fds);
	error_called = 0;
	last_data = NULL;
	last_error = NULL;
	io = nih_io_reopen (NULL, fds[0], NULL, NULL, my_error_handler, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	close (fds[0]);
	nih_io_close (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);
	TEST_TRUE (free_called);

	nih_free (last_error);

	close (fds[1]);
}

void
test_watcher (void)
{
	NihIo  *io;
	int     fds[2];
	fd_set  readfds, writefds, exceptfds;
	FILE   *output;

	TEST_FUNCTION ("nih_io_watcher");

	/* Check that data to be read on a socket watched by NihIo ends up
	 * in the receive buffer, and results in the reader function being
	 * called with the right arguments.
	 */
	TEST_FEATURE ("with data to read");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], my_reader, my_close_handler,
			    my_error_handler, &io);

	write (fds[1], "this is a test", 14);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);

	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (read_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 14);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test", 14);


	/* Check that the reader function is called again when more data
	 * comes in, and that the buffer contains both sets of data.
	 */
	TEST_FEATURE ("with more data to read");
	write (fds[1], " of the callback code", 19);

	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (read_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 33);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test of the callback code",
		     33);


	/* Check that the reader function is also closed when the remote end
	 * has been closed; along with the close function.
	 */
	TEST_FEATURE ("with remote end closed");
	read_called = 0;
	close_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (read_called);
	TEST_TRUE (close_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 33);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test of the callback code",
		     33);


	/* Check that the reader function and error handler are called if
	 * the local end gets closed.  The error should be EBADF.
	 */
	TEST_FEATURE ("with local end closed");
	read_called = 0;
	error_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;
	last_error = NULL;

	close (fds[0]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_TRUE (read_called);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, io->recv_buf->buf);
	TEST_EQ (last_len, io->recv_buf->len);
	TEST_EQ (io->recv_buf->len, 33);
	TEST_EQ_MEM (io->recv_buf->buf, "this is a test of the callback code",
		     33);

	nih_free (last_error);
	nih_free (io);


	/* Check that if the remote end closes and there's no close handler,
	 * the file descriptor is closed and the structure freed.
	 */
	TEST_FEATURE ("with no close handler");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], my_reader, NULL, NULL, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (free_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);


	/* Check that if the local end closes and there's no error handler
	 * that the structure is freed.
	 */
	TEST_FEATURE ("with no error handler");
	pipe (fds);
	io = nih_io_reopen (NULL, fds[0], my_reader, NULL, NULL, &io);

	free_called = 0;
	nih_alloc_set_destructor (io, destructor_called);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

	nih_log_set_priority (NIH_LOG_FATAL);
	close (fds[0]);
	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);
	nih_log_set_priority (NIH_LOG_DEBUG);

	TEST_TRUE (free_called);

	FD_ZERO (&readfds);


	/* Check that data in the send buffer is written to the file
	 * descriptor if it's pollable for writing.  Once the data has been
	 * written, the watch should no longer be checking for writability.
	 */
	TEST_FEATURE ("with data to write");
	output = tmpfile ();
	io = nih_io_reopen (NULL, fileno (output), NULL,
			    my_close_handler, my_error_handler, &io);

	nih_io_printf (io, "this is a test\n");

	FD_SET (fileno (output), &writefds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	rewind (output);

	TEST_FILE_EQ (output, "this is a test\n");
	TEST_FILE_END (output);

	TEST_EQ (io->send_buf->len, 0);
	TEST_EQ (io->send_buf->size, 0);
	TEST_EQ_P (io->send_buf->buf, NULL);

	TEST_FALSE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can write more data and that is send out to the
	 * file descriptor as well.
	 */
	TEST_FEATURE ("with more data to write");
	nih_io_printf (io, "so is this\n");
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	rewind (output);

	TEST_FILE_EQ (output, "this is a test\n");
	TEST_FILE_EQ (output, "so is this\n");
	TEST_FILE_END (output);

	TEST_EQ (io->send_buf->len, 0);
	TEST_EQ (io->send_buf->size, 0);
	TEST_EQ_P (io->send_buf->buf, NULL);

	TEST_FALSE (io->watch->events & NIH_IO_WRITE);

	fclose (output);


	/* Check that an attempt to write to a closed file results in the
	 * error handler being called.
	 */
	TEST_FEATURE ("with closed file");
	error_called = 0;
	last_data = NULL;
	last_error = NULL;

	nih_io_printf (io, "this write fails\n");
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);

	nih_free (last_error);

	nih_free (io);
}


void
test_read (void)
{
	NihIo *io;
	char  *str;

	TEST_FUNCTION ("nih_io_read");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "this is a test of the io code", 29);


	/* Check that we can read data in the NihIo receive buffer, and the
	 * data is returned NULL-terminated, allocated with nih_alloc and
	 * removed from the front of the receive buffer itself.
	 */
	TEST_FEATURE ("with full buffer");
	str = nih_io_read (NULL, io, 14);

	TEST_ALLOC_SIZE (str, 15);
	TEST_EQ (str[14], '\0');
	TEST_EQ_STR (str, "this is a test");
	TEST_EQ (io->recv_buf->len, 15);
	TEST_EQ_MEM (io->recv_buf->buf, " of the io code", 15);

	nih_free (str);


	/* Check that we can empty all of the data from the NihIo receive
	 * buffer, which results in the buffer being freed.
	 */
	TEST_FEATURE ("with request to empty buffer");
	str = nih_io_read (NULL, io, 15);

	TEST_ALLOC_SIZE (str, 16);
	TEST_EQ (str[15], '\0');
	TEST_EQ_STR (str, " of the io code");
	TEST_EQ (io->recv_buf->len, 0);
	TEST_EQ (io->recv_buf->size, 0);
	TEST_EQ_P (io->recv_buf->buf, NULL);

	nih_free (str);

	nih_free (io);
}

void
test_write (void)
{
	NihIo *io;

	TEST_FUNCTION ("nih_io_write");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);

	/* Check that we can write data into the NihIo send buffer, the
	 * buffer should contain the data and be a page in size.  The
	 * watch should also now be looking for writability.
	 */
	TEST_FEATURE ("with empty buffer");
	nih_io_write (io, "test", 4);

	TEST_ALLOC_SIZE (io->send_buf->buf, BUFSIZ);
	TEST_EQ (io->send_buf->size, BUFSIZ);
	TEST_EQ (io->send_buf->len, 4);
	TEST_EQ_MEM (io->send_buf->buf, "test", 4);
	TEST_TRUE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can write more data onto the end of the NihIo
	 * send buffer, which increases its size.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_write (io, "ing the io code", 10);

	TEST_EQ (io->send_buf->len, 14);
	TEST_EQ_MEM (io->send_buf->buf, "testing the io", 14);

	nih_free (io);
}

void
test_get (void)
{
	NihIo *io;
	char  *str;

	TEST_FUNCTION ("nih_io_get");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "some data\n", 10);
	nih_io_buffer_push (io->recv_buf, "and another line\n", 17);
	nih_io_buffer_push (io->recv_buf, "incomplete", 10);

	/* Check that we can take data from the front of a buffer up until
	 * the first embedded new line (which isn't returned), and have the
	 * buffer shuffled up.
	 */
	TEST_FEATURE ("with full buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 10);
	TEST_EQ_STR (str, "some data");

	nih_free (str);


	/* Check that we can read up to the next line line. */
	TEST_FEATURE ("with part-full buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 17);
	TEST_EQ_STR (str, "and another line");

	nih_free (str);


	/* Check that NULL is returned if the data in the buffer doesn't
	 * contain the delimeter or a NULL terminator.
	 */
	TEST_FEATURE ("with incomplete line in buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_EQ_P (str, NULL);


	/* Check that a NULL terminator is sufficient to return the data
	 * in the buffer, which should now be empty.
	 */
	TEST_FEATURE ("with null-terminated string in buffer");
	nih_io_buffer_push (io->recv_buf, "\0", 1);
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 11);
	TEST_EQ_STR (str, "incomplete");

	TEST_EQ (io->recv_buf->len, 0);

	nih_free (str);

	nih_free (io);
}

void
test_printf (void)
{
	NihIo *io;

	TEST_FUNCTION ("nih_io_printf");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);

	/* Check that we can write a line of formatted data into the send
	 * buffer, which should be written without a NULL terminator.
	 * The watch should also look for writability.
	 */
	TEST_FEATURE ("with empty buffer");
	nih_io_printf (io, "this is a %d %s test\n", 4, "format");

	TEST_ALLOC_SIZE (io->send_buf->buf, BUFSIZ);
	TEST_EQ (io->send_buf->size, BUFSIZ);
	TEST_EQ (io->send_buf->len, 24);
	TEST_EQ_MEM (io->send_buf->buf, "this is a 4 format test\n", 24);
	TEST_TRUE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can append a further line of formatted data into
	 * the send buffer
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_printf (io, "and this is %s line\n", "another");

	TEST_EQ (io->send_buf->len, 49);
	TEST_EQ_MEM (io->send_buf->buf,
		     "this is a 4 format test\nand this is another line\n",
		     49);

	nih_free (io);
}


void
test_set_nonblock (void)
{
	int fds[2];

	/* Check that we can trivially mark a socket to be non-blocking. */
	TEST_FUNCTION ("nih_io_set_nonblock");
	pipe (fds);
	nih_io_set_nonblock (fds[0]);

	TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

	close (fds[0]);
	close (fds[1]);
}

void
test_set_cloexec (void)
{
	int fds[2];

	/* Check that we can trivially mark a socket to be closed on exec. */
	TEST_FUNCTION ("nih_io_set_cloexec");
	pipe (fds);
	nih_io_set_cloexec (fds[0]);

	TEST_TRUE (fcntl (fds[0], F_GETFD) & FD_CLOEXEC);

	close (fds[0]);
	close (fds[1]);
}


int
main (int   argc,
      char *argv[])
{
	test_add_watch ();
	test_select_fds ();
	test_handle_fds ();
	test_buffer_new ();
	test_buffer_resize ();
	test_buffer_push ();
	test_buffer_pop ();
	test_reopen ();
	test_shutdown ();
	test_close ();
	test_watcher ();
	test_read ();
	test_write ();
	test_get ();
	test_printf ();
	test_set_nonblock ();
	test_set_cloexec ();

	return 0;
}
