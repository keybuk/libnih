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
#include <nih/logging.h>


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

	nih_free (visited);


	/* Check that we get a ENOTDIR error if we try and walk a file.
	 */
	TEST_FEATURE ("with non-directory");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	visitor_called = 0;

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

	strcpy (filename, dirname);
	strcat (filename, "/frodo/baggins");
	unlink (filename);

	strcpy (filename, dirname);
	strcat (filename, "/frodo");
	rmdir (filename);

	rmdir (dirname);
}


static int create_called = 0;
static int modify_called = 0;
static int delete_called = 0;
static NihDirWatch *last_dir_watch = NULL;
static char *last_path = NULL;

static void
my_create_handler (void        *data,
		   NihDirWatch *watch,
		   const char  *path)
{
	create_called++;
	last_data = data;
	last_dir_watch = watch;
	if (last_path) {
		char *old;

		old = last_path;
		last_path = nih_sprintf (NULL, "%s::%s", old, path);
		nih_free (old);
	} else {
		last_path = nih_strdup (NULL, path);
	}
}

static void
my_modify_handler (void        *data,
		   NihDirWatch *watch,
		   const char  *path)
{
	modify_called++;
	last_data = data;
	last_dir_watch = watch;
	if (last_path)
		nih_free (last_path);

	last_path = nih_strdup (NULL, path);
}

static void
my_delete_handler (void        *data,
		   NihDirWatch *watch,
		   const char  *path)
{
	delete_called++;
	last_data = data;
	last_dir_watch = watch;
	if (last_path)
		nih_free (last_path);

	if (path) {
		last_path = nih_strdup (NULL, path);
	} else {
		last_path = NULL;
	}
}

static int
my_logger (NihLogLevel  priority,
	   const char  *message)
{
	return 0;
}

void
test_dir_add_watch (void)
{
	FILE         *fd;
	char          dirname[PATH_MAX], filename[PATH_MAX];
	NihDirWatch  *watch;
	NihFileWatch *fwatch;
	NihList      *watches;
	NihError     *err;

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

	/* Naughty way of grabbing the file watch list */
	fwatch = nih_file_add_watch (NULL, filename, IN_MODIFY,
				     my_watcher, NULL);
	watches = fwatch->entry.prev;
	nih_free (fwatch);


	/* Check that we can add a watch on the top-level directory of a
	 * tree.  We should get a structure returned that's correctly filled
	 * in, we should also be able to see individual file watches on each
	 * directory within it.
	 *
	 * Anything matching the filter should be excluded.
	 */
	TEST_FEATURE ("with top-level directory");
	watch = nih_dir_add_watch (NULL, dirname, TRUE, my_filter,
				   my_create_handler, my_modify_handler,
				   my_delete_handler, &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihDirWatch));
	TEST_ALLOC_PARENT (watch->path, watch);
	TEST_ALLOC_SIZE (watch->path, strlen (dirname) + 1);
	TEST_EQ_STR (watch->path, dirname);
	TEST_EQ (watch->subdirs, TRUE);
	TEST_EQ_P (watch->filter, my_filter);
	TEST_EQ_P (watch->create_handler, my_create_handler);
	TEST_EQ_P (watch->modify_handler, my_modify_handler);
	TEST_EQ_P (watch->delete_handler, my_delete_handler);
	TEST_EQ_P (watch->data, &watch);

	TEST_LIST_NOT_EMPTY (watches);

	fwatch = (NihFileWatch *)watches->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihFileWatch));
	TEST_ALLOC_PARENT (fwatch, watch);
	TEST_EQ_STR (fwatch->path, dirname);
	nih_list_remove (&fwatch->entry);

	fwatch = (NihFileWatch *)watches->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihFileWatch));
	TEST_ALLOC_PARENT (fwatch, watch);
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	TEST_EQ_STR (fwatch->path, filename);
	nih_list_remove (&fwatch->entry);

	fwatch = (NihFileWatch *)watches->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihFileWatch));
	TEST_ALLOC_PARENT (fwatch, watch);
	strcpy (filename, dirname);
	strcat (filename, "/baz");
	TEST_EQ_STR (fwatch->path, filename);
	nih_list_remove (&fwatch->entry);

	TEST_LIST_EMPTY (watches);

	nih_free (watch);


	/* Check that we can add a watch without sub-directories included
	 * in the tree; we should only get a watch on the top-level.
	 */
	TEST_FEATURE ("with no sub-directories");
	watch = nih_dir_add_watch (NULL, dirname, FALSE, NULL,
				   my_create_handler, my_modify_handler,
				   my_delete_handler, &watch);

	TEST_ALLOC_SIZE (watch, sizeof (NihDirWatch));
	TEST_ALLOC_PARENT (watch->path, watch);
	TEST_ALLOC_SIZE (watch->path, strlen (dirname) + 1);
	TEST_EQ_STR (watch->path, dirname);
	TEST_EQ (watch->subdirs, FALSE);

	TEST_LIST_NOT_EMPTY (watches);

	fwatch = (NihFileWatch *)watches->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihFileWatch));
	TEST_ALLOC_PARENT (fwatch, watch);
	TEST_EQ_STR (fwatch->path, dirname);
	nih_list_remove (&fwatch->entry);

	TEST_LIST_EMPTY (watches);

	nih_free (watch);


	/* Check that we get a ENOTDIR error if we try and watch something
	 * that's not a directory.
	 */
	TEST_FEATURE ("with non-directory");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	watch = nih_dir_add_watch (NULL, filename, TRUE, NULL,
				   NULL, NULL, NULL, &watch);

	TEST_EQ_P (watch, NULL);

	TEST_LIST_EMPTY (watches);

	err = nih_error_get ();
	TEST_EQ (err->number, ENOTDIR);
	nih_free (err);


	/* Check that a failure to walk the tree results in all of the
	 * watches being aborted, as we're in an inconsitent state.
	 */
	TEST_FEATURE ("with error in sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/baz");

	chmod (filename, 0000);

	nih_log_set_logger (my_logger);
	watch = nih_dir_add_watch (NULL, dirname, TRUE, NULL,
				   my_create_handler, my_modify_handler,
				   my_delete_handler, &watch);
	nih_log_set_logger (nih_logger_printf);

	TEST_EQ_P (watch, NULL);

	TEST_LIST_EMPTY (watches);

	err = nih_error_get ();
	TEST_EQ (err->number, EACCES);
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

	strcpy (filename, dirname);
	strcat (filename, "/frodo/baggins");
	unlink (filename);

	strcpy (filename, dirname);
	strcat (filename, "/frodo");
	rmdir (filename);

	rmdir (dirname);
}

static int destructor_called = 0;

static int
my_destructor (void *ptr)
{
	destructor_called++;

	return 0;
}

void
test_dir_watcher (void)
{
	FILE         *fd;
	char          dirname[PATH_MAX], filename[PATH_MAX], newname[PATH_MAX];
	NihDirWatch  *watch;
	NihFileWatch *fwatch;
	NihList      *watches;
	fd_set        readfds, writefds, exceptfds;
	int           nfds;

	TEST_FUNCTION ("nih_dir_watcher");
	TEST_FILENAME (dirname);
	mkdir (dirname, 0755);

	/* Naughty way of grabbing the file watch list */
	fwatch = nih_file_add_watch (NULL, dirname, IN_MODIFY,
				     my_watcher, NULL);
	watches = fwatch->entry.prev;
	nih_free (fwatch);

	watch = nih_dir_add_watch (NULL, dirname, TRUE, NULL,
				   my_create_handler, my_modify_handler,
				   my_delete_handler, &watch);


	/* Check that the create handler is called if a new file is
	 * created in the directory, and is passed the full path.
	 */
	TEST_FEATURE ("with creation of a file");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	create_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	nih_free (last_path);


	/* Check that the modify handler is called if we modify the file. */
	TEST_FEATURE ("with modification to a file");
	fd = fopen (filename, "w");
	fprintf (fd, "another test\n");
	fclose (fd);

	modify_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (modify_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	nih_free (last_path);


	/* Check that both delete and create are called if we rename the
	 * file to something else.
	 */
	TEST_FEATURE ("with renaming of a file");
	strcpy (newname, dirname);
	strcat (newname, "/bar");

	rename (filename, newname);

	delete_called = 0;
	create_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_TRUE (create_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);

	strcpy (newname, dirname);
	strcat (newname, "/foo::");
	strcat (newname, dirname);
	strcat (newname, "/bar");
	TEST_EQ_STR (last_path, newname);

	nih_free (last_path);


	/* Check that we just get delete called if we delete it. */
	TEST_FEATURE ("with deletion of a file");
	strcpy (filename, dirname);
	strcat (filename, "/bar");

	unlink (filename);

	delete_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	nih_free (last_path);


	/* Check that creating a sub-directory when subdirs is TRUE results
	 * in a watch being added for that sub-directory, as well as the
	 * create handler being called.
	 */
	TEST_FEATURE ("with creation of a sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/meep");

	mkdir (filename, 0755);

	create_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	fwatch = (NihFileWatch *)watches->next->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihFileWatch));
	TEST_ALLOC_PARENT (fwatch, watch);
	TEST_EQ_STR (fwatch->path, filename);

	nih_free (last_path);


	/* Check that a sub-directory being moved results in it being
	 * deleted and created again.
	 */
	TEST_FEATURE ("with move of a sub-directory");
	strcpy (newname, dirname);
	strcat (newname, "/mope");

	rename (filename, newname);

	delete_called = 0;
	create_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_TRUE (create_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	strcpy (newname, dirname);
	strcat (newname, "/meep::");
	strcat (newname, dirname);
	strcat (newname, "/mope");
	TEST_EQ_STR (last_path, newname);

	strcpy (filename, dirname);
	strcat (filename, "/mope");

	fwatch = (NihFileWatch *)watches->next->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihFileWatch));
	TEST_ALLOC_PARENT (fwatch, watch);
	TEST_EQ_STR (fwatch->path, filename);

	nih_free (last_path);


	/* Check that the create handler is called if we create a file
	 * inside that directory.
	 */
	TEST_FEATURE ("with creation of a file in subdir");
	strcpy (filename, dirname);
	strcat (filename, "/mope/whee");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	create_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	nih_free (last_path);


	/* Check that we just get delete called if we delete a file in a
	 * sub-directory.
	 */
	TEST_FEATURE ("with deletion of a file from subdir");
	unlink (filename);

	delete_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	nih_free (last_path);


	/* Check that we just get delete called if we remove the sub-directory
	 * itself, we should also lose the watch on it.
	 */
	TEST_FEATURE ("with deletion of sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/mope");

	rmdir (filename);

	delete_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_STR (last_path, filename);

	fwatch = (NihFileWatch *)watches->next->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihList));

	nih_free (last_path);


	/* Check that delete is called with the special NULL argument if
	 * we remove the top-level directory, that we lose the watch on
	 * it, and that the structure itself is freed.
	 */
	TEST_FEATURE ("with deletion of top-level directory");
	rmdir (dirname);

	destructor_called = 0;
	nih_alloc_set_destructor (watch, my_destructor);

	delete_called = 0;
	last_data = NULL;
	last_dir_watch = NULL;
	last_path = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_data, &watch);
	TEST_EQ_P (last_dir_watch, watch);
	TEST_EQ_P (last_path, NULL);

	fwatch = (NihFileWatch *)watches->next;
	TEST_ALLOC_SIZE (fwatch, sizeof (NihList));

	TEST_TRUE (destructor_called);
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
	test_dir_add_watch ();
	test_dir_watcher ();
	test_map ();
	test_unmap ();

	return 0;
}
