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

#include <config.h>

#include <sys/select.h>

#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/io.h>
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

int
test_add_watch (void)
{
	NihIoWatch *watch;
	int         ret = 0, fds[2];

	printf ("Testing nih_io_add_watch()\n");
	assert (pipe (fds) == 0);
	watch = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				  my_watcher, &ret);

	/* File descriptor should be that given */
	if (watch->fd != fds[0]) {
		printf ("BAD: file descriptor set incorrectly.\n");
		ret = 1;
	}

	/* Events mask should be that given */
	if (watch->events != NIH_IO_READ) {
		printf ("BAD: watcher events set incorrectly.\n");
		ret = 1;
	}

	/* Watcher function should be that given */
	if (watch->watcher != my_watcher) {
		printf ("BAD: watcher function set incorrectly.\n");
		ret = 1;
	}

	/* Watcher data should be that given */
	if (watch->data != &ret) {
		printf ("BAD: watcher data set incorrectly.\n");
		ret = 1;
	}

	nih_list_free (&watch->entry);

	close (fds[0]);
	close (fds[1]);

	return ret;
}


int
test_select_fds (void)
{
	NihIoWatch *watch1, *watch2, *watch3;
	fd_set      readfds, writefds, exceptfds;
	int         ret = 0, nfds, fds[2];

	printf ("Testing nih_io_select_fds()\n");
	assert (pipe (fds) == 0);
	watch1 = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				   my_watcher, &ret);
	watch2 = nih_io_add_watch (NULL, fds[1], NIH_IO_WRITE,
				   my_watcher, &ret);
	watch3 = nih_io_add_watch (NULL, fds[0], NIH_IO_EXCEPT,
				   my_watcher, &ret);

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);

	/* Should be highest fd plus one */
	if (nfds != MAX (fds[0], fds[1]) + 1) {
		printf ("BAD: nfds wasn't what we expected.\n");
		ret = 1;
	}

	/* First entry's file descriptor should be watched for read */
	if (! FD_ISSET (watch1->fd, &readfds)) {
		printf ("BAD: first file descriptor not watched.\n");
		ret = 1;
	}

	/* First entry's file descriptor should not be watched for write */
	if (FD_ISSET (watch1->fd, &writefds)) {
		printf ("BAD: first file descriptor watched unexpectedly.\n");
		ret = 1;
	}

	/* Second entry's file descriptor should be watched for write */
	if (! FD_ISSET (watch2->fd, &writefds)) {
		printf ("BAD: second file descriptor not watched.\n");
		ret = 1;
	}

	/* Second entry's file descriptor should not be watched for others */
	if (FD_ISSET (watch2->fd, &readfds)
	    || FD_ISSET (watch2->fd, &exceptfds)) {
		printf ("BAD: second file descriptor watched unexpectedly.\n");
		ret = 1;
	}

	/* Third entry's file descriptor should be watched for exception */
	if (! FD_ISSET (watch3->fd, &exceptfds)) {
		printf ("BAD: third file descriptor not watched.\n");
		ret = 1;
	}

	nih_list_free (&watch3->entry);
	nih_list_free (&watch2->entry);
	nih_list_free (&watch1->entry);

	close (fds[0]);
	close (fds[1]);

	return ret;
}

int
test_handle_fds (void)
{
	NihIoWatch    *watch1, *watch2, *watch3;
	fd_set         readfds, writefds, exceptfds;
	int            ret = 0, fds[2];

	printf ("Testing nih_io_handle_fds()\n");
	assert (pipe (fds) == 0);
	watch1 = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
				   my_watcher, &ret);
	watch2 = nih_io_add_watch (NULL, fds[1], NIH_IO_WRITE,
				   my_watcher, &ret);
	watch3 = nih_io_add_watch (NULL, fds[0], NIH_IO_EXCEPT,
				   my_watcher, &ret);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	FD_SET (watch1->fd, &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Watcher should be called just once */
	if (watcher_called != 1) {
		printf ("BAD: watcher called incorrect number of times.\n");
		ret = 1;
	}

	/* Last data should be pointer from watch */
	if (last_data != &ret) {
		printf ("BAD: last data pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Last watch should be first one */
	if (last_watch != watch1) {
		printf ("BAD: last watch wasn't what we expected.\n");
		ret = 1;
	}

	/* Last events should be those we set */
	if (last_events != NIH_IO_READ) {
		printf ("BAD: last events wasn't what we expected.\n");
		ret = 1;
	}


	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	FD_ZERO (&readfds);
	FD_SET (watch3->fd, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Watcher should be called just once */
	if (watcher_called != 1) {
		printf ("BAD: watcher called incorrect number of times.\n");
		ret = 1;
	}

	/* Last data should be pointer from watch */
	if (last_data != &ret) {
		printf ("BAD: last data pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Last watch should be third one */
	if (last_watch != watch3) {
		printf ("BAD: last watch wasn't what we expected.\n");
		ret = 1;
	}

	/* Last events should be those we set */
	if (last_events != NIH_IO_EXCEPT) {
		printf ("BAD: last events wasn't what we expected.\n");
		ret = 1;
	}


	watcher_called = 0;
	FD_ZERO (&exceptfds);
	FD_SET (watch2->fd, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Watcher should not be called */
	if (watcher_called != 0) {
		printf ("BAD: watcher called incorrect number of times.\n");
		ret = 1;
	}


	nih_list_free (&watch3->entry);
	nih_list_free (&watch2->entry);
	nih_list_free (&watch1->entry);

	close (fds[0]);
	close (fds[1]);

	return ret;
}


int
test_buffer_new (void)
{
	NihIoBuffer *buf;
	int          ret = 0;

	printf ("Testing nih_io_buffer_new()\n");
	buf = nih_io_buffer_new (NULL);

	/* Buffer should be NULL */
	if (buf->buf != NULL) {
		printf ("BAD: buffer pointer set incorrectly.\n");
		ret = 1;
	}

	/* Buffer size should be zero */
	if (buf->size != 0) {
		printf ("BAD: buffer size set incorrectly.\n");
		ret = 1;
	}

	/* Buffer length should be zero */
	if (buf->len != 0) {
		printf ("BAD: buffer length set incorrectly.\n");
		ret = 1;
	}

	/* Should be allocated with nih_alloc */
	if (nih_alloc_size (buf) != sizeof (NihIoBuffer)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_free (buf);

	return ret;
}

int
test_buffer_resize (void)
{
	NihIoBuffer *buf;
	int          ret = 0;

	printf ("Testing nih_io_buffer_resize()\n");
	buf = nih_io_buffer_new (NULL);


	printf ("...with empty buffer and half increase\n");
	nih_io_buffer_resize (buf, BUFSIZ / 2);

	/* Size should now be BUFSIZ */
	if (buf->size != BUFSIZ) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should be the same size */
	if (nih_alloc_size (buf->buf) != buf->size) {
		printf ("BAD: buffer allocation wasn't the same.\n");
		ret = 1;
	}

	/* Parent of the buffer should the struct */
	if (nih_alloc_parent (buf->buf) != buf) {
		printf ("BAD: nih_alloc parent wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with empty but alloc'd buffer and full increase\n");
	nih_io_buffer_resize (buf, BUFSIZ);

	/* Size should still be BUFSIZ (we didn't increase the length) */
	if (buf->size != BUFSIZ) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should be the same size */
	if (nih_alloc_size (buf->buf) != buf->size) {
		printf ("BAD: buffer allocation wasn't the same.\n");
		ret = 1;
	}


	printf ("...with empty but alloc'd buffer and larger increase\n");
	nih_io_buffer_resize (buf, BUFSIZ + BUFSIZ / 2);

	/* Size should now be twice BUFSIZ (still didn't increase length) */
	if (buf->size != BUFSIZ * 2) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should be the same size */
	if (nih_alloc_size (buf->buf) != buf->size) {
		printf ("BAD: buffer allocation wasn't the same.\n");
		ret = 1;
	}


	printf ("...with alloc'd buffer and no data\n");
	nih_io_buffer_resize (buf, 0);

	/* Size should now drop to zero again (still nothing in length) */
	if (buf->size != 0) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should have been freed */
	if (buf->buf != NULL) {
		printf ("BAD: buffer wasn't freed.\n");
		ret = 1;
	}


	printf ("...with part-full buffer and increase.\n");
	buf->len = BUFSIZ / 2;
	nih_io_buffer_resize (buf, BUFSIZ);

	/* Size should be twice BUFSIZ (enough for both bits) */
	if (buf->size != BUFSIZ * 2) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should be the same size */
	if (nih_alloc_size (buf->buf) != buf->size) {
		printf ("BAD: buffer allocation wasn't the same.\n");
		ret = 1;
	}


	printf ("...with no change.\n");
	buf->len = BUFSIZ + BUFSIZ / 2;
	nih_io_buffer_resize (buf, 80);

	/* Size should still be twice BUFSIZ (it'll still fit) */
	if (buf->size != BUFSIZ * 2) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should be the same size */
	if (nih_alloc_size (buf->buf) != buf->size) {
		printf ("BAD: buffer allocation wasn't the same.\n");
		ret = 1;
	}


	nih_free (buf);

	return ret;
}

int
test_buffer_pop (void)
{
	NihIoBuffer *buf;
	char        *str;
	int          ret = 0;

	printf ("Testing nih_io_buffer_pop()\n");
	buf = nih_io_buffer_new (NULL);
	nih_io_buffer_push (buf, "this is a test of the buffer code", 33);


	printf ("...with full buffer\n");
	str = nih_io_buffer_pop (NULL, buf, 14);

	/* String should be NULL terminated */
	if (str[14] != '\0') {
		printf ("BAD: return value wasn't NULL terminated.\n");
		ret = 1;
	}

	/* String returned should be data from the front */
	if (strcmp (str, "this is a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 15) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Buffer length should now be 19 */
	if (buf->len != 19) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should have been moved up */
	if (strncmp (buf->buf, " of the buffer code", 19)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with request to empty buffer\n");
	str = nih_io_buffer_pop (NULL, buf, 19);

	/* String should be NULL terminated */
	if (str[19] != '\0') {
		printf ("BAD: return value wasn't NULL terminated.\n");
		ret = 1;
	}

	/* String returned should be the data */
	if (strcmp (str, " of the buffer code")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 20) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Buffer length should now be zero */
	if (buf->len != 0) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer size should now be zero */
	if (buf->size != 0) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should have been freed */
	if (buf->buf != NULL) {
		printf ("BAD: buffer wasn't freed.\n");
		ret = 1;
	}

	nih_free (str);

	nih_free (buf);

	return 0;
}

int
test_buffer_push (void)
{
	NihIoBuffer *buf;
	int          ret = 0;

	printf ("Testing nih_io_buffer_push()\n");
	buf = nih_io_buffer_new (NULL);

	printf ("...with empty buffer\n");
	nih_io_buffer_push (buf, "test", 4);

	/* Buffer length should be four */
	if (buf->len != 4) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should contain our data */
	if (strncmp (buf->buf, "test", 4)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	/* Size should be BUFSIZ */
	if (buf->size != BUFSIZ) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should be the same size */
	if (nih_alloc_size (buf->buf) != buf->size) {
		printf ("BAD: buffer allocation wasn't the same.\n");
		ret = 1;
	}


	printf ("...with data in the buffer\n");
	nih_io_buffer_push (buf, "ing the buffer code", 14);

	/* Buffer length should be eighteen */
	if (buf->len != 18) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should contain our data (note we didn't copy it all) */
	if (strncmp (buf->buf, "testing the buffer", 18)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}


	nih_free (buf);

	return ret;
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


int
test_reopen (void)
{
	struct sigaction  oldact;
	NihIo            *io;
	int               ret = 0, fds[2], flags;

	printf ("Testing nih_io_reopen()\n");
	assert (pipe (fds) == 0);
	assert ((flags = fcntl (fds[0], F_GETFL)) >= 0);
	assert (! (flags & O_NONBLOCK));

	io = nih_io_reopen (NULL, fds[0], my_reader, my_close_handler, my_error_handler,
			    &ret);

	/* Reader should be set */
	if (io->reader != my_reader) {
		printf ("BAD: reader set incorrectly.\n");
		ret = 1;
	}

	/* Close handler should be set */
	if (io->close_handler != my_close_handler) {
		printf ("BAD: close handler set incorrectly.\n");
		ret = 1;
	}

	/* Error handler should be set */
	if (io->error_handler != my_error_handler) {
		printf ("BAD: error handler set incorrectly.\n");
		ret = 1;
	}

	/* Data should be set */
	if (io->data != &ret) {
		printf ("BAD: data pointer set incorrectly.\n");
		ret = 1;
	}

	/* Watch should be on file descriptor given */
	if (io->watch->fd != fds[0]) {
		printf ("BAD: file descriptor set incorrectly.\n");
		ret = 1;
	}

	/* Events should be read */
	if (io->watch->events != NIH_IO_READ) {
		printf ("BAD: watch events set incorrectly.\n");
		ret = 1;
	}

	/* SIGPIPE should be ignored */
	assert (sigaction (SIGPIPE, NULL, &oldact) == 0);
	if (oldact.sa_handler != SIG_IGN) {
		printf ("BAD: PIPE signal not ignored.\n");
		ret = 1;
	}

	/* Socket should be non-blocking */
	assert ((flags = fcntl (fds[0], F_GETFL)) >= 0);
	if (! (flags & O_NONBLOCK)) {
		printf ("BAD: file descriptor not set O_NONBLOCK.\n");
		ret = 1;
	}

	/* Should be allocated with nih_alloc */
	if (nih_alloc_size (io) != sizeof (NihIo)) {
		printf ("BAD: nih_alloc wasn't used.\n");
		ret = 1;
	}

	/* nih_alloc parent of watch should be io structure */
	if (nih_alloc_parent (io->watch) != io) {
		printf ("BAD: nih_alloc parent wasn't what we expected.\n");
		ret = 1;
	}

	/* nih_alloc parent of send_buf should be io structure */
	if (nih_alloc_parent (io->send_buf) != io) {
		printf ("BAD: nih_alloc parent wasn't what we expected.\n");
		ret = 1;
	}

	/* nih_alloc parent of recv_buf should be io structure */
	if (nih_alloc_parent (io->recv_buf) != io) {
		printf ("BAD: nih_alloc parent wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (io);

	close (fds[0]);
	close (fds[1]);

	return ret;
}

static int free_called;

static int
destructor_called (void *ptr)
{
	free_called++;

	return 0;
}

int
test_shutdown (void)
{
	NihIo  *io;
	fd_set  readfds, writefds, exceptfds;
	int     ret = 0, fds[2], flags;

	printf ("Testing nih_io_shutdown()\n");

	assert (pipe (fds) == 0);
	io = nih_io_reopen (NULL, fds[0], NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "some data", 9);
	nih_alloc_set_destructor (io, destructor_called);
	free_called = 0;
	nih_io_shutdown (io);

	/* Socket should have been marked as shutdown */
	if (! io->shutdown) {
		printf ("BAD: shutdown not set correctly.\n");
		ret = 1;
	}

	/* Descriptor should not yet have been closed */
	flags = fcntl (fds[0], F_GETFD);
	if (flags < 0) {
		printf ("BAD: file descriptor closed unexpectedly.\n");
		ret = 1;
	}

	/* Should not have been freed */
	if (free_called) {
		printf ("BAD: structure freed unexpectedly.\n");
		ret = 1;
	}


	/* Now handle the fds (which will take the data off) */
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Descriptor should have been closed */
	flags = fcntl (fds[0], F_GETFD);
	if ((flags != -1) || (errno != EBADF)) {
		printf ("BAD: file descriptor was not closed.\n");
		ret = 1;
	}

	/* Should have been freed */
	if (! free_called) {
		printf ("BAD: structure was not freed.\n");
		ret = 1;
	}

	close (fds[1]);

	return ret;
}

int
test_close (void)
{
	NihIo *io;
	int    ret = 0, fds[2], flags;

	printf ("Testing nih_io_close()\n");

	printf ("...with open file descriptor\n");
	assert (pipe (fds) == 0);
	io = nih_io_reopen (NULL, fds[0], NULL, NULL, my_error_handler, &ret);
	nih_alloc_set_destructor (io, destructor_called);
	free_called = 0;
	error_called = 0;
	nih_io_close (io);

	/* Error handler should not be called */
	if (error_called) {
		printf ("BAD: error handler called unexpectedly.\n");
		ret = 1;
	}

	/* Descriptor should have been closed */
	flags = fcntl (fds[0], F_GETFD);
	if ((flags != -1) || (errno != EBADF)) {
		printf ("BAD: file descriptor was not closed.\n");
		ret = 1;
	}

	/* Should have been freed */
	if (! free_called) {
		printf ("BAD: structure was not freed.\n");
		ret = 1;
	}

	close (fds[1]);


	printf ("...with closed file descriptor\n");
	assert (pipe (fds) == 0);
	io = nih_io_reopen (NULL, fds[0], NULL, NULL, my_error_handler, &ret);
	nih_alloc_set_destructor (io, destructor_called);
	free_called = 0;
	error_called = 0;
	last_data = NULL;
	last_error = NULL;
	close (fds[0]);
	nih_io_close (io);

	/* Error handler should have been called */
	if (! error_called) {
		printf ("BAD: error handler wasn't called.\n");
		ret = 1;
	}

	/* Error handler should have been passed data */
	if (last_data != &ret) {
		printf ("BAD: data pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Error should be EBADF */
	if (last_error->number != EBADF) {
		printf ("BAD: error wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been freed */
	if (! free_called) {
		printf ("BAD: structure was not freed.\n");
		ret = 1;
	}

	nih_free (last_error);


	close (fds[1]);

	return ret;
}


int
test_watcher (void)
{
	NihIo  *io;
	FILE   *output;
	char    text[80];
	fd_set  readfds, writefds, exceptfds;
	int     ret = 0, fds[2], flags;

	printf ("Testing nih_io_watcher()\n");

	printf ("...with data to read\n");
	assert (pipe (fds) == 0);
	io = nih_io_reopen (NULL, fds[0], my_reader, my_close_handler, my_error_handler,
			    &ret);

	assert (write (fds[1], "this is a test", 14) == 14);
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);
	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Our reader should have been called */
	if (! read_called) {
		printf ("BAD: reader wasn't called.\n");
		ret = 1;
	}

	/* Should have been passed the data pointer */
	if (last_data != &ret) {
		printf ("BAD: data pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the receive buffer */
	if (last_str != io->recv_buf->buf) {
		printf ("BAD: str pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the buffer length */
	if (last_len != io->recv_buf->len) {
		printf ("BAD: length argument wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have 14 bytes in it */
	if (io->recv_buf->len != 14) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have our string in it */
	if (strncmp (io->recv_buf->buf, "this is a test", 14)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}


	printf ("...with more data to read\n");
	assert (write (fds[1], " of the callback code", 19) == 19);
	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Our read function should have been called */
	if (! read_called) {
		printf ("BAD: reader wasn't called.\n");
		ret = 1;
	}

	/* Should have been passed the data pointer */
	if (last_data != &ret) {
		printf ("BAD: data pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the receive buffer */
	if (last_str != io->recv_buf->buf) {
		printf ("BAD: str pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the buffer length */
	if (last_len != io->recv_buf->len) {
		printf ("BAD: length argument wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have 33 bytes in it */
	if (io->recv_buf->len != 33) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have both strings in it */
	if (strncmp (io->recv_buf->buf, "this is a test of the callback code",
		     33)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}


	printf ("...with remote end closed\n");
	assert (close (fds[1]) == 0);
	read_called = 0;
	close_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Our read function should have been called */
	if (! read_called) {
		printf ("BAD: reader wasn't called.\n");
		ret = 1;
	}

	/* Should have been passed the receive buffer */
	if (last_str != io->recv_buf->buf) {
		printf ("BAD: str pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the buffer length */
	if (last_len != io->recv_buf->len) {
		printf ("BAD: length argument wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have 33 bytes in it */
	if (io->recv_buf->len != 33) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have both strings in it */
	if (strncmp (io->recv_buf->buf, "this is a test of the callback code",
		     33)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	/* Close handler should have also been called */
	if (! close_called) {
		printf ("BAD: close handler wasn't called.\n");
		ret = 1;
	}

	/* Should have been passed the data pointer */
	if (last_data != &ret) {
		printf ("BAD: data pointer wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with local end closed\n");
	assert (close (fds[0]) == 0);
	read_called = 0;
	error_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Our read function should have been called */
	if (! read_called) {
		printf ("BAD: reader wasn't called.\n");
		ret = 1;
	}

	/* Should have been passed the receive buffer */
	if (last_str != io->recv_buf->buf) {
		printf ("BAD: str pointer wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the buffer length */
	if (last_len != io->recv_buf->len) {
		printf ("BAD: length argument wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have 33 bytes in it */
	if (io->recv_buf->len != 33) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Receive buffer should have both strings in it */
	if (strncmp (io->recv_buf->buf, "this is a test of the callback code",
		     33)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	/* Error handler should have also been called */
	if (! error_called) {
		printf ("BAD: error handler wasn't called.\n");
		ret = 1;
	}

	/* Error should be EBADF */
	if (last_error->number != EBADF) {
		printf ("BAD: error wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the data pointer */
	if (last_data != &ret) {
		printf ("BAD: data pointer wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (last_error);

	close (fds[0]);
	nih_free (io);


	printf ("...with no close handler\n");
	assert (pipe (fds) == 0);
	FD_ZERO (&readfds);
	io = nih_io_reopen (NULL, fds[0], my_reader, NULL, NULL, &ret);
	nih_alloc_set_destructor (io, destructor_called);
	assert (close (fds[1]) == 0);
	free_called = 0;
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Local end should have been closed */
	flags = fcntl (fds[0], F_GETFD);
	if ((flags != -1) || (errno != EBADF)) {
		printf ("BAD: file descriptor was not closed.\n");
		ret = 1;
	}

	/* Structure should have been freed */
	if (! free_called) {
		printf ("BAD: structure was not freed.\n");
		ret = 1;
	}


	printf ("...with no error handler\n");
	assert (pipe (fds) == 0);
	FD_ZERO (&readfds);
	io = nih_io_reopen (NULL, fds[0], my_reader, NULL, NULL, &ret);
	nih_alloc_set_destructor (io, destructor_called);
	assert (close (fds[0]) == 0);
	assert (close (fds[1]) == 0);
	free_called = 0;
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Local end should have been closed */
	flags = fcntl (fds[0], F_GETFD);
	if ((flags != -1) || (errno != EBADF)) {
		printf ("BAD: file descriptor was not closed.\n");
		ret = 1;
	}

	/* Structure should have been freed */
	if (! free_called) {
		printf ("BAD: structure was not freed.\n");
		ret = 1;
	}


	printf ("...with data to write\n");
	output = tmpfile ();
	FD_ZERO (&readfds);
	io = nih_io_reopen (NULL, fileno (output), NULL,
			    my_close_handler, my_error_handler, &ret);
	nih_io_printf (io, "this is a test\n");
	FD_SET (fileno (output), &writefds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	fseek (output, 0, SEEK_SET);

	/* Data should have been written */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "this is a test\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should no longer be watching for write */
	if (io->watch->events & NIH_IO_WRITE) {
		printf ("BAD: watch watching for write unexpectedly.\n");
		ret = 1;
	}


	printf ("...with more data to write\n");
	nih_io_printf (io, "so is this\n");
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);
	fseek (output, 0, SEEK_SET);

	/* Data should have been written */
	fgets (text, sizeof (text), output);
	if (strcmp (text, "this is a test\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	fgets (text, sizeof (text), output);
	if (strcmp (text, "so is this\n")) {
		printf ("BAD: output wasn't what we expected.\n");
		ret = 1;
	}

	/* Should no longer be watching for write */
	if (io->watch->events & NIH_IO_WRITE) {
		printf ("BAD: watch watching for write unexpectedly.\n");
		ret = 1;
	}


	printf ("...with closed file\n");
	fclose (output);
	error_called = 0;
	last_data = NULL;
	last_error = NULL;
	nih_io_printf (io, "this write fails\n");
	FD_SET (fds[0], &readfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Error handler should have been called */
	if (! error_called) {
		printf ("BAD: error handler was not called.\n");
		ret = 1;
	}

	/* Error should be EBADF */
	if (last_error->number != EBADF) {
		printf ("BAD: error wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been passed the data pointer */
	if (last_data != &ret) {
		printf ("BAD: data pointer wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (last_error);

	nih_free (io);


	return ret;
}


int
test_read (void)
{
	NihIo *io;
	char  *str;
	int    ret = 0;

	printf ("Testing nih_io_read()\n");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "this is a test of the io code", 29);


	printf ("...with full buffer\n");
	str = nih_io_read (NULL, io, 14);

	/* String should be NULL terminated */
	if (str[14] != '\0') {
		printf ("BAD: return value wasn't NULL terminated.\n");
		ret = 1;
	}

	/* String returned should be data from the front */
	if (strcmp (str, "this is a test")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 15) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Buffer length should now be 15 */
	if (io->recv_buf->len != 15) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should have been moved up */
	if (strncmp (io->recv_buf->buf, " of the io code", 15)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with request to empty buffer\n");
	str = nih_io_read (NULL, io, 15);

	/* String should be NULL terminated */
	if (str[15] != '\0') {
		printf ("BAD: return value wasn't NULL terminated.\n");
		ret = 1;
	}

	/* String returned should be the data */
	if (strcmp (str, " of the io code")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 16) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Buffer length should now be zero */
	if (io->recv_buf->len != 0) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer size should now be zero */
	if (io->recv_buf->size != 0) {
		printf ("BAD: buffer size wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should have been freed */
	if (io->recv_buf->buf != NULL) {
		printf ("BAD: buffer wasn't freed.\n");
		ret = 1;
	}

	nih_free (str);

	nih_free (io);

	return 0;
}

int
test_write (void)
{
	NihIo *io;
	int    ret = 0;

	printf ("Testing nih_io_write()\n");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);

	printf ("...with empty buffer\n");
	nih_io_write (io, "test", 4);

	/* Buffer length should be four */
	if (io->send_buf->len != 4) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should contain our data */
	if (strncmp (io->send_buf->buf, "test", 4)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	/* Watch should now be flagged for write */
	if (! (io->watch->events & NIH_IO_WRITE)) {
		printf ("BAD: watch not looking for writability.\n");
		ret = 1;
	}


	printf ("...with data in the buffer\n");
	nih_io_write (io, "ing the io code", 10);

	/* Buffer length should be eighteen */
	if (io->send_buf->len != 14) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should contain our data (note we didn't copy it all) */
	if (strncmp (io->send_buf->buf, "testing the io", 14)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}


	nih_free (io);

	return ret;
}


int
test_get (void)
{
	NihIo *io;
	char  *str;
	int    ret = 0;

	printf ("Testing nih_io_get()\n");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);
	nih_io_buffer_push (io->recv_buf, "some data\n", 10);
	nih_io_buffer_push (io->recv_buf, "and another line\n", 17);
	nih_io_buffer_push (io->recv_buf, "incomplete", 10);

	printf ("...with full buffer\n");
	str = nih_io_get (NULL, io, "\n");

	/* String returned should be the first line without the newline */
	if (strcmp (str, "some data")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 10) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with part-full buffer\n");
	str = nih_io_get (NULL, io, "\n");

	/* String returned should be the second line without the newline */
	if (strcmp (str, "and another line")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 17) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_free (str);


	printf ("...with incomplete line in buffer\n");
	str = nih_io_get (NULL, io, "\n");

	/* NULL should be returned */
	if (str != NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}


	printf ("...with null-terminated string in buffer\n");
	nih_io_buffer_push (io->recv_buf, "\0", 1);
	str = nih_io_get (NULL, io, "\n");

	/* String should be returned */
	if (strcmp (str, "incomplete")) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have been allocated with nih_alloc */
	if (nih_alloc_size (str) != 11) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Buffer should be empty */
	if (io->recv_buf->len != 0) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (str);

	nih_free (io);

	return ret;
}

int
test_printf (void)
{
	NihIo *io;
	int    ret = 0;

	printf ("Testing nih_io_printf()\n");
	io = nih_io_reopen (NULL, 0, NULL, NULL, NULL, NULL);

	printf ("...with empty buffer\n");
	nih_io_printf (io, "this is a %d %s test\n", 4, "format");

	/* Buffer length should be 24 */
	if (io->send_buf->len != 24) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should contain formatted data */
	if (strncmp (io->send_buf->buf, "this is a 4 format test\n", 24)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	/* Watch should now be flagged for write */
	if (! (io->watch->events & NIH_IO_WRITE)) {
		printf ("BAD: watch not looking for writability.\n");
		ret = 1;
	}


	printf ("...with data in the buffer\n");
	nih_io_printf (io, "and this is %s line\n", "another");

	/* Buffer length should be 49 */
	if (io->send_buf->len != 49) {
		printf ("BAD: buffer length wasn't what we expected.\n");
		ret = 1;
	}

	/* Buffer should contain our data */
	if (strncmp (io->send_buf->buf,
		     "this is a 4 format test\nand this is another line\n",
		     49)) {
		printf ("BAD: buffer contents weren't what we expected.\n");
		ret = 1;
	}

	nih_free (io);

	return ret;
}

int
test_set_nonblock (void)
{
	int ret = 0, fds[2], flags;

	printf ("Testing nih_io_set_nonblock()\n");
	assert (pipe (fds) == 0);
	assert ((flags = fcntl (fds[0], F_GETFL)) >= 0);
	assert (! (flags & O_NONBLOCK));

	nih_io_set_nonblock (fds[0]);

	assert ((flags = fcntl (fds[0], F_GETFL)) >= 0);

	/* O_NONBLOCK should be set */
	if (! (flags & O_NONBLOCK)) {
		printf ("BAD: expected flag was not set.\n");
		ret = 1;
	}

	close (fds[0]);
	close (fds[1]);

	return ret;
}

int
test_set_cloexec (void)
{
	int ret = 0, fds[2], flags;

	printf ("Testing nih_io_set_cloexec()\n");
	assert (pipe (fds) == 0);
	assert ((flags = fcntl (fds[0], F_GETFD)) >= 0);
	assert (! (flags & FD_CLOEXEC));

	nih_io_set_cloexec (fds[0]);

	assert ((flags = fcntl (fds[0], F_GETFD)) >= 0);

	/* FD_CLOEXEC should be set */
	if (! (flags & FD_CLOEXEC)) {
		printf ("BAD: expected flag was not set.\n");
		ret = 1;
	}

	close (fds[0]);
	close (fds[1]);

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_add_watch ();
	ret |= test_select_fds ();
	ret |= test_handle_fds ();
	ret |= test_buffer_new ();
	ret |= test_buffer_resize ();
	ret |= test_buffer_push ();
	ret |= test_buffer_pop ();
	ret |= test_reopen ();
	ret |= test_shutdown ();
	ret |= test_close ();
	ret |= test_watcher ();
	ret |= test_read ();
	ret |= test_write ();
	ret |= test_get ();
	ret |= test_printf ();
	ret |= test_set_nonblock ();
	ret |= test_set_cloexec ();

	return ret;
}
