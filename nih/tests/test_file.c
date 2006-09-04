/* libnih
 *
 * test_file.c - test suite for nih/file.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/inotify.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/io.h>
#include <nih/file.h>


static int watcher_called = 0;
static void *last_data = NULL;
static NihFileWatch *last_watch = NULL;
static uint32_t last_events = 0;
static const char *last_name = NULL;

static void
my_watcher (void         *data,
	    NihFileWatch *watch,
	    uint32_t      events,
	    const char   *name)
{
	watcher_called++;
	last_data = data;
	last_watch = watch;
	last_events = events;
	last_name = name ? nih_strdup (watch, name) : NULL;
}


int
test_add_watch (void)
{
	NihFileWatch *watch;
	FILE         *fd;
	fd_set        readfds, writefds, exceptfds;
	char          dirname[22], filename[32];
	int           ret = 0, nfds = 0;

	printf ("Testing nih_file_add_watch()\n");

	printf ("...with file\n");
	sprintf (filename, "/tmp/test_file.%d", getpid ());
	unlink (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	watch = nih_file_add_watch (NULL, filename, IN_OPEN, my_watcher, &ret);

	/* Watch descriptor should be valid */
	if (watch->wd < 0) {
		printf ("BAD: watch fd set incorrectly.\n");
		ret = 1;
	}

	/* Watch path should be the filename */
	if (strcmp (watch->path, filename)) {
		printf ("BAD: watch filename set incorrectly.\n");
		ret = 1;
	}

	/* Watch events should be IN_OPEN */
	if (watch->events != IN_OPEN) {
		printf ("BAD: watch events set incorrectly.\n");
		ret = 1;
	}

	/* Watcher should be function we gave */
	if (watch->watcher != my_watcher) {
		printf ("BAD: watcher set incorrectly.\n");
		ret = 1;
	}

	/* Watcher data should be pointer we gave */
	if (watch->data != &ret) {
		printf ("BAD: watcher data set incorrectly.\n");
		ret = 1;
	}

	/* Should be in a list */
	if (NIH_LIST_EMPTY (&watch->entry)) {
		printf ("BAD: watcher wasn't placed in the list.\n");
		ret = 1;
	}

	/* Structure should be allocated with nih_alloc */
	if (nih_alloc_size (watch) != sizeof (NihFileWatch)) {
		printf ("BAD: nih_alloc was not used.\n");
		ret = 1;
	}

	/* Path should be child */
	if (nih_alloc_parent (watch->path) != watch) {
		printf ("BAD: I/O watch parent wasn't file watch.\n");
		ret = 1;
	}


	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	last_name = NULL;

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Watcher should have been called */
	if (! watcher_called) {
		printf ("BAD: watcher was not called.\n");
		ret = 1;
	}

	/* Data pointer should have been that registered */
	if (last_data != &ret) {
		printf ("BAD: watcher data wasn't what we expected.\n");
		ret = 1;
	}

	/* Watch should have been the one we created */
	if (last_watch != watch) {
		printf ("BAD: watcher watch wasn't what we expected.\n");
		ret = 1;
	}

	/* Events mask should include OPEN */
	if (! (last_events & IN_OPEN)) {
		printf ("BAD: inotify event wasn't what we expected.\n");
		ret = 1;
	}

	/* Name should be NULL */
	if (last_name != NULL) {
		printf ("BAD: last name wasn't what we expected.\n");
		ret = 1;
	}


	nih_free (watch);
	unlink (filename);


	printf ("...with directory\n");

	strcpy (dirname, filename);
	strcat (filename, "/foo");

	assert (mkdir (dirname, 0700) == 0);

	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	last_name = NULL;

	watch = nih_file_add_watch (NULL, dirname, IN_CREATE,
				    my_watcher, &ret);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	/* Watcher should have been called */
	if (! watcher_called) {
		printf ("BAD: watcher was not called.\n");
		ret = 1;
	}

	/* Data pointer should have been that registered */
	if (last_data != &ret) {
		printf ("BAD: watcher data wasn't what we expected.\n");
		ret = 1;
	}

	/* Watch should have been the one we created */
	if (last_watch != watch) {
		printf ("BAD: watcher watch wasn't what we expected.\n");
		ret = 1;
	}

	/* Events mask should include CREATE */
	if (! (last_events & IN_CREATE)) {
		printf ("BAD: inotify event wasn't what we expected.\n");
		ret = 1;
	}

	/* Name should be sub-path */
	if (strcmp (last_name, "foo")) {
		printf ("BAD: last name wasn't what we expected.\n");
		ret = 1;
	}

	nih_free (watch);
	unlink (filename);
	rmdir (dirname);

	return ret;
}


int
test_map (void)
{
	FILE  *fd;
	char   filename[24], *map;
	size_t length;
	int    ret = 0;

	printf ("Testing nih_file_map()\n");
	sprintf (filename, "/tmp/test_file.%d", getpid ());
	unlink (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	length = 0;
	map = nih_file_map (filename, O_RDONLY, &length);

	/* Return value should not be NULL */
	if (map == NULL) {
		printf ("BAD: return value wasn't what we expected.\n");
		ret = 1;
	}

	/* Length should be 5 chars */
	if (length != 5) {
		printf ("BAD: map length wasn't what we expected.\n");
		ret = 1;
	}

	/* Memory should contain file contents */
	if (strncmp (map, "test\n", 5)) {
		printf ("BAD: memory map wasn't what we expected.\n");
		ret = 1;
	}


	printf ("Testing nih_file_unmap()\n");
	nih_file_unmap (map, length);

	return 0;
}


int
main (int   argc,
      char *argv[])
{
	int ret = 0;

	ret |= test_add_watch ();
	ret |= test_map ();

	return ret;
}
