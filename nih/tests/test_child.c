/* libnih
 *
 * test_child.c - test suite for nih/child.c
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
#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/child.h>


static int reaper_called = 0;
static void *last_data = NULL;
static pid_t last_pid;
static int last_status;

static void
my_reaper (void  *data,
	   pid_t  pid,
	   int    status)
{
	reaper_called++;
	last_data = data;
	last_pid = pid;
	last_status = status;
}

int
test_add_watch (void)
{
	NihChildWatch *watch;
	int            ret = 0;

	printf ("Testing nih_child_add_watch()\n");

	printf ("...with pid\n");
	watch = nih_child_add_watch (getpid (), my_reaper, &ret);

	/* Process id should be that given */
	if (watch->pid != getpid ()) {
		printf ("BAD: process id set incorrectly.\n");
		ret = 1;
	}

	/* Reaper should be function given */
	if (watch->reaper != my_reaper) {
		printf ("BAD: reaper function set incorrectly.\n");
		ret = 1;
	}

	/* Reaper data should be pointer given */
	if (watch->data != &ret) {
		printf ("BAD: reaper data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the child watches list */
	if (NIH_LIST_EMPTY (&watch->entry)) {
		printf ("BAD: not placed into child watches list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (watch) != sizeof (NihChildWatch)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_list_free (&watch->entry);


	printf ("...with -1 for pid\n");
	watch = nih_child_add_watch (-1, my_reaper, &ret);

	/* Process id should be -1 */
	if (watch->pid != -1) {
		printf ("BAD: process id set incorrectly.\n");
		ret = 1;
	}

	/* Reaper should be function given */
	if (watch->reaper != my_reaper) {
		printf ("BAD: reaper function set incorrectly.\n");
		ret = 1;
	}

	/* Reaper data should be pointer given */
	if (watch->data != &ret) {
		printf ("BAD: reaper data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in the child watches list */
	if (NIH_LIST_EMPTY (&watch->entry)) {
		printf ("BAD: not placed into child watches list.\n");
		ret = 1;
	}

	/* Should have been allocated using nih_alloc */
	if (nih_alloc_size (watch) != sizeof (NihChildWatch)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	nih_list_free (&watch->entry);

	return ret;
}

static int destroyed = 0;

static int
my_destructor (void *ptr)
{
	destroyed = 1;

	return 0;
}

int
test_poll (void)
{
	NihChildWatch *watch1, *watch2;
	pid_t          pid;
	int            ret = 0;

	printf ("Testing nih_child_poll()\n");

	pid = fork ();
	if (pid == 0) {
		/* Child */
		poll (NULL, 0, -1);

		exit (0);
	}

	assert (pid >= 0);


	watch1 = nih_child_add_watch (-1, my_reaper, &ret);
	nih_alloc_set_destructor (watch1, my_destructor);
	watch2 = nih_child_add_watch (pid, my_reaper, &pid);
	nih_alloc_set_destructor (watch2, my_destructor);

	reaper_called = 0;
	destroyed = 0;
	last_data = NULL;
	last_pid = 0;
	last_status = 0;
	assert (kill (pid, SIGTERM) == 0);
	usleep (1000); /* Urgh */
	nih_child_poll ();

	/* Both reapers should have been triggered */
	if (reaper_called != 2) {
		printf ("BAD: incorrect number of reapers called.\n");
		ret = 1;
	}

	/* Process id should have been passed */
	if (last_pid != pid) {
		printf ("BAD: last process id wasn't what we expected.\n");
		ret = 1;
	}

	/* Status should have been passed */
	if ((! WIFSIGNALED (last_status))
	    || (WTERMSIG (last_status) != SIGTERM)) {
		printf ("BAD: last status wasn't what we expected.\n");
		ret = 1;
	}

	/* Second watcher should have been destroyed */
	if (! destroyed) {
		printf ("BAD: watcher wasn't destroyed.\n");
		ret = 1;
	}


	pid = fork ();
	if (pid == 0) {
		/* Child */
		poll (NULL, 0, -1);

		exit (0);
	}

	assert (pid >= 0);

	reaper_called = 0;
	destroyed = 0;
	last_data = NULL;
	last_pid = 0;
	last_status = 0;
	assert (kill (pid, SIGTERM) == 0);
	usleep (1000); /* Urgh */
	nih_child_poll ();

	/* Only one reaper should have been triggered */
	if (reaper_called != 1) {
		printf ("BAD: incorrect number of reapers called.\n");
		ret = 1;
	}

	/* Process id should have been passed */
	if (last_pid != pid) {
		printf ("BAD: last process id wasn't what we expected.\n");
		ret = 1;
	}

	/* Status should have been passed */
	if ((! WIFSIGNALED (last_status))
	    || (WTERMSIG (last_status) != SIGTERM)) {
		printf ("BAD: last status wasn't what we expected.\n");
		ret = 1;
	}

	/* Data should have been the data pointer of the first one */
	if (last_data != &ret) {
		printf ("BAD: last data wasn't what we expected.\n");
		ret = 1;
	}

	/* Watcher should not have been destroyed */
	if (destroyed) {
		printf ("BAD: watcher destroyed unexpectedly.\n");
		ret = 1;
	}

	reaper_called = 0;
	nih_child_poll ();

	/* No reapers should have been triggered */
	if (reaper_called != 0) {
		printf ("BAD: reapers called unexpectedly.\n");
		ret = 1;
	}

	return ret;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_add_watch ();
	ret |= test_poll ();

	return ret;
}
