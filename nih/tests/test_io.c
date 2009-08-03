/* libnih
 *
 * test_io.c - test suite for nih/io.c
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

#include <nih/test.h>

#include <linux/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/ip.h>

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
#include <nih/errors.h>


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
	fd_set      readfds, writefds, exceptfds;
	int         nfds, fds[2];

	/* Check that we can add a watch on a file descriptor and that the
	 * structure is properly filled in and placed in a list.
	 */
	TEST_FUNCTION ("nih_io_add_watch");
	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);

	TEST_ALLOC_FAIL {
		assert0 (pipe (fds));
		watch = nih_io_add_watch (NULL, fds[0], NIH_IO_READ,
					  my_watcher, &watch);

		if (test_alloc_failed) {
			TEST_EQ_P (watch, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (watch, sizeof (NihIoWatch));
		TEST_EQ (watch->fd, fds[0]);
		TEST_EQ (watch->events, NIH_IO_READ);
		TEST_EQ_P (watch->watcher, my_watcher);
		TEST_EQ_P (watch->data, &watch);

		nih_free (watch);

		close (fds[0]);
		close (fds[1]);
	}
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
	assert0 (pipe (fds));
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

	TEST_EQ (nfds, nih_max (fds[0], fds[1]) + 1);
	TEST_TRUE (FD_ISSET (fds[0], &readfds));
	TEST_FALSE (FD_ISSET (fds[0], &writefds));
	TEST_TRUE (FD_ISSET (fds[0], &exceptfds));
	TEST_FALSE (FD_ISSET (fds[1], &readfds));
	TEST_TRUE (FD_ISSET (fds[1], &writefds));
	TEST_FALSE (FD_ISSET (fds[1], &exceptfds));

	nih_free (watch1);
	nih_free (watch2);
	nih_free (watch3);

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
	assert0 (pipe (fds));
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


	nih_free (watch1);
	nih_free (watch2);
	nih_free (watch3);

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
	TEST_ALLOC_FAIL {
		buf = nih_io_buffer_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (buf, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (buf, sizeof (NihIoBuffer));
		TEST_EQ_P (buf->buf, NULL);
		TEST_EQ (buf->size, 0);
		TEST_EQ (buf->len, 0);

		nih_free (buf);
	}
}

void
test_buffer_resize (void)
{
	NihIoBuffer *buf;
	int          ret;

	TEST_FUNCTION ("nih_io_buffer_resize");

	/* Check that we can resize a NULL buffer; we ask for half a page
	 * and expect to get a full page allocated as a child of the buffer
	 * itself.
	 */
	TEST_FEATURE ("with empty buffer and half increase");
	buf = nih_io_buffer_new (NULL);

	TEST_ALLOC_FAIL {
		buf->size = 0;
		ret = nih_io_buffer_resize (buf, BUFSIZ / 2);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (buf->buf, buf);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
		TEST_EQ (buf->size, BUFSIZ);
		TEST_EQ (buf->len, 0);
	}


	/* Check that we can increase the size by a full page, and not
	 * have anything change because there's no space used yet.
	 */
	TEST_FEATURE ("with empty but alloc'd buffer and full increase");
	TEST_ALLOC_FAIL {
		buf->size = BUFSIZ;
		ret = nih_io_buffer_resize (buf, BUFSIZ);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
		TEST_EQ (buf->size, BUFSIZ);
	}


	/* Check that we can increase the size beyond a full page, and
	 * get another page of allocated space.
	 */
	TEST_FEATURE ("with empty but alloc'd buffer and larger increase");
	TEST_ALLOC_FAIL {
		buf->size = BUFSIZ;
		ret = nih_io_buffer_resize (buf, BUFSIZ + BUFSIZ / 2);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
		TEST_EQ (buf->size, BUFSIZ * 2);
	}


	/* Check that we can drop the size of an allocated but empty buffer
	 * back to zero and have the buffer freed.
	 */
	TEST_FEATURE ("with alloc'd buffer and no data");
	TEST_ALLOC_FAIL {
		buf->size = BUFSIZ * 2;
		ret = nih_io_buffer_resize (buf, 0);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (buf->size, 0);
		TEST_EQ_P (buf->buf, NULL);
	}


	/* Check that asking for a page more space when we claim to be
	 * using half a page gives us a full two pages of space.
	 */
	TEST_FEATURE ("with part-full buffer and increase");
	TEST_ALLOC_FAIL {
		buf->size = 0;
		buf->len = BUFSIZ / 2;
		ret = nih_io_buffer_resize (buf, BUFSIZ);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
		TEST_EQ (buf->size, BUFSIZ * 2);
		TEST_EQ (buf->len, BUFSIZ / 2);
	}


	/* Check that asking for an increase smaller than the difference
	 * between the buffer size and length has no effect.
	 */
	TEST_FEATURE ("with no change");
	TEST_ALLOC_FAIL {
		buf->size = BUFSIZ * 2;
		buf->len = BUFSIZ + BUFSIZ / 2;
		ret = nih_io_buffer_resize (buf, 80);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ * 2);
		TEST_EQ (buf->size, BUFSIZ * 2);
		TEST_EQ (buf->len, BUFSIZ + BUFSIZ / 2);
	}

	nih_free (buf);
}

void
test_buffer_pop (void)
{
	NihIoBuffer *buf;
	char        *str;
	size_t       len;

	TEST_FUNCTION ("nih_io_buffer_pop");
	buf = nih_io_buffer_new (NULL);
	assert0 (nih_io_buffer_push (buf,
				     "this is a test of the buffer code", 33));


	/* Check that we can pop some bytes out of a buffer, and have a
	 * NULL-terminated string returned that is allocated with nih_alloc.
	 * The buffer should be shrunk appropriately and moved up.
	 */
	TEST_FEATURE ("with full buffer");
	TEST_ALLOC_FAIL {
		len = 14;
		str = nih_io_buffer_pop (NULL, buf, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (buf->len, 19);
			TEST_EQ_MEM (buf->buf, " of the buffer code", 19);
			continue;
		}

		TEST_EQ (len, 14);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ (str[14], '\0');
		TEST_EQ_STR (str, "this is a test");

		TEST_EQ (buf->len, 19);
		TEST_EQ_MEM (buf->buf, " of the buffer code", 19);

		nih_free (str);
	}


	/* Check that we can empty the buffer and the buffer is freed. */
	TEST_FEATURE ("with request to empty buffer");
	TEST_ALLOC_FAIL {
		len = 19;
		str = nih_io_buffer_pop (NULL, buf, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (buf->len, 0);
			TEST_EQ (buf->size, 0);
			TEST_EQ_P (buf->buf, NULL);
			continue;
		}

		TEST_EQ (len, 19);
		TEST_ALLOC_SIZE (str, 20);
		TEST_EQ (str[19], '\0');
		TEST_EQ_STR (str, " of the buffer code");

		TEST_EQ (buf->len, 0);
		TEST_EQ (buf->size, 0);
		TEST_EQ_P (buf->buf, NULL);

		nih_free (str);
	}


	/* Check that we can request more data than is in the buffer.
	 * We should get everything's there, and len should be updated to
	 * indicate the shortfall.
	 */
	TEST_FEATURE ("with request for more than buffer size");
	assert0 (nih_io_buffer_push (buf, "another test", 12));
	TEST_ALLOC_FAIL {
		len = 20;
		str = nih_io_buffer_pop (NULL, buf, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (buf->len, 0);
			TEST_EQ (buf->size, 0);
			TEST_EQ_P (buf->buf, NULL);
			continue;
		}

		TEST_EQ (len, 12);
		TEST_ALLOC_SIZE (str, 13);
		TEST_EQ (str[12], '\0');
		TEST_EQ_STR (str, "another test");

		TEST_EQ (buf->len, 0);
		TEST_EQ (buf->size, 0);
		TEST_EQ_P (buf->buf, NULL);

		nih_free (str);
	}


	nih_free (buf);
}

void
test_buffer_shrink (void)
{
	NihIoBuffer *buf;

	TEST_FUNCTION ("nih_io_buffer_shrink");
	buf = nih_io_buffer_new (NULL);
	assert0 (nih_io_buffer_push (buf,
				     "this is a test of the buffer code", 33));


	/* Check that we can shrink the buffer by a small number of bytes. */
	TEST_FEATURE ("with full buffer");
	TEST_ALLOC_FAIL {
		nih_io_buffer_shrink (buf, 14);

		TEST_EQ (buf->len, 19);
		TEST_EQ_MEM (buf->buf, " of the buffer code", 19);
	}


	/* Check that we can empty the buffer and the buffer is freed. */
	TEST_FEATURE ("with request to empty buffer");
	TEST_ALLOC_FAIL {
		nih_io_buffer_shrink (buf, 19);

		TEST_EQ (buf->len, 0);
		TEST_EQ (buf->size, 0);
		TEST_EQ_P (buf->buf, NULL);
	}


	/* Check that we can shrink the buffer by more bytes than its length
	 * and just end up freeing it.
	 */
	TEST_FEATURE ("with request larger than buffer size");
	assert0 (nih_io_buffer_push (buf, "another test", 12));
	TEST_ALLOC_FAIL {
		nih_io_buffer_shrink (buf, 20);

		TEST_EQ (buf->len, 0);
		TEST_EQ (buf->size, 0);
		TEST_EQ_P (buf->buf, NULL);
	}

	nih_free (buf);
}

void
test_buffer_push (void)
{
	NihIoBuffer *buf;
	int          ret;

	TEST_FUNCTION ("nih_io_buffer_push");
	buf = nih_io_buffer_new (NULL);

	/* Check that we can push data into an empty buffer, which will
	 * store it in the buffer.
	 */
	TEST_FEATURE ("with empty buffer");
	TEST_ALLOC_FAIL {
		buf->len = 0;
		buf->size = 0;
		ret = nih_io_buffer_push (buf, "test", 4);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
		TEST_EQ (buf->size, BUFSIZ);
		TEST_EQ (buf->len, 4);
		TEST_EQ_MEM (buf->buf, "test", 4);
	}


	/* Check that we can push more data into that buffer, which will
	 * append it to the data already there.
	 */
	TEST_FEATURE ("with data in the buffer");
	TEST_ALLOC_FAIL {
		buf->len = 4;
		buf->size = BUFSIZ;
		ret = nih_io_buffer_push (buf, "ing the buffer code", 14);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (buf->buf, BUFSIZ);
		TEST_EQ (buf->size, BUFSIZ);
		TEST_EQ (buf->len, 18);
		TEST_EQ_MEM (buf->buf, "testing the buffer code", 18);
	}

	nih_free (buf);
}


void
test_message_new (void)
{
	NihIoMessage *msg;

	/* Check that we can create a new empty message, that doesn't appear
	 * in any list and with the structure and msghdr members correct.
	 */
	TEST_FUNCTION ("nih_io_message_new");
	TEST_ALLOC_FAIL {
		msg = nih_io_message_new (NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (msg, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_LIST_EMPTY (&msg->entry);
		TEST_EQ_P (msg->addr, NULL);
		TEST_EQ (msg->addrlen, 0);
		TEST_ALLOC_SIZE (msg->data, sizeof (NihIoBuffer));
		TEST_ALLOC_PARENT (msg->data, msg);
		TEST_ALLOC_SIZE (msg->control, sizeof (struct cmsghdr *));
		TEST_ALLOC_PARENT (msg->control, msg);
		TEST_EQ_P (msg->control[0], NULL);

		nih_free (msg);
	}
}

void
test_message_add_control (void)
{
	NihIoMessage  *msg;
	struct ucred   cred;
	int            ret, value;

	TEST_FUNCTION ("nih_io_message_add_control");
	test_alloc_failed = 0;
	msg = nih_io_message_new (NULL);

	/* Check that we can add a control message header to a message that
	 * doesn't yet have one.  The array should be increased in size and
	 * the messages should be children of it underneath.
	 */
	TEST_FEATURE ("with empty message");
	TEST_ALLOC_FAIL {
		value = 0;
		ret = nih_io_message_add_control (msg, SOL_SOCKET, SCM_RIGHTS,
						  sizeof (int), &value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_PARENT (msg->control, msg);
		TEST_ALLOC_SIZE (msg->control, sizeof (struct cmsghdr *) * 2);

		TEST_ALLOC_PARENT (msg->control[0], msg->control);
		TEST_ALLOC_SIZE (msg->control[0], CMSG_SPACE (sizeof (int)));

		TEST_EQ (msg->control[0]->cmsg_level, SOL_SOCKET);
		TEST_EQ (msg->control[0]->cmsg_type, SCM_RIGHTS);
		TEST_EQ (msg->control[0]->cmsg_len, CMSG_LEN (sizeof (int)));
		TEST_EQ_MEM (CMSG_DATA (msg->control[0]), &value,
			     sizeof (int));

		TEST_EQ_P (msg->control[1], NULL);
	}


	/* Check that we can append more control data onto the end of an
	 * existing message.  The array should include both messages.
	 */
	TEST_FEATURE ("with existing control data");
	TEST_ALLOC_FAIL {
		cred.pid = cred.uid = cred.gid = 1;
		ret = nih_io_message_add_control (msg, SOL_SOCKET,
						  SCM_CREDENTIALS,
						  sizeof (cred), &cred);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_ALLOC_PARENT (msg->control, msg);
		TEST_ALLOC_SIZE (msg->control, sizeof (struct cmsghdr *) * 3);

		TEST_ALLOC_PARENT (msg->control[0], msg->control);
		TEST_ALLOC_SIZE (msg->control[0], CMSG_SPACE (sizeof (int)));

		TEST_EQ (msg->control[0]->cmsg_level, SOL_SOCKET);
		TEST_EQ (msg->control[0]->cmsg_type, SCM_RIGHTS);
		TEST_EQ (msg->control[0]->cmsg_len, CMSG_LEN (sizeof (int)));
		TEST_EQ_MEM (CMSG_DATA (msg->control[0]), &value, sizeof (int));

		TEST_ALLOC_PARENT (msg->control[1], msg->control);
		TEST_ALLOC_SIZE (msg->control[1], CMSG_SPACE (sizeof (cred)));

		TEST_EQ (msg->control[1]->cmsg_level, SOL_SOCKET);
		TEST_EQ (msg->control[1]->cmsg_type, SCM_CREDENTIALS);
		TEST_EQ (msg->control[1]->cmsg_len, CMSG_LEN (sizeof (cred)));
		TEST_EQ_MEM (CMSG_DATA (msg->control[1]), &cred, sizeof (cred));

		TEST_EQ_P (msg->control[2], NULL);
	}

	nih_free (msg);
}

void
test_message_recv (void)
{
	NihError           *err;
	NihIoMessage       *msg;
	struct sockaddr_un  addr0, addr1;
	size_t              addr0len, addr1len, len;
	struct msghdr       msghdr;
	struct iovec        iov[1];
	char                buf[BUFSIZ * 2], cbuf[CMSG_SPACE(sizeof (int))];
	struct cmsghdr     *cmsg;
	int                 fds[2], *fdptr;

	TEST_FUNCTION ("nih_io_message_recv");
	nih_error_init ();

	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);

	msghdr.msg_name = NULL;
	msghdr.msg_namelen = 0;
	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = NULL;
	msghdr.msg_controllen = 0;
	msghdr.msg_flags = 0;

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof (buf);

	/* Check that we can receive a message from a socket with just
	 * text, and no control data.  The message structure should be
	 * allocated and filled properly.  If we run out of memory, NULL
	 * should be returned, and the message should be left in the socket.
	 */
	TEST_FEATURE ("with no control data");
	TEST_ALLOC_FAIL {
		memcpy (buf, "test", 4);
		iov[0].iov_len = 4;

		sendmsg (fds[0], &msghdr, 0);

		len = 0;
		msg = nih_io_message_recv (NULL, fds[1], &len);

		if (test_alloc_failed) {
			TEST_EQ_P (msg, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ (read (fds[1], buf, sizeof (buf)), 4);

			continue;
		}

		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_LIST_EMPTY (&msg->entry);

		TEST_EQ (len, 4);
		TEST_EQ (msg->data->len, 4);
		TEST_EQ_MEM (msg->data->buf, "test", 4);

		nih_free (msg);
	}


	/* Check that we can receive a message that contains control data,
	 * and that it's put in the structure.
	 */
	TEST_FEATURE ("with control data");
	TEST_ALLOC_FAIL {
		msghdr.msg_control = cbuf;
		msghdr.msg_controllen = sizeof (cbuf);

		cmsg = CMSG_FIRSTHDR (&msghdr);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN (sizeof (int));

		fdptr = (int *)CMSG_DATA (cmsg);
		memcpy (fdptr, &fds[0], sizeof (int));

		msghdr.msg_controllen = cmsg->cmsg_len;

		sendmsg (fds[0], &msghdr, 0);

		len = 0;
		msg = nih_io_message_recv (NULL, fds[1], &len);

		/* 8th alloc onwards is control data, and we mandate that
		 * always succeeds.
		 */
		if (test_alloc_failed && (test_alloc_failed < 8)) {
			TEST_EQ_P (msg, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ (read (fds[1], buf, sizeof (buf)), 4);

			continue;
		}

		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_LIST_EMPTY (&msg->entry);

		TEST_EQ (len, 4);
		TEST_EQ (msg->data->len, 4);
		TEST_EQ_MEM (msg->data->buf, "test", 4);

		TEST_ALLOC_SIZE (msg->control, sizeof (struct cmsghdr *) * 2);
		TEST_ALLOC_PARENT (msg->control, msg);

		TEST_ALLOC_SIZE (msg->control[0], CMSG_SPACE (sizeof (int)));
		TEST_ALLOC_PARENT (msg->control[0], msg->control);

		TEST_EQ (msg->control[0]->cmsg_level, SOL_SOCKET);
		TEST_EQ (msg->control[0]->cmsg_type, SCM_RIGHTS);
		TEST_EQ (msg->control[0]->cmsg_len, CMSG_LEN (sizeof (int)));

		TEST_EQ_P (msg->control[1], NULL);

		nih_free (msg);

		msghdr.msg_control = NULL;
		msghdr.msg_controllen = 0;
	}


	/* Check that we can get messages larger than the usual buffer size */
	TEST_FEATURE ("with message that would be truncated");
	TEST_ALLOC_FAIL {
		memset (buf, ' ', BUFSIZ * 2);
		iov[0].iov_len = BUFSIZ * 2;

		sendmsg (fds[0], &msghdr, 0);

		len = 0;
		msg = nih_io_message_recv (NULL, fds[1], &len);

		if (test_alloc_failed) {
			TEST_EQ_P (msg, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ (read (fds[1], buf, sizeof (buf)), BUFSIZ * 2);

			continue;
		}

		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_LIST_EMPTY (&msg->entry);

		TEST_EQ (len, BUFSIZ * 2);
		TEST_EQ (msg->data->len, BUFSIZ * 2);

		nih_free (msg);
	}


	/* Check that we can receive a message from a non-specific source
	 * over an unconnected socket.
	 */
	TEST_FEATURE ("with unconnected AF_UNIX sockets");
	addr0.sun_family = AF_UNIX;
	addr0.sun_path[0] = '\0';

	addr0len = offsetof (struct sockaddr_un, sun_path) + 1;
	addr0len += snprintf (addr0.sun_path + 1,
			      sizeof (addr0.sun_path) - 1,
			      "/com/netsplit/libnih/test_io/%d.0",
			      getpid ());

	fds[0] = socket (PF_UNIX, SOCK_DGRAM, 0);
	bind (fds[0], (struct sockaddr *)&addr0, addr0len);

	addr1.sun_family = AF_UNIX;
	addr1.sun_path[0] = '\0';

	addr1len = offsetof (struct sockaddr_un, sun_path) + 1;
	addr1len += snprintf (addr1.sun_path + 1,
			      sizeof (addr1.sun_path) - 1,
			      "/com/netsplit/libnih/test_io/%d.1",
			      getpid ());

	fds[1] = socket (PF_UNIX, SOCK_DGRAM, 0);
	bind (fds[1], (struct sockaddr *)&addr1, addr1len);

	msghdr.msg_name = (struct sockaddr *)&addr1;
	msghdr.msg_namelen = addr1len;

	memcpy (buf, "test", 4);
	iov[0].iov_len = 4;

	TEST_ALLOC_FAIL {
		sendmsg (fds[0], &msghdr, 0);

		len = 0;
		msg = nih_io_message_recv (NULL, fds[1], &len);

		if (test_alloc_failed) {
			TEST_EQ_P (msg, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			TEST_EQ (read (fds[1], buf, sizeof (buf)), 4);

			continue;
		}

		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_LIST_EMPTY (&msg->entry);

		TEST_EQ (msg->data->len, 4);
		TEST_EQ_MEM (msg->data->buf, "test", 4);

		TEST_EQ (msg->addrlen, addr0len);
		TEST_EQ (msg->addr->sa_family, PF_UNIX);
		TEST_EQ_MEM (((struct sockaddr_un *)msg->addr)->sun_path,
			     addr0.sun_path,
			     addr0len - offsetof (struct sockaddr_un,
						  sun_path));

		nih_free (msg);
	}

	close (fds[0]);
	close (fds[1]);


	/* Check that we get an error if the socket is closed.
	 */
	TEST_FEATURE ("with closed socket");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		len = 0;
		msg = nih_io_message_recv (NULL, fds[1], &len);

		TEST_EQ_P (msg, NULL);

		err = nih_error_get ();
		if (test_alloc_failed && (test_alloc_failed < 7)) {
			TEST_EQ (err->number, ENOMEM);
		} else {
			TEST_EQ (err->number, EBADF);
		}
		nih_free (err);
	}
	nih_error_pop_context ();
}

void
test_message_send (void)
{
	NihError           *err;
	NihIoMessage       *msg;
	struct sockaddr_un  addr;
	size_t              addrlen;
	struct msghdr       msghdr;
	struct iovec        iov[1];
	char                buf[BUFSIZ], cbuf[CMSG_SPACE(sizeof (int))];
	struct cmsghdr     *cmsg;
	ssize_t             len, ret;
	int                 fds[2];

	TEST_FUNCTION ("nih_io_message_send");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);

	msghdr.msg_name = NULL;
	msghdr.msg_namelen = 0;
	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = NULL;
	msghdr.msg_controllen = 0;
	msghdr.msg_flags = 0;

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof (buf);

	/* Check that we can send a message down a socket with just the
	 * ordinary text, and no control data.
	 */
	TEST_FEATURE ("with no control data");
	msg = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg->data, "test", 4));

	TEST_ALLOC_FAIL {
		ret = nih_io_message_send (msg, fds[0]);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_EQ (ret, 4);

		len = recvmsg (fds[1], &msghdr, 0);

		TEST_EQ (len, 4);
		TEST_EQ_MEM (buf, "test", 4);
	}


	/* Check that we can include control message information in the
	 * message, and have it come out the other end.
	 */
	TEST_FEATURE ("with control data");
	assert0 (nih_io_message_add_control (msg, SOL_SOCKET, SCM_RIGHTS,
					     sizeof (int), &fds[0]));

	TEST_ALLOC_FAIL {
		ret = nih_io_message_send (msg, fds[0]);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_EQ (ret, 4);

		msghdr.msg_control = cbuf;
		msghdr.msg_controllen = sizeof (cbuf);

		len = recvmsg (fds[1], &msghdr, 0);

		TEST_EQ (len, 4);
		TEST_EQ_MEM (buf, "test", 4);

		cmsg = CMSG_FIRSTHDR (&msghdr);
		TEST_EQ (cmsg->cmsg_level, SOL_SOCKET);
		TEST_EQ (cmsg->cmsg_type, SCM_RIGHTS);
		TEST_EQ (cmsg->cmsg_len, CMSG_LEN (sizeof (int)));
	}

	close (fds[0]);
	close (fds[1]);

	nih_free (msg->control[0]);
	msg->control[0] = NULL;


	/* Check that we can send a message to a specific destination over
	 * an unconnected socket.
	 */
	TEST_FEATURE ("with unconnected sockets");
	addr.sun_family = AF_UNIX;
	addr.sun_path[0] = '\0';

	addrlen = offsetof (struct sockaddr_un, sun_path) + 1;
	addrlen += snprintf (addr.sun_path + 1, sizeof (addr.sun_path) - 1,
			     "/com/netsplit/libnih/test_io/%d", getpid ());

	fds[0] = socket (PF_UNIX, SOCK_DGRAM, 0);
	fds[1] = socket (PF_UNIX, SOCK_DGRAM, 0);
	bind (fds[1], (struct sockaddr *)&addr, addrlen);

	msg->addr = (struct sockaddr *)&addr;
	msg->addrlen = addrlen;

	TEST_ALLOC_FAIL {
		ret = nih_io_message_send (msg, fds[0]);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_EQ (ret, 4);

		msghdr.msg_control = NULL;
		msghdr.msg_controllen = 0;

		len = recvmsg (fds[1], &msghdr, 0);

		TEST_EQ (len, 4);
		TEST_EQ_MEM (buf, "test", 4);
	}

	nih_free (msg);

	close (fds[0]);
	close (fds[1]);


	/* Check that we get an error if the socket is closed. */
	TEST_FEATURE ("with closed socket");
	nih_error_push_context ();
	msg = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg->data, "test", 4));

	TEST_ALLOC_FAIL {
		ret = nih_io_message_send (msg, fds[0]);

		TEST_LT (ret, 0);

		err = nih_error_get ();
		if (test_alloc_failed && (test_alloc_failed < 2)) {
			TEST_EQ (err->number, ENOMEM);
		} else {
			TEST_EQ (err->number, EBADF);
		}
		nih_free (err);
	}

	nih_free (msg);
	nih_error_pop_context ();
}


static int read_called = 0;
static int close_called = 0;
static int error_called = 0;
static NihError *last_error = NULL;
static const char *last_str = NULL;
static size_t last_len = 0;

static int remove_message = 0;

static void
my_reader (void       *data,
	   NihIo      *io,
	   const char *str,
	   size_t      len)
{
	read_called++;

	if (remove_message) {
		nih_free (nih_io_read_message (NULL, io));
		remove_message = 0;
		return;
	}

	if (! data)
		nih_free (io);

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
	NihError         *err;

	TEST_FUNCTION ("nih_io_reopen");

	/* Check that we can create a stream mode NihIo structure from an
	 * existing file descriptor; the structure should be correctly
	 * populated and assigned an NihIoWatch.  The file descriptor
	 * should be altered so that it is non-blocking.
	 */
	TEST_FEATURE ("with stream mode");
	TEST_ALLOC_FAIL {
		assert0 (pipe (fds));
		io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
				    my_reader, my_close_handler,
				    my_error_handler, &io);

		if (test_alloc_failed) {
			TEST_EQ_P (io, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_ALLOC_SIZE (io, sizeof (NihIo));
		TEST_ALLOC_PARENT (io->send_buf, io);
		TEST_ALLOC_SIZE (io->send_buf, sizeof (NihIoBuffer));
		TEST_ALLOC_PARENT (io->recv_buf, io);
		TEST_ALLOC_SIZE (io->recv_buf, sizeof (NihIoBuffer));
		TEST_EQ (io->type, NIH_IO_STREAM);
		TEST_EQ_P (io->reader, my_reader);
		TEST_EQ_P (io->close_handler, my_close_handler);
		TEST_EQ_P (io->error_handler, my_error_handler);
		TEST_EQ_P (io->data, &io);
		TEST_FALSE (io->shutdown);
		TEST_EQ_P (io->free, NULL);

		TEST_ALLOC_PARENT (io->watch, io);
		TEST_EQ (io->watch->fd, fds[0]);
		TEST_EQ (io->watch->events, NIH_IO_READ);
		TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

		nih_free (io);
		close (fds[1]);
	}


	/* Check that we can create a message mode NihIo structure from an
	 * existing file descriptor; the structure should be correctly
	 * populated and assigned an NihIoWatch.  The file descriptor
	 * should be altered so that it is non-blocking.
	 */
	TEST_FEATURE ("with message mode");
	TEST_ALLOC_FAIL {
		assert0 (pipe (fds));
		io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
				    my_reader, my_close_handler,
				    my_error_handler, &io);

		if (test_alloc_failed) {
			TEST_EQ_P (io, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
			continue;
		}

		TEST_ALLOC_SIZE (io, sizeof (NihIo));
		TEST_ALLOC_PARENT (io->send_q, io);
		TEST_ALLOC_SIZE (io->send_q, sizeof (NihList));
		TEST_ALLOC_PARENT (io->recv_q, io);
		TEST_ALLOC_SIZE (io->recv_q, sizeof (NihList));
		TEST_EQ (io->type, NIH_IO_MESSAGE);
		TEST_EQ_P (io->reader, my_reader);
		TEST_EQ_P (io->close_handler, my_close_handler);
		TEST_EQ_P (io->error_handler, my_error_handler);
		TEST_EQ_P (io->data, &io);
		TEST_FALSE (io->shutdown);
		TEST_EQ_P (io->free, NULL);

		TEST_ALLOC_PARENT (io->watch, io);
		TEST_EQ (io->watch->fd, fds[0]);
		TEST_EQ (io->watch->events, NIH_IO_READ);
		TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

		nih_free (io);
		close (fds[1]);
	}


	/* Check that the SIGPIPE signal will now be ignored */
	sigaction (SIGPIPE, NULL, &oldact);
	TEST_EQ (oldact.sa_handler, SIG_IGN);


	/* Check that we get EBADF raised if we try and reopen a file that
	 * is closed.
	 */
	TEST_FEATURE ("with closed file");
	nih_error_push_context ();
	assert0 (pipe (fds));
	close (fds[0]);
	close (fds[1]);

	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    my_reader, my_close_handler,
			    my_error_handler, &io);

	TEST_EQ_P (io, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, EBADF);
	nih_free (err);
	nih_error_pop_context ();
}


void
test_shutdown (void)
{
	NihIo        *io;
	NihIoMessage *msg;
	int           fds[2];
	fd_set        readfds, writefds, exceptfds;

	TEST_FUNCTION ("nih_io_shutdown");
	assert0 (pipe (fds));
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);
	assert0 (nih_io_buffer_push (io->recv_buf, "some data", 9));

	TEST_FREE_TAG (io);

	/* Check that shutting down a socket with data in the buffer
	 * merely marks it as shutdown and neither closes the socket or
	 * frees the structure.
	 */
	TEST_FEATURE ("with data in the buffer");
	nih_io_shutdown (io);

	TEST_TRUE (io->shutdown);
	TEST_NOT_FREE (io);
	TEST_GE (fcntl (fds[0], F_GETFD), 0);


	/* Check that handling the data in the buffer, emptying it, causes
	 * the shutdown socket to be closed and the structure to be freed.
	 */
	TEST_FEATURE ("with data being handled");
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);
	nih_io_buffer_shrink (io->recv_buf, 9);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that shutting down a socket with no data in the buffer
	 * results in it being immediately closed and freed.
	 */
	TEST_FEATURE ("with no data in the buffer");
	assert0 (pipe (fds));
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);

	TEST_FREE_TAG (io);

	nih_io_shutdown (io);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);


	/* Check that shutting down a socket with a message in the receive
	 * queue merely marks it as shutdown and neither closes the socket
	 * or frees the structure.
	 */
	TEST_FEATURE ("with message in the queue");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data, "some data", 9));
	nih_list_add (io->recv_q, &msg->entry);

	TEST_FREE_TAG (io);

	nih_io_shutdown (io);

	TEST_NOT_FREE (io);
	TEST_TRUE (io->shutdown);
	TEST_GE (fcntl (fds[0], F_GETFD), 0);


	/* Check that removing the message from the queue, emptying it, causes
	 * the shutdown socket to be closed and the structure to be freed.
	 */
	TEST_FEATURE ("with message being handled");
	nih_free (msg);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);
	FD_SET (fds[0], &readfds);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);


	/* Check that shutting down a socket with no message in the queue
	 * results in it being immediately closed and freed.
	 */
	TEST_FEATURE ("with no message in the queue");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);

	TEST_FREE_TAG (io);

	nih_io_shutdown (io);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);
}

void
test_destroy (void)
{
	NihIo *io;
	int    fds[2];

	TEST_FUNCTION ("nih_io_destroy");

	/* Check that freeing an open file descriptor doesn't call the error
	 * handler, and just closes the fd and frees the structure.
	 */
	TEST_FEATURE ("with open file descriptor");
	nih_error_push_context ();
	assert0 (pipe (fds));
	error_called = 0;
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, my_error_handler, &io);

	nih_free (io);

	TEST_FALSE (error_called);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);
	nih_error_pop_context ();


	/* Check that closing a file descriptor that's already closed
	 * results in the error handler being called with an EBADF system
	 * error and the data pointer, followed by the structure being
	 * freed.
	 */
	TEST_FEATURE ("with closed file descriptor");
	nih_error_push_context ();
	assert0 (pipe (fds));
	error_called = 0;
	last_data = NULL;
	last_error = NULL;
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, my_error_handler, &io);

	close (fds[0]);
	nih_free (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);

	nih_free (last_error);

	close (fds[1]);
	nih_error_pop_context ();
}

void
test_watcher (void)
{
	NihIo         *io;
	NihIoMessage  *msg, *msg2;
	int            fds[2];
	ssize_t        len;
	struct msghdr  msghdr;
	struct iovec   iov[1];
	char           buf[BUFSIZ * 2];
	fd_set         readfds, writefds, exceptfds;
	FILE          *output;

	TEST_FUNCTION ("nih_io_watcher");

	/* Check that data to be read on a socket watched by NihIo ends up
	 * in the receive buffer, and results in the reader function being
	 * called with the right arguments.
	 */
	TEST_FEATURE ("with data to read");
	assert0 (pipe (fds));
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	TEST_ALLOC_FAIL {
		io->recv_buf->len = 0;
		io->recv_buf->size = 0;

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

		if (test_alloc_failed == 1) {
			TEST_FALSE (read_called);

			TEST_EQ (read (fds[0], buf, sizeof (buf)), 14);
			continue;
		}

		TEST_TRUE (read_called);
		TEST_EQ_P (last_data, &io);
		TEST_EQ_P (last_str, io->recv_buf->buf);
		TEST_EQ (last_len, io->recv_buf->len);
		TEST_EQ (io->recv_buf->len, 14);
		TEST_EQ_MEM (io->recv_buf->buf, "this is a test", 14);
	}


	/* Check that the reader function is called again when more data
	 * comes in, and that the buffer contains both sets of data.  This
	 * should be true even if we're out of memory.
	 */
	TEST_FEATURE ("with more data to read");
	TEST_ALLOC_FAIL {
		io->recv_buf->len = 14;
		io->recv_buf->size = BUFSIZ;

		assert (write (fds[1], " of the callback code", 19) == 19);

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
		TEST_EQ_MEM (io->recv_buf->buf,
			     "this is a test of the callback code", 33);
	}


	/* Check that the reader function can call nih_free(), resulting
	 * in the structure being closed once it has finished the watcher
	 * function.
	 */
	TEST_FEATURE ("with free called in reader");
	io->data = NULL;

	TEST_FREE_TAG (io);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that the reader function is also called when the remote end
	 * has been closed; along with the close function.
	 */
	TEST_FEATURE ("with remote end closed");
	nih_error_push_context ();
	assert0 (pipe (fds));
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	assert0 (nih_io_buffer_push (io->recv_buf,
				     "this is a test of the callback code",
				     33));

	read_called = 0;
	close_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

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
	nih_error_pop_context ();


	/* Check that the reader function and error handler are called if
	 * the local end gets closed.  The error should be EBADF.
	 */
	TEST_FEATURE ("with local end closed");
	nih_error_push_context ();
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

	error_called = 0;
	last_error = NULL;

	nih_free (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);

	nih_free (last_error);

	nih_error_pop_context ();


	/* Check that if the remote end closes and there's no close handler,
	 * the file descriptor is closed and the structure freed.
	 */
	TEST_FEATURE ("with no close handler");
	nih_error_push_context ();
	assert0 (pipe (fds));
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, NULL, NULL, &io);

	TEST_FREE_TAG (io);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);
	nih_error_pop_context ();


	/* Check that if the local end closes and there's no error handler
	 * that the structure is freed.
	 */
	TEST_FEATURE ("with no error handler");
	nih_error_push_context ();
	assert0 (pipe (fds));
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    my_reader, NULL, NULL, &io);

	TEST_FREE_TAG (io);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

 	nih_log_set_priority (NIH_LOG_FATAL);
	close (fds[0]);
	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);
	nih_log_set_priority (NIH_LOG_MESSAGE);

	TEST_FREE (io);
	nih_error_pop_context ();


	/* Check that data in the send buffer is written to the file
	 * descriptor if it's pollable for writing.  Once the data has been
	 * written, the watch should no longer be checking for writability.
	 */
	TEST_FEATURE ("with data to write");
	output = tmpfile ();
	io = nih_io_reopen (NULL, fileno (output), NIH_IO_STREAM,
			    NULL, my_close_handler, my_error_handler, &io);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			assert0 (nih_io_printf (io, "this is a test\n"));
		}

		FD_ZERO (&readfds);
		FD_SET (fileno (output), &writefds);
		nih_io_handle_fds (&readfds, &writefds, &exceptfds);

		rewind (output);

		TEST_FILE_EQ (output, "this is a test\n");
		TEST_FILE_END (output);

		TEST_EQ (io->send_buf->len, 0);
		TEST_EQ (io->send_buf->size, 0);
		TEST_EQ_P (io->send_buf->buf, NULL);

		TEST_FALSE (io->watch->events & NIH_IO_WRITE);
	}


	/* Check that we can write more data and that is send out to the
	 * file descriptor as well.
	 */
	TEST_FEATURE ("with more data to write");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			assert0 (nih_io_printf (io, "so is this\n"));
		}

		nih_io_handle_fds (&readfds, &writefds, &exceptfds);

		rewind (output);

		TEST_FILE_EQ (output, "this is a test\n");
		TEST_FILE_EQ (output, "so is this\n");
		TEST_FILE_END (output);

		TEST_EQ (io->send_buf->len, 0);
		TEST_EQ (io->send_buf->size, 0);
		TEST_EQ_P (io->send_buf->buf, NULL);

		TEST_FALSE (io->watch->events & NIH_IO_WRITE);
	}

	fclose (output);


	/* Check that an attempt to write to a closed file results in the
	 * error handler being called directly, rather than needing to wait
	 * for a read again.
	 */
	TEST_FEATURE ("with closed file");
	nih_error_push_context ();
	error_called = 0;
	last_data = NULL;
	last_error = NULL;

	assert0 (nih_io_printf (io, "this write fails\n"));

	FD_ZERO (&writefds);
	FD_SET (fds[0], &writefds);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (io->send_buf->len, 17);
	TEST_EQ_MEM (io->send_buf->buf, "this write fails\n", 17);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);

	nih_free (last_error);

	error_called = 0;
	last_error = NULL;

	nih_free (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);

	nih_free (last_error);

	nih_error_pop_context ();


	/* Check that a message to be read on a socket watched by NihIo ends
	 * up in the receive queue, and results in the reader function being
	 * called just once with the right arguments.
	 */
	TEST_FEATURE ("with message to read");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	TEST_ALLOC_FAIL {
		msghdr.msg_name = NULL;
		msghdr.msg_namelen = 0;
		msghdr.msg_iov = iov;
		msghdr.msg_iovlen = 1;;
		msghdr.msg_control = NULL;
		msghdr.msg_controllen = 0;
		msghdr.msg_flags = 0;

		iov[0].iov_base = buf;
		iov[0].iov_len = sizeof (buf);

		memcpy (buf, "this is a test", 14);
		iov[0].iov_len = 14;

		sendmsg (fds[1], &msghdr, 0);

		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		FD_ZERO (&exceptfds);
		FD_SET (fds[0], &readfds);

		read_called = 0;
		last_data = NULL;
		last_str = NULL;
		last_len = 0;

		nih_io_handle_fds (&readfds, &writefds, &exceptfds);

		if (test_alloc_failed && (test_alloc_failed < 8)) {
			TEST_EQ (recvmsg (fds[0], &msghdr, 0), 14);
			continue;
		} else if (test_alloc_failed) {
			msg = (NihIoMessage *)io->recv_q->prev;

			nih_free (msg);
			continue;
		}

		TEST_LIST_NOT_EMPTY (io->recv_q);

		msg = (NihIoMessage *)io->recv_q->next;

		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_ALLOC_PARENT (msg, io);

		TEST_EQ (msg->data->len, 14);
		TEST_EQ_MEM (msg->data->buf, "this is a test", 14);

		TEST_EQ (read_called, 1);
		TEST_EQ_P (last_data, &io);
		TEST_EQ_P (last_str, msg->data->buf);
		TEST_EQ (last_len, msg->data->len);
	}


	/* Check that the reader function is called again when more data
	 * comes in, but that it is only called once with the data in
	 * the older message, not the newer.
	 */
	TEST_FEATURE ("with another message to read");
	memcpy (buf, "another test", 12);
	iov[0].iov_len = 12;

	sendmsg (fds[1], &msghdr, 0);

	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_LIST_NOT_EMPTY (io->recv_q);

	msg = (NihIoMessage *)io->recv_q->next;

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_ALLOC_PARENT (msg, io);

	TEST_EQ (msg->data->len, 14);
	TEST_EQ_MEM (msg->data->buf, "this is a test", 14);

	TEST_EQ (read_called, 1);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, msg->data->buf);
	TEST_EQ (last_len, msg->data->len);

	msg = (NihIoMessage *)io->recv_q->next->next;

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_ALLOC_PARENT (msg, io);

	TEST_EQ (msg->data->len, 12);
	TEST_EQ_MEM (msg->data->buf, "another test", 12);


	/* Check that the reader is called twice if the first invocation
	 * removes the oldest message, the second invocation should be with
	 * the new message.
	 */
	TEST_FEATURE ("with message removed during call");
	read_called = 0;
	remove_message = 1;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_LIST_NOT_EMPTY (io->recv_q);

	msg = (NihIoMessage *)io->recv_q->next;

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_ALLOC_PARENT (msg, io);

	TEST_EQ (msg->data->len, 12);
	TEST_EQ_MEM (msg->data->buf, "another test", 12);

	TEST_EQ (read_called, 2);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, msg->data->buf);
	TEST_EQ (last_len, msg->data->len);


	/* Check that the reader is only called once if the message is
	 * removed, and that has no ill effect.
	 */
	TEST_FEATURE ("with last message removed during call");
	read_called = 0;
	remove_message = 1;

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_LIST_EMPTY (io->recv_q);

	TEST_EQ (read_called, 1);


	/* Check that the reader function can call nih_free(), resulting
	 * in the structure being closed once it has finished the watcher
	 * function.
	 */
	TEST_FEATURE ("with close called in reader for message");
	io->data = NULL;

	TEST_FREE_TAG (io);

	memcpy (buf, "test with close", 15);
	iov[0].iov_len = 15;

	sendmsg (fds[1], &msghdr, 0);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	close (fds[1]);


	/* Check that the error handler is called if the local end of a
	 * socket is closed (we should get EBADF).  The reader should also
	 * be called with the oldest message currently in the queue.
	 */
	TEST_FEATURE ("with local end closed");
	nih_error_push_context ();
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data, "this is a test", 14));
	nih_list_add (io->recv_q, &msg->entry);

	error_called = 0;
	last_error = NULL;
	read_called = 0;
	last_data = NULL;
	last_str = NULL;
	last_len = 0;

	close (fds[0]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_LIST_NOT_EMPTY (io->recv_q);

	msg = (NihIoMessage *)io->recv_q->next;

	TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
	TEST_ALLOC_PARENT (msg, io);

	TEST_EQ (msg->data->len, 14);
	TEST_EQ_MEM (msg->data->buf, "this is a test", 14);

	TEST_EQ (read_called, 1);
	TEST_EQ_P (last_data, &io);
	TEST_EQ_P (last_str, msg->data->buf);
	TEST_EQ (last_len, msg->data->len);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);

	nih_free (last_error);

	error_called = 0;
	last_error = NULL;

	nih_free (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);

	nih_free (last_error);

	nih_error_pop_context ();


	/* Check that if the local end of a socket is closed, and there's
	 * no error handler, the structure freed.
	 */
	TEST_FEATURE ("with no error handler");
	nih_error_push_context ();
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    my_reader, NULL, NULL, &io);

	TEST_FREE_TAG (io);

	FD_ZERO (&readfds);
	FD_SET (fds[0], &readfds);

 	nih_log_set_priority (NIH_LOG_FATAL);
	close (fds[0]);
	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);
 	nih_log_set_priority (NIH_LOG_MESSAGE);

	TEST_FREE (io);
	nih_error_pop_context ();


	/* Check that a message in the send queue is written to the socket
	 * if it's pollable for writing, removed from the queue and that
	 * the socket is no longer watched for writability.
	 */
	TEST_FEATURE ("with message to write");
	socketpair (PF_UNIX, SOCK_DGRAM, 0, fds);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    my_reader, my_close_handler, my_error_handler,
			    &io);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			msg = nih_io_message_new (NULL);
			assert0 (nih_io_buffer_push (msg->data,
						     "this is a test", 14));
			nih_io_send_message (io, msg);
			nih_discard (msg);
		}

		TEST_FREE_TAG (msg);

		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
		FD_SET (fds[0], &writefds);

		nih_io_handle_fds (&readfds, &writefds, &exceptfds);

		if (test_alloc_failed) {
			TEST_LIST_NOT_EMPTY (io->send_q);
			TEST_TRUE (io->watch->events & NIH_IO_WRITE);
			TEST_NOT_FREE (msg);

			nih_free (msg);
			continue;
		}

		iov[0].iov_base = buf;
		iov[0].iov_len = sizeof (buf);

		TEST_LIST_EMPTY (io->send_q);
		TEST_FALSE (io->watch->events & NIH_IO_WRITE);
		TEST_FREE (msg);

		len = recvmsg (fds[1], &msghdr, 0);

		TEST_EQ (len, 14);
		TEST_EQ_MEM (buf, "this is a test\n", 14);
	}


	/* Check that we can write another message to the send queue, which
	 * should also go straight out and have the writability cleared.
	 */
	TEST_FEATURE ("with another message to write");
	msg = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg->data, "another test", 12));
	nih_io_send_message (io, msg);
	nih_discard (msg);

	TEST_FREE_TAG (msg);

	FD_ZERO (&writefds);
	FD_SET (fds[0], &writefds);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_LIST_EMPTY (io->send_q);
	TEST_FALSE (io->watch->events & NIH_IO_WRITE);
	TEST_FREE (msg);

	len = recvmsg (fds[1], &msghdr, 0);

	TEST_EQ (len, 12);
	TEST_EQ_MEM (buf, "another test", 12);


	/* Check that we can place multiple messages in the send queue and
	 * have them all go straight out.
	 */
	TEST_FEATURE ("with multiple messages to write");
	msg = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg->data, "this is a test", 14));
	nih_io_send_message (io, msg);
	nih_discard (msg);

	TEST_FREE_TAG (msg);

	msg2 = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg2->data, "another test", 12));
	nih_io_send_message (io, msg2);
	nih_discard (msg2);

	TEST_FREE_TAG (msg2);

	FD_ZERO (&writefds);
	FD_SET (fds[0], &writefds);

	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_LIST_EMPTY (io->send_q);
	TEST_FALSE (io->watch->events & NIH_IO_WRITE);
	TEST_FREE (msg);
	TEST_FREE (msg2);

	len = recvmsg (fds[1], &msghdr, 0);

	TEST_EQ (len, 14);
	TEST_EQ_MEM (buf, "this is a test", 14);

	len = recvmsg (fds[1], &msghdr, 0);

	TEST_EQ (len, 12);
	TEST_EQ_MEM (buf, "another test", 12);


	/* Check that an attempt to write to a closed descriptor results in
	 * the error handler being called directly, rather than needing to
	 * wait for a read again.
	 */
	TEST_FEATURE ("with closed socket");
	nih_error_push_context ();
	error_called = 0;
	last_data = NULL;
	last_error = NULL;

	msg = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg->data, "one more test", 13));
	nih_io_send_message (io, msg);
	nih_discard (msg);

	TEST_FREE_TAG (msg);

	FD_ZERO (&writefds);
	FD_SET (fds[0], &writefds);

	close (fds[0]);
	close (fds[1]);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_NOT_FREE (msg);
	TEST_LIST_NOT_EMPTY (io->send_q);
	TEST_EQ_P (io->send_q->next, &msg->entry);
	TEST_TRUE (io->watch->events & NIH_IO_WRITE);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);
	TEST_EQ_P (last_data, &io);

	nih_free (last_error);

	error_called = 0;
	last_error = NULL;

	nih_free (io);

	TEST_TRUE (error_called);
	TEST_EQ (last_error->number, EBADF);

	nih_free (last_error);

	nih_error_pop_context ();
}


void
test_read_message (void)
{
	NihIo        *io;
	NihIoMessage *msg, *ptr;
	int           fds[2];

	TEST_FUNCTION ("nih_io_read_message");
	assert0 (pipe (fds));
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data, "this is a test", 14));
	nih_list_add (io->recv_q, &msg->entry);

	/* Check that we can read a message in the NihIo receive queue,
	 * the message returned should be the same message we queued and
	 * should be reparented as well as removed from the queue.
	 */
	TEST_FEATURE ("with message in queue");
	ptr = nih_io_read_message (NULL, io);

	TEST_EQ_P (ptr, msg);
	TEST_ALLOC_PARENT (msg, NULL);
	TEST_LIST_EMPTY (&msg->entry);
	TEST_LIST_EMPTY (io->recv_q);


	/* Check that we get NULL when the receive queue is empty. */
	TEST_FEATURE ("with empty queue");
	ptr = nih_io_read_message (NULL, io);

	TEST_EQ_P (ptr, NULL);


	/* Check that the socket is closed and the structure freed when
	 * we take the last data from a shutdown socket.
	 */
	TEST_FEATURE ("with shutdown socket");
	TEST_FREE_TAG (io);

	nih_ref (msg, io);
	nih_list_add (io->recv_q, &msg->entry);
	nih_io_shutdown (io);
	ptr = nih_io_read_message (NULL, io);

	TEST_EQ_P (ptr, msg);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	nih_free (msg);
}

void
test_send_message (void)
{
	NihIo        *io;
	NihIoMessage *msg1, *msg2;
	int           fds[2];

	TEST_FUNCTION ("nih_io_send_message");
	assert0 (pipe (fds));
	close (fds[0]);

	io = nih_io_reopen (NULL, fds[1], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);


	/* Check that we can send a message into the empty send queue, it
	 * should be added directly to the send queue, and a reference
	 * taken.
	 */
	TEST_FEATURE ("with empty send queue");
	msg1 = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg1->data, "this is a test", 14));

	nih_io_send_message (io, msg1);

	TEST_EQ_P (io->send_q->next, &msg1->entry);
	TEST_ALLOC_PARENT (msg1, io);

	TEST_TRUE (io->watch->events & NIH_IO_WRITE);


	/* Check that we can send a message when there's already one in
	 * the send queue, it should be appended to the queue.
	 */
	TEST_FEATURE ("with message already in send queue");
	msg2 = nih_io_message_new (NULL);
	assert0 (nih_io_buffer_push (msg2->data, "this is a test", 14));

	nih_io_send_message (io, msg2);

	TEST_EQ_P (io->send_q->next, &msg1->entry);
	TEST_EQ_P (io->send_q->prev, &msg2->entry);

	nih_free (msg1);
	nih_free (msg2);
	nih_free (io);
}


void
test_read (void)
{
	NihIo        *io;
	NihIoMessage *msg;
	char         *str;
	size_t        len;
	int           fds[2];

	TEST_FUNCTION ("nih_io_read");
	assert0 (pipe (fds));
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);
	assert0 (nih_io_buffer_push (io->recv_buf,
				     "this is a test of the io code", 29));


	/* Check that we can read data in the NihIo receive buffer, and the
	 * data is returned NULL-terminated, allocated with nih_alloc and
	 * removed from the front of the receive buffer itself.
	 */
	TEST_FEATURE ("with full buffer");
	TEST_ALLOC_FAIL {
		len = 14;
		str = nih_io_read (NULL, io, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ (len, 14);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ (str[14], '\0');
		TEST_EQ_STR (str, "this is a test");
		TEST_EQ (io->recv_buf->len, 15);
		TEST_EQ_MEM (io->recv_buf->buf, " of the io code", 15);

		nih_free (str);
	}


	/* Check that we can empty all of the data from the NihIo receive
	 * buffer, which results in the buffer being freed.
	 */
	TEST_FEATURE ("with request to empty buffer");
	TEST_ALLOC_FAIL {
		len = 15;
		str = nih_io_read (NULL, io, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ (len, 15);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ (str[15], '\0');
		TEST_EQ_STR (str, " of the io code");
		TEST_EQ (io->recv_buf->len, 0);
		TEST_EQ (io->recv_buf->size, 0);
		TEST_EQ_P (io->recv_buf->buf, NULL);

		nih_free (str);
	}


	/* Check that we can request more data than is in the buffer, and
	 * get a short read with len updated.
	 */
	TEST_FEATURE ("with larger request than buffer");
	assert0 (nih_io_buffer_push (io->recv_buf, "another test", 12));

	TEST_ALLOC_FAIL {
		len = 20;
		str = nih_io_read (NULL, io, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ (len, 12);
		TEST_ALLOC_SIZE (str, 13);
		TEST_EQ (str[12], '\0');
		TEST_EQ_STR (str, "another test");
		TEST_EQ (io->recv_buf->len, 0);
		TEST_EQ (io->recv_buf->size, 0);
		TEST_EQ_P (io->recv_buf->buf, NULL);

		nih_free (str);
	}


	/* Check that the socket is closed and the structure freed when
	 * we take the last data from a shutdown socket.
	 */
	TEST_FEATURE ("with shutdown socket and last data");
	TEST_FREE_TAG (io);

	assert0 (nih_io_buffer_push (io->recv_buf, "this is a test", 14));
	nih_io_shutdown (io);
	len = 14;
	str = nih_io_read (NULL, io, &len);

	TEST_EQ (len, 14);
	TEST_NE_P (str, NULL);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	nih_free (str);


	/* Check that we can request data while in message mode, and receive
	 * data from the first message; which should have its buffer shrunk.
	 */
	TEST_FEATURE ("with full message in queue");
	assert0 (pipe (fds));
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data,
				     "this is a test of the io code", 29));
	nih_list_add (io->recv_q, &msg->entry);

	TEST_ALLOC_FAIL {
		len = 14;
		str = nih_io_read (NULL, io, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ (len, 14);
		TEST_ALLOC_SIZE (str, 15);
		TEST_EQ (str[14], '\0');
		TEST_EQ_STR (str, "this is a test");
		TEST_EQ (msg->data->len, 15);
		TEST_EQ_MEM (msg->data->buf, " of the io code", 15);

		nih_free (str);
	}


	/* Check that when we empty the buffer of the message, it is freed
	 * and removed from the receive queue.
	 */
	TEST_FEATURE ("with request to empty message buffer");
	TEST_FREE_TAG (msg);

	TEST_ALLOC_FAIL {
		len = 15;
		str = nih_io_read (NULL, io, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ (len, 15);
		TEST_ALLOC_SIZE (str, 16);
		TEST_EQ (str[15], '\0');
		TEST_EQ_STR (str, " of the io code");
		TEST_FREE (msg);

		TEST_LIST_EMPTY (io->recv_q);

		nih_free (str);
	}


	/* Check that we receive a zero-length string and len is set to
	 * zero if there is no message in the queue.
	 */
	TEST_FEATURE ("with empty message queue");
	TEST_ALLOC_FAIL {
		len = 10;
		str = nih_io_read (NULL, io, &len);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ (len, 0);
		TEST_ALLOC_SIZE (str, 1);
		TEST_EQ (str[0], '\0');

		nih_free (str);
	}


	/* Check that the socket is closed and the structure freed when
	 * we take the last data from a shutdown socket.
	 */
	TEST_FEATURE ("with shutdown socket and last message");
	TEST_FREE_TAG (io);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data, "this is a test", 14));
	nih_list_add (io->recv_q, &msg->entry);

	nih_io_shutdown (io);
	len = 14;
	str = nih_io_read (NULL, io, &len);

	TEST_EQ (len, 14);
	TEST_NE_P (str, NULL);

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	nih_free (str);
}

void
test_write (void)
{
	NihIo        *io;
	NihIoMessage *msg;
	int           ret, fds[2];

	TEST_FUNCTION ("nih_io_write");
	assert0 (pipe (fds));
	close (fds[0]);

	io = nih_io_reopen (NULL, fds[1], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);

	/* Check that we can write data into the NihIo send buffer, the
	 * buffer should contain the data and be a page in size.  The
	 * watch should also now be looking for writability.
	 */
	TEST_FEATURE ("with empty buffer");
	TEST_ALLOC_FAIL {
		io->send_buf->len = 0;
		io->send_buf->size = 0;
		ret = nih_io_write (io, "test", 4);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (io->send_buf->buf, BUFSIZ);
		TEST_EQ (io->send_buf->size, BUFSIZ);
		TEST_EQ (io->send_buf->len, 4);
		TEST_EQ_MEM (io->send_buf->buf, "test", 4);
		TEST_TRUE (io->watch->events & NIH_IO_WRITE);
	}


	/* Check that we can write more data onto the end of the NihIo
	 * send buffer, which increases its size.
	 */
	TEST_FEATURE ("with data in the buffer");
	TEST_ALLOC_FAIL {
		io->send_buf->len = 4;
		io->send_buf->size = BUFSIZ;
		ret = nih_io_write (io, "ing the io code", 10);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (io->send_buf->len, 14);
		TEST_EQ_MEM (io->send_buf->buf, "testing the io", 14);

		nih_free (io);
	}


	/* Check that we can write data into a message mode NihIo, and
	 * have it made into a new message in the send queue.
	 */
	TEST_FEATURE ("with empty send queue");
	assert0 (pipe (fds));
	close (fds[0]);

	io = nih_io_reopen (NULL, fds[1], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);
	TEST_ALLOC_FAIL {
		ret = nih_io_write (io, "test", 4);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_LIST_NOT_EMPTY (io->send_q);

		msg = (NihIoMessage *)io->send_q->next;

		TEST_ALLOC_PARENT (msg, io);
		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_ALLOC_SIZE (msg->data->buf, BUFSIZ);
		TEST_EQ (msg->data->size, BUFSIZ);
		TEST_EQ (msg->data->len, 4);
		TEST_EQ_MEM (msg->data->buf, "test", 4);
		TEST_TRUE (io->watch->events & NIH_IO_WRITE);
	}


	/* Check that we can write more data as another new message onto
	 * the send queue.
	 */
	TEST_FEATURE ("with message in the send queue");
	TEST_ALLOC_FAIL {
		ret = nih_io_write (io, "ing the io code", 10);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_LIST_NOT_EMPTY (io->send_q);

		msg = (NihIoMessage *)io->send_q->next;

		TEST_EQ (msg->data->len, 4);
		TEST_EQ_MEM (msg->data->buf, "test", 4);

		msg = (NihIoMessage *)io->send_q->next->next;

		TEST_ALLOC_PARENT (msg, io);
		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_ALLOC_SIZE (msg->data->buf, BUFSIZ);
		TEST_EQ (msg->data->size, BUFSIZ);
		TEST_EQ (msg->data->len, 10);
		TEST_EQ_MEM (msg->data->buf, "ing the io code", 10);
		TEST_TRUE (io->watch->events & NIH_IO_WRITE);
	}

	nih_free (io);
}

void
test_get (void)
{
	NihIo        *io;
	NihIoMessage *msg;
	char         *str;
	int           fds[2];

	TEST_FUNCTION ("nih_io_get");
	assert0 (pipe (fds));
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);
	assert0 (nih_io_buffer_push (io->recv_buf, "some data\n", 10));
	assert0 (nih_io_buffer_push (io->recv_buf, "and another line\n", 17));
	assert0 (nih_io_buffer_push (io->recv_buf, "incomplete", 10));

	/* Check that we can take data from the front of a buffer up until
	 * the first embedded new line (which isn't returned), and have the
	 * buffer shuffled up.
	 */
	TEST_FEATURE ("with full buffer");
	TEST_ALLOC_FAIL {
		str = nih_io_get (NULL, io, "\n");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (io->recv_buf->len, 27);
			TEST_EQ_MEM (io->recv_buf->buf,
				     "and another line\nincomplete", 27);
			continue;
		}

		TEST_ALLOC_SIZE (str, 10);
		TEST_EQ_STR (str, "some data");

		TEST_EQ (io->recv_buf->len, 27);
		TEST_EQ_MEM (io->recv_buf->buf, "and another line\nincomplete",
			     27);

		nih_free (str);
	}


	/* Check that we can read up to the next line line. */
	TEST_FEATURE ("with part-full buffer");
	TEST_ALLOC_FAIL {
		str = nih_io_get (NULL, io, "\n");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (io->recv_buf->len, 10);
			TEST_EQ_MEM (io->recv_buf->buf, "incomplete", 10);
			continue;
		}

		TEST_ALLOC_SIZE (str, 17);
		TEST_EQ_STR (str, "and another line");

		TEST_EQ (io->recv_buf->len, 10);
		TEST_EQ_MEM (io->recv_buf->buf, "incomplete", 10);

		nih_free (str);
	}


	/* Check that NULL is returned if the data in the buffer doesn't
	 * contain the delimeter or a NULL terminator.
	 */
	TEST_FEATURE ("with incomplete line in buffer");
	str = nih_io_get (NULL, io, "\n");

	TEST_EQ_P (str, NULL);

	TEST_EQ (io->recv_buf->len, 10);
	TEST_EQ_MEM (io->recv_buf->buf, "incomplete", 10);

	/* Check that a NULL terminator is sufficient to return the data
	 * in the buffer, which should now be empty.
	 */
	TEST_FEATURE ("with null-terminated string in buffer");
	assert0 (nih_io_buffer_push (io->recv_buf, "\0", 1));
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 11);
	TEST_EQ_STR (str, "incomplete");

	TEST_EQ (io->recv_buf->len, 0);

	nih_free (str);


	/* Check that if we empty the buffer of a shutdown socket, the
	 * socket is closed and freed.
	 */
	TEST_FEATURE ("with shutdown socket and emptied buffer");
	TEST_FREE_TAG (io);

	assert0 (nih_io_buffer_push (io->recv_buf, "some data\n", 10));
	nih_io_shutdown (io);
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 10);
	TEST_EQ_STR (str, "some data");

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	nih_free (str);


	/* Check that we can operate in message mode and receive data from
	 * the oldest message.
	 */
	TEST_FEATURE ("with full message in queue");
	assert0 (pipe (fds));
	close (fds[1]);
	io = nih_io_reopen (NULL, fds[0], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data, "some data\n", 10));
	assert0 (nih_io_buffer_push (msg->data, "and another line\n", 17));
	assert0 (nih_io_buffer_push (msg->data, "incomplete", 10));
	nih_list_add (io->recv_q, &msg->entry);

	TEST_ALLOC_FAIL {
		str = nih_io_get (NULL, io, "\n");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (msg->data->len, 27);
			TEST_EQ_MEM (msg->data->buf,
				     "and another line\nincomplete", 27);
			continue;
		}

		TEST_ALLOC_SIZE (str, 10);
		TEST_EQ_STR (str, "some data");

		TEST_EQ (msg->data->len, 27);
		TEST_EQ_MEM (msg->data->buf, "and another line\nincomplete",
			     27);

		nih_free (str);
	}


	/* Check that we can read up to the next line line. */
	TEST_FEATURE ("with part-full message in queue");
	TEST_ALLOC_FAIL {
		str = nih_io_get (NULL, io, "\n");

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			TEST_EQ (msg->data->len, 10);
			TEST_EQ_MEM (msg->data->buf, "incomplete", 10);
			continue;
		}

		TEST_ALLOC_SIZE (str, 17);
		TEST_EQ_STR (str, "and another line");

		TEST_EQ (msg->data->len, 10);
		TEST_EQ_MEM (msg->data->buf, "incomplete", 10);

		nih_free (str);
	}


	/* Check that NULL is returned if the data in the buffer doesn't
	 * contain the delimeter or a NULL terminator.
	 */
	TEST_FEATURE ("with incomplete line in message");
	str = nih_io_get (NULL, io, "\n");

	TEST_EQ_P (str, NULL);

	TEST_EQ (msg->data->len, 10);
	TEST_EQ_MEM (msg->data->buf, "incomplete", 10);


	/* Check that a NULL terminator is sufficient to return the data
	 * in the buffer, which should now be empty.  This should result
	 * in the message being removed from the queue and freed.
	 */
	TEST_FEATURE ("with null-terminated string in message");
	TEST_FREE_TAG (msg);

	assert0 (nih_io_buffer_push (msg->data, "\0", 1));
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 11);
	TEST_EQ_STR (str, "incomplete");

	TEST_FREE (msg);
	TEST_LIST_EMPTY (io->recv_q);

	nih_free (str);


	/* Check that we get NULL if there is no message in the queue. */
	TEST_FEATURE ("with empty message queue");
	str = nih_io_get (NULL, io, "\n");

	TEST_EQ_P (str, NULL);


	/* Check that if we empty the buffer of a shutdown socket, the
	 * socket is closed and freed.
	 */
	TEST_FEATURE ("with shutdown socket and emptied message");
	TEST_FREE_TAG (io);

	msg = nih_io_message_new (io);
	assert0 (nih_io_buffer_push (msg->data, "some data\n", 10));
	nih_list_add (io->recv_q, &msg->entry);

	nih_io_shutdown (io);
	str = nih_io_get (NULL, io, "\n");

	TEST_ALLOC_SIZE (str, 10);
	TEST_EQ_STR (str, "some data");

	TEST_FREE (io);
	TEST_LT (fcntl (fds[0], F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	nih_free (str);
}

void
test_printf (void)
{
	NihIo        *io;
	NihIoMessage *msg;
	int           ret, fds[2];

	TEST_FUNCTION ("nih_io_printf");
	assert0 (pipe (fds));
	close (fds[0]);

	io = nih_io_reopen (NULL, fds[1], NIH_IO_STREAM,
			    NULL, NULL, NULL, NULL);

	/* Check that we can write a line of formatted data into the send
	 * buffer, which should be written without a NULL terminator.
	 * The watch should also look for writability.
	 */
	TEST_FEATURE ("with empty buffer");
	TEST_ALLOC_FAIL {
		io->send_buf->len = 0;
		io->send_buf->size = 0;
		ret = nih_io_printf (io, "this is a %d %s test\n",
				     4, "format");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_SIZE (io->send_buf->buf, BUFSIZ);
		TEST_EQ (io->send_buf->size, BUFSIZ);
		TEST_EQ (io->send_buf->len, 24);
		TEST_EQ_MEM (io->send_buf->buf,
			     "this is a 4 format test\n", 24);
		TEST_TRUE (io->watch->events & NIH_IO_WRITE);
	}


	/* Check that we can append a further line of formatted data into
	 * the send buffer
	 */
	TEST_FEATURE ("with data in the buffer");
	TEST_ALLOC_FAIL {
		io->send_buf->len = 24;
		io->send_buf->size = BUFSIZ;
		ret = nih_io_printf (io, "and this is %s line\n", "another");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (io->send_buf->len, 49);
		TEST_EQ_MEM (io->send_buf->buf,
			     ("this is a 4 format test\n"
			      "and this is another line\n"),
			     49);
	}

	nih_free (io);


	/* Check that we can write a line of formatted data when we're in
	 * message mode, and have it put in a new message and placed into
	 * the send queue.
	 */
	TEST_FEATURE ("with empty send queue");
	assert0 (pipe (fds));
	close (fds[0]);

	io = nih_io_reopen (NULL, fds[1], NIH_IO_MESSAGE,
			    NULL, NULL, NULL, NULL);
	TEST_ALLOC_FAIL {
		ret = nih_io_printf (io, "this is a %d %s test\n",
				     4, "format");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_LIST_NOT_EMPTY (io->send_q);

		msg = (NihIoMessage *)io->send_q->next;

		TEST_ALLOC_PARENT (msg, io);
		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_ALLOC_SIZE (msg->data->buf, BUFSIZ);
		TEST_EQ (msg->data->size, BUFSIZ);
		TEST_EQ (msg->data->len, 24);
		TEST_EQ_MEM (msg->data->buf, "this is a 4 format test\n", 24);
		TEST_TRUE (io->watch->events & NIH_IO_WRITE);
	}


	/* Check that we can append a further line of formatted data into
	 * the send queue as another new message.
	 */
	TEST_FEATURE ("with message in the queue");
	TEST_ALLOC_FAIL {
		ret = nih_io_printf (io, "and this is %s line\n", "another");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_LIST_NOT_EMPTY (io->send_q);

		msg = (NihIoMessage *)io->send_q->next;

		TEST_EQ (msg->data->len, 24);
		TEST_EQ_MEM (msg->data->buf, "this is a 4 format test\n", 24);

		msg = (NihIoMessage *)io->send_q->next->next;

		TEST_ALLOC_PARENT (msg, io);
		TEST_ALLOC_SIZE (msg, sizeof (NihIoMessage));
		TEST_ALLOC_SIZE (msg->data->buf, BUFSIZ);
		TEST_EQ (msg->data->size, BUFSIZ);
		TEST_EQ (msg->data->len, 25);
		TEST_EQ_MEM (msg->data->buf, "and this is another line\n", 25);
		TEST_TRUE (io->watch->events & NIH_IO_WRITE);

	}

	nih_free (io);
}


void
test_set_nonblock (void)
{
	int fds[2], ret;

	TEST_FUNCTION ("nih_io_set_nonblock");

	/* Check that we can trivially mark a socket to be non-blocking. */
	TEST_FEATURE ("with valid descriptor");
	assert0 (pipe (fds));
	ret = nih_io_set_nonblock (fds[0]);

	TEST_EQ (ret, 0);
	TEST_TRUE (fcntl (fds[0], F_GETFL) & O_NONBLOCK);

	close (fds[0]);
	close (fds[1]);


	/* Check that we get -1 if the file descriptor is closed. */
	TEST_FEATURE ("with closed descriptor");
	ret = nih_io_set_nonblock (fds[0]);

	TEST_LT (ret, 0);
}

void
test_set_cloexec (void)
{
	int fds[2], ret;

	TEST_FUNCTION ("nih_io_set_cloexec");

	/* Check that we can trivially mark a socket to be closed on exec. */
	TEST_FEATURE ("with valid descriptor");
	assert0 (pipe (fds));
	ret = nih_io_set_cloexec (fds[0]);

	TEST_EQ (ret, 0);
	TEST_TRUE (fcntl (fds[0], F_GETFD) & FD_CLOEXEC);

	close (fds[0]);
	close (fds[1]);


	/* Check that we get -1 if the file descriptor is closed. */
	TEST_FEATURE ("with closed descriptor");
	ret = nih_io_set_cloexec (fds[0]);

	TEST_LT (ret, 0);
}

void
test_get_family (void)
{
	int fd;

	TEST_FUNCTION ("nih_io_get_family");

	/* Check that we can obtain the family of a UNIX socket. */
	fd = socket (PF_UNIX, SOCK_STREAM, 0);

	if (fd < 0) {
		printf ("SKIP: unix not available\n");
	} else {
		TEST_EQ (nih_io_get_family (fd), PF_UNIX);

		close (fd);
	}


	/* Check that we can obtain the family of an IPv4 socket. */
	fd = socket (PF_INET, SOCK_STREAM, 0);

	if (fd < 0) {
		printf ("SKIP: inet not available\n");
	} else {
		TEST_EQ (nih_io_get_family (fd), PF_INET);

		close (fd);
	}


	/* Check that we can obtain the family of an IPv6 socket. */
	fd = socket (PF_INET6, SOCK_STREAM, 0);

	if (fd < 0) {
		printf ("SKIP: inet6 not available\n");
	} else {
		TEST_EQ (nih_io_get_family (fd), PF_INET6);

		close (fd);
	}


	/* Check that we get -1 on error. */
	fd = open ("/dev/null", O_RDONLY);
	close (fd);

	TEST_LT (nih_io_get_family (fd), 0);
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
	test_buffer_pop ();
	test_buffer_shrink ();
	test_buffer_push ();
	test_message_new ();
	test_message_add_control ();
	test_message_recv ();
	test_message_send ();
	test_reopen ();
	test_shutdown ();
	test_destroy ();
	test_watcher ();
	test_read_message ();
	test_send_message ();
	test_read ();
	test_write ();
	test_get ();
	test_printf ();
	test_set_nonblock ();
	test_set_cloexec ();
	test_get_family ();

	return 0;
}
