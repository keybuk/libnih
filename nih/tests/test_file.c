/* libnih
 *
 * test_file.c - test suite for nih/file.c
 *
 * Copyright © 2007 Scott James Remnant <scott@netsplit.com>.
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

#include <sys/mman.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/io.h>
#include <nih/file.h>
#include <nih/error.h>


static int watcher_called = 0;
static void *last_data = NULL;
static NihFileWatch *last_watch = NULL;
static uint32_t last_events = 0;
static uint32_t last_cookie = 0;
static const char *last_name = NULL;

static void
my_watcher (void         *data,
	    NihFileWatch *watch,
	    uint32_t      events,
	    uint32_t      cookie,
	    const char   *name)
{
	watcher_called++;
	last_data = data;
	last_watch = watch;
	last_events = events;
	last_cookie = cookie;
	last_name = name ? nih_strdup (watch, name) : NULL;
}

void
test_add_watch (void)
{
	NihFileWatch *watch;
	FILE         *fd;
	fd_set        readfds, writefds, exceptfds;
	char          filename[PATH_MAX], dirname[PATH_MAX], newname[PATH_MAX];
	int           nfds = 0;

	TEST_FUNCTION ("nih_file_add_watch");

	/* Check that we can add a watch on a filename, and that the returned
	 * structure is all filled out correctly.
	 */
	TEST_FEATURE ("with file");
	TEST_FILENAME (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	watch = nih_file_add_watch (NULL, filename, IN_OPEN, my_watcher,
				    &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihFileWatch));
	TEST_GE (watch->wd, 0);
	TEST_EQ_STR (watch->path, filename);
	TEST_ALLOC_PARENT (watch->path, watch);
	TEST_EQ (watch->events, IN_OPEN);
	TEST_EQ_P (watch->watcher, my_watcher);
	TEST_EQ_P (watch->data, &watch);
	TEST_LIST_NOT_EMPTY (&watch->entry);

	/* Check that a modification to that file results in the watcher
	 * being called with appropriate parameters.
	 */
	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	last_cookie = 0;
	last_name = NULL;

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (watcher_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_watch, watch);
	TEST_TRUE (last_events & IN_OPEN);
	TEST_EQ (last_cookie, 0);
	TEST_EQ_P (last_name, NULL);

	nih_free (watch);
	unlink (filename);


	/* Check that we can watch a directory, and get notified when files
	 * are created within it.
	 */
	TEST_FEATURE ("with directory");
	strcpy (dirname, filename);
	strcat (filename, "/foo");
	mkdir (dirname, 0700);

	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	last_cookie = 0;
	last_name = NULL;

	watch = nih_file_add_watch (NULL, dirname, IN_CREATE,
				    my_watcher, &watch);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (watcher_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_watch, watch);
	TEST_TRUE (last_events & IN_CREATE);
	TEST_EQ (last_cookie, 0);
	TEST_EQ_STR (last_name, "foo");

	nih_free (watch);
	unlink (filename);


	/* Check that a rename of a file within a directory has the cookie
	 * set.
	 */
	TEST_FEATURE ("with rename");
	watcher_called = 0;
	last_data = NULL;
	last_watch = NULL;
	last_events = 0;
	last_cookie = 0;
	last_name = NULL;

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	watch = nih_file_add_watch (NULL, dirname, IN_MOVE,
				    my_watcher, &watch);

	strcpy (newname, dirname);
	strcat (newname, "/bar");

	rename (filename, newname);

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_EQ (watcher_called, 2);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_watch, watch);
	TEST_TRUE (last_events & IN_MOVED_TO);
	TEST_NE (last_cookie, 0);
	TEST_EQ_STR (last_name, "bar");

	nih_free (watch);
	unlink (newname);
	rmdir (dirname);
}


typedef struct visited {
	NihList  entry;

	void    *data;
	char    *path;
} Visited;

static NihList *visited = NULL;
static int visitor_called = 0;

static int
my_visitor (void       *data,
	    const char *path)
{
	Visited *v;

	visitor_called++;

	v = nih_new (visited, Visited);
	nih_list_init (&v->entry);

	v->data = data;
	v->path = nih_strdup (v, path);

	nih_list_add (visited, &v->entry);

	if (data == (void *)-1) {
		errno = EINVAL;
		nih_return_system_error (-1);
	}

	return 0;
}

static int
my_filter (const char *path)
{
	char *slash;

	slash = strrchr (path, '/');
	if (! strcmp (slash, "/frodo"))
		return TRUE;

	return FALSE;
}

void
test_dir_walk (void)
{
	FILE     *fd;
	char      dirname[PATH_MAX], filename[PATH_MAX];
	int       ret;
	Visited  *v;
	NihError *err;

	TEST_FUNCTION ("nih_dir_walk");
	TEST_FILENAME (dirname);
	mkdir (dirname, 0755);

	strcpy (filename, dirname);
	strcat (filename, "/foo");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	strcpy (filename, dirname);
	strcat (filename, "/bar");

	mkdir (filename, 0755);

	strcpy (filename, dirname);
	strcat (filename, "/bar/frodo");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	strcpy (filename, dirname);
	strcat (filename, "/bar/bilbo");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	strcpy (filename, dirname);
	strcat (filename, "/baz");

	mkdir (filename, 0755);

	strcpy (filename, dirname);
	strcat (filename, "/frodo");
	mkdir (filename, 0755);

	strcpy (filename, dirname);
	strcat (filename, "/frodo/baggins");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);


	/* Check that both directories and files can be visited with no
	 * filter, the visitor should be called for each one and have the
	 * correct data pointer and path called.
	 */
	TEST_FEATURE ("with both dirs and files and no filter");
	visitor_called = 0;
	visited = nih_list_new (NULL);

	ret = nih_dir_walk (dirname, S_IFREG | S_IFDIR, NULL,
			    my_visitor, &ret);

	TEST_EQ (ret, 0);
	TEST_EQ (visitor_called, 7);

	v = (Visited *)visited->next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/foo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar/frodo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar/bilbo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/baz");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/frodo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/frodo/baggins");
	TEST_EQ_STR (v->path, filename);

	nih_free (visited);


	/* Check that only directories can be visited by omitting S_IFREG
	 * from the types list.
	 */
	TEST_FEATURE ("with only dirs and no filter");
	visitor_called = 0;
	visited = nih_list_new (NULL);

	ret = nih_dir_walk (dirname, S_IFDIR, NULL, my_visitor, &ret);

	TEST_EQ (ret, 0);
	TEST_EQ (visitor_called, 3);

	v = (Visited *)visited->next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/baz");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/frodo");
	TEST_EQ_STR (v->path, filename);

	nih_free (visited);


	/* Check that only files can be visited by omitting S_IFDIR from
	 * the types list.  Sub-directories should still be descended into.
	 */
	TEST_FEATURE ("with only files and no filter");
	visitor_called = 0;
	visited = nih_list_new (NULL);

	ret = nih_dir_walk (dirname, S_IFREG, NULL, my_visitor, &ret);

	TEST_EQ (ret, 0);
	TEST_EQ (visitor_called, 4);

	v = (Visited *)visited->next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/foo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar/frodo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar/bilbo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/frodo/baggins");
	TEST_EQ_STR (v->path, filename);

	nih_free (visited);


	/* Check that a filter can be used to restrict the names of
	 * objects visited and descended into.
	 */
	TEST_FEATURE ("with filter");
	visitor_called = 0;
	visited = nih_list_new (NULL);

	ret = nih_dir_walk (dirname, S_IFREG | S_IFDIR, my_filter,
			    my_visitor, &ret);

	TEST_EQ (ret, 0);
	TEST_EQ (visitor_called, 4);

	v = (Visited *)visited->next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/foo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/bar/bilbo");
	TEST_EQ_STR (v->path, filename);

	v = (Visited *)v->entry.next;
	TEST_EQ (v->data, &ret);
	strcpy (filename, dirname);
	strcat (filename, "/baz");
	TEST_EQ_STR (v->path, filename);

	nih_free (visited);


	/* Check that the return value and error from the visitor is
	 * returned, and no further objects are visited.
	 */
	TEST_FEATURE ("with error in visitor");
	visitor_called = 0;
	visited = nih_list_new (NULL);

	ret = nih_dir_walk (dirname, S_IFREG | S_IFDIR, my_filter,
			    my_visitor, (void *)-1);

	TEST_EQ (ret, -1);
	TEST_EQ (visitor_called, 1);

	err = nih_error_get ();
	TEST_EQ (err->number, EINVAL);
	nih_free (err);


	/* Check that we get a ENOTDIR error if we try and walk a file.
	 */
	TEST_FEATURE ("with non-directory");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	visitor_called = 0;
	visited = nih_list_new (NULL);

	ret = nih_dir_walk (filename, S_IFREG | S_IFDIR, my_filter,
			    my_visitor, &ret);

	TEST_EQ (ret, -1);
	TEST_EQ (visitor_called, 0);

	err = nih_error_get ();
	TEST_EQ (err->number, ENOTDIR);
	nih_free (err);


	strcpy (filename, dirname);
	strcat (filename, "/foo");
	unlink (filename);

	strcpy (filename, dirname);
	strcat (filename, "/bar/frodo");
	unlink (filename);

	strcpy (filename, dirname);
	strcat (filename, "/bar/bilbo");
	unlink (filename);

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	rmdir (filename);

	strcpy (filename, dirname);
	strcat (filename, "/baz");
	rmdir (filename);

	rmdir (dirname);
}

void
test_map (void)
{
	FILE  *fd;
	char   filename[PATH_MAX], text[80], *map;
	size_t length;

	TEST_FUNCTION ("nih_file_map");

	/* Check that we can map a file into memory for reading, and that
	 * the memory contents match the file.
	 */
	TEST_FEATURE ("with read mode");
	TEST_FILENAME (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	length = 0;
	map = nih_file_map (filename, O_RDONLY, &length);

	TEST_NE_P (map, NULL);
	TEST_EQ (length, 5);
	TEST_EQ_MEM (map, "test\n", 5);

	munmap (map, length);
	unlink (filename);


	/* Check that we can map a file for both reading and writing, the
	 * memory contents should match the file.
	 */
	TEST_FEATURE ("with read/write mode");
	TEST_FILENAME (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	length = 0;
	map = nih_file_map (filename, O_RDWR, &length);

	TEST_NE_P (map, NULL);
	TEST_EQ (length, 5);
	TEST_EQ_MEM (map, "test\n", 5);

	/* Check that we can alter the memory at that address, and have the
	 * file altered.
	 */
	memcpy (map, "cool\n", 5);
	TEST_EQ_MEM (map, "cool\n", 5);

	munmap (map, length);
	fd = fopen (filename, "r");

	fgets (text, sizeof (text), fd);
	TEST_EQ_STR (text, "cool\n");

	fclose (fd);
	unlink (filename);
}

void
test_unmap (void)
{
	FILE  *fd;
	char   filename[PATH_MAX], *map;
	size_t length;
	int    ret;

	TEST_FUNCTION ("nih_file_unmap");

	/* Check that we can unmap a file that we mapped with nih_map.
	 * Mostly just make sure it returns zero.
	 */
	TEST_FILENAME (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	length = 0;
	map = nih_file_map (filename, O_RDONLY, &length);
	ret = nih_file_unmap (map, length);

	TEST_EQ (ret, 0);

	unlink (filename);
}


int
main (int   argc,
      char *argv[])
{
	test_add_watch ();
	test_dir_walk ();
	test_map ();
	test_unmap ();

	return 0;
}
