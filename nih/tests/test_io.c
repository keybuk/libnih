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

#include <sys/poll.h>

#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/io.h>


static int callback_called = 0;
static void *last_data = NULL;
static NihIoWatch *last_watch = NULL;
static short last_events = 0;

static void
my_callback (void *data, NihIoWatch *watch, short events)
{
	callback_called++;
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
	watch = nih_io_add_watch (NULL, fds[0], POLLIN, my_callback, &ret);

	/* File descriptor should be that given */
	if (watch->fd != fds[0]) {
		printf ("BAD: file descriptor set incorrectly.\n");
		ret = 1;
	}

	/* Events mask should be that given */
	if (watch->events != POLLIN) {
		printf ("BAD: poll events set incorrectly.\n");
		ret = 1;
	}

	/* Callback function should be that given */
	if (watch->callback != my_callback) {
		printf ("BAD: callback function set incorrectly.\n");
		ret = 1;
	}

	/* Callback data should be that given */
	if (watch->data != &ret) {
		printf ("BAD: callback data set incorrectly.\n");
		ret = 1;
	}

	nih_list_free (&watch->entry);

	close (fds[0]);
	close (fds[1]);

	return ret;
}


int
test_poll_fds (void)
{
	NihIoWatch    *watch1, *watch2, *watch3;
	struct pollfd *ufds;
	nfds_t         nfds;
	int            ret = 0, retval, fds[2];

	printf ("Testing nih_io_poll_fds()\n");
	assert (pipe (fds) == 0);
	watch1 = nih_io_add_watch (NULL, fds[0], POLLIN, my_callback, &ret);
	watch2 = nih_io_add_watch (NULL, fds[1], POLLOUT, my_callback, &ret);
	watch3 = nih_io_add_watch (NULL, fds[0], POLLERR, my_callback, &ret);

	ufds = NULL;
	nfds = 0;
	retval = nih_io_poll_fds (&ufds, &nfds);

	/* Return value should be zero */
	if (retval != 0) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Should have three structures */
	if (nfds != 3) {
		printf ("BAD: nfds wasn't what we expected.\n");
		ret = 1;
	}

	/* Structure array should be allocated with nih_alloc */
	if (nih_alloc_size (ufds) != sizeof (struct pollfd) * 3) {
		printf ("BAD: nih_alloc not used or size incorrect.\n");
		ret = 1;
	}

	/* First entry's file descriptor should be from first watch */
	if (ufds[0].fd != fds[0]) {
		printf ("BAD: first file descriptor incorrect.\n");
		ret = 1;
	}

	/* First entry's events should be from first watch */
	if (ufds[0].events != POLLIN) {
		printf ("BAD: first events incorrect.\n");
		ret = 1;
	}

	/* First entry's revents should be reset */
	if (ufds[0].revents != 0) {
		printf ("BAD: first returned events not cleared.\n");
		ret = 1;
	}

	/* Second entry's file descriptor should be from second watch */
	if (ufds[1].fd != fds[1]) {
		printf ("BAD: second file descriptor incorrect.\n");
		ret = 1;
	}

	/* Second entry's events should be from second watch */
	if (ufds[1].events != POLLOUT) {
		printf ("BAD: second events incorrect.\n");
		ret = 1;
	}

	/* Second entry's revents should be reset */
	if (ufds[1].revents != 0) {
		printf ("BAD: second returned events not cleared.\n");
		ret = 1;
	}

	/* Third entry's file descriptor should be from third watch */
	if (ufds[2].fd != fds[0]) {
		printf ("BAD: third file descriptor incorrect.\n");
		ret = 1;
	}

	/* Third entry's events should be from third watch */
	if (ufds[2].events != POLLERR) {
		printf ("BAD: third events incorrect.\n");
		ret = 1;
	}

	/* Third entry's revents should be reset */
	if (ufds[2].revents != 0) {
		printf ("BAD: third returned events not cleared.\n");
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
	struct pollfd *ufds;
	nfds_t         nfds;
	int            ret = 0, fds[2];

	printf ("Testing nih_io_poll_fds()\n");
	assert (pipe (fds) == 0);
	watch1 = nih_io_add_watch (NULL, fds[0], POLLIN, my_callback, &ret);
	watch2 = nih_io_add_watch (NULL, fds[1], POLLOUT, my_callback, &ret);
	watch3 = nih_io_add_watch (NULL, fds[0], POLLERR, my_callback, &ret);

	ufds = NULL;
	nfds = 0;
	assert (nih_io_poll_fds (&ufds, &nfds) == 0);

	callback_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	ufds[0].revents = POLLIN;
	nih_io_handle_fds (ufds, nfds);

	/* Callback should be called just once */
	if (callback_called != 1) {
		printf ("BAD: callback called incorrect number of times.\n");
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
	if (last_events != POLLIN) {
		printf ("BAD: last events wasn't what we expected.\n");
		ret = 1;
	}


	callback_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	ufds[0].revents = 0;
	ufds[2].revents = POLLERR | POLLHUP;
	nih_io_handle_fds (ufds, nfds);

	/* Callback should be called just once */
	if (callback_called != 1) {
		printf ("BAD: callback called incorrect number of times.\n");
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
	if (last_events != (POLLERR | POLLHUP)) {
		printf ("BAD: last events wasn't what we expected.\n");
		ret = 1;
	}


	callback_called = 0;
	ufds[2].revents = 0;
	ufds[1].revents = POLLHUP;
	nih_io_handle_fds (ufds, nfds);

	/* Callback should not be called */
	if (callback_called != 0) {
		printf ("BAD: callback called unexpectedly.\n");
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
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_add_watch ();
	ret |= test_poll_fds ();
	ret |= test_handle_fds ();

	return ret;
}
