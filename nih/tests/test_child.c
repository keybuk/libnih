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

#include <nih/test.h>

#include <signal.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/child.h>


static int reaper_called = 0;
static void *last_data = NULL;
static pid_t last_pid;
static int last_killed = FALSE;
static int last_status;

static void
my_reaper (void  *data,
	   pid_t  pid,
	   int    killed,
	   int    status)
{
	reaper_called++;
	last_data = data;
	last_pid = pid;
	last_killed = killed;
	last_status = status;
}

void
test_add_watch (void)
{
	NihChildWatch *watch;

	TEST_FUNCTION ("nih_child_add_watch");

	/* Check that we can add a watch on a specific pid, and that the
	 * structure is filled in correctly and part of a list.
	 */
	TEST_FEATURE ("with pid");
	watch = nih_child_add_watch (NULL, getpid (), my_reaper, &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihChildWatch));
	TEST_EQ (watch->pid, getpid ());
	TEST_EQ_P (watch->reaper, my_reaper);
	TEST_EQ_P (watch->data, &watch);
	TEST_LIST_NOT_EMPTY (&watch->entry);

	nih_list_free (&watch->entry);


	/* Check that we can add a watch on a pid of -1, which represents
	 * any child.
	 */
	TEST_FEATURE ("with -1 for pid");
	watch = nih_child_add_watch (NULL, -1, my_reaper, &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihChildWatch));
	TEST_EQ (watch->pid, -1);
	TEST_EQ_P (watch->reaper, my_reaper);
	TEST_EQ_P (watch->data, &watch);
	TEST_LIST_NOT_EMPTY (&watch->entry);

	nih_list_free (&watch->entry);
}


static int destroyed = 0;

static int
my_destructor (void *ptr)
{
	destroyed = 1;

	return 0;
}

void
test_poll (void)
{
	NihChildWatch *watch1, *watch2;
	pid_t          pid;

	TEST_FUNCTION ("nih_child_poll");

	/* Check that everything works when we have two watchers, one for
	 * any pid and one for a specific pid.  When our child dies, both
	 * should get called; the specific one should be freed as it is
	 * no longer useful.
	 */
	TEST_FEATURE ("with pid-specific watcher");

	TEST_CHILD (pid) {
		pause ();
	}

	watch1 = nih_child_add_watch (NULL, -1, my_reaper, &watch1);
	nih_alloc_set_destructor (watch1, my_destructor);
	watch2 = nih_child_add_watch (NULL, pid, my_reaper, &watch2);
	nih_alloc_set_destructor (watch2, my_destructor);

	reaper_called = 0;
	destroyed = 0;
	last_data = NULL;
	last_pid = 0;
	last_killed = FALSE;
	last_status = 0;

	kill (pid, SIGTERM);

	while (reaper_called < 2)
		nih_child_poll ();

	TEST_EQ (reaper_called, 2);
	TEST_EQ (last_pid, pid);
	TEST_TRUE (last_killed);
	TEST_EQ (last_status, SIGTERM);
	TEST_TRUE (destroyed);


	/* Check that if we poll again, only the catch-all watcher is
	 * triggered.
	 */
	TEST_CHILD (pid) {
		pause ();
	}

	reaper_called = 0;
	destroyed = 0;
	last_data = NULL;
	last_pid = 0;
	last_killed = FALSE;
	last_status = 0;

	kill (pid, SIGTERM);

	while (reaper_called < 1)
		nih_child_poll ();

	TEST_EQ (reaper_called, 1);
	TEST_EQ (last_pid, pid);
	TEST_TRUE (last_killed);
	TEST_EQ (last_status, SIGTERM);
	TEST_EQ_P (last_data, &watch1);
	TEST_FALSE (destroyed);


	/* Finally check that a poll when nothing has died does nothing. */
	reaper_called = 0;
	nih_child_poll ();

	TEST_EQ (reaper_called, 0);

	nih_free (watch1);
}


int
main (int   argc,
      char *argv[])
{
	test_add_watch ();
	test_poll ();

	return 0;
}
