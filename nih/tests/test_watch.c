/* libnih
 *
 * test_watch.c - test suite for nih/watch.c
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

#include <sys/inotify.h>
#include <sys/select.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/io.h>
#include <nih/file.h>
#include <nih/watch.h>
#include <nih/error.h>
#include <nih/logging.h>


static int
my_filter (void       *data,
	   const char *path,
	   int         is_dir)
{
	char *slash;

	slash = strrchr (path, '/');
	if (! strcmp (slash, "/frodo"))
		return TRUE;

	return FALSE;
}

static int create_called = 0;
static int modify_called = 0;
static int delete_called = 0;
static NihWatch *last_watch = NULL;
static char *last_path = NULL;
static void *last_data = NULL;

static void
my_create_handler (void        *data,
		   NihWatch    *watch,
		   const char  *path,
		   struct stat *statbuf)
{
	create_called++;
	last_data = data;
	last_watch = watch;

	if (last_path) {
		char *old;

		old = last_path;
		last_path = NIH_MUST (nih_sprintf (NULL, "%s::%s", old, path));
		nih_free (old);
	} else {
		last_path = NIH_MUST (nih_strdup (NULL, path));
	}
}

static void
my_modify_handler (void        *data,
		   NihWatch    *watch,
		   const char  *path,
		   struct stat *statbuf)
{
	modify_called++;
	last_data = data;
	last_watch = watch;

	if (last_path)
		nih_free (last_path);

	last_path = NIH_MUST (nih_strdup (NULL, path));
}

static void
my_delete_handler (void       *data,
		   NihWatch   *watch,
		   const char *path)
{
	delete_called++;
	last_data = data;
	last_watch = watch;

	if (last_path)
		nih_free (last_path);

	if (path) {
		last_path = NIH_MUST (nih_strdup (NULL, path));
		if (! strcmp (path, watch->path))
			nih_free (watch);
	} else {
		last_path = NULL;
	}
}

static int logger_called = 0;

static int
my_logger (NihLogLevel  priority,
	   const char  *message)
{
	logger_called++;

	return 0;
}

void
test_new (void)
{
	FILE           *fd;
	char            dirname[PATH_MAX], filename[PATH_MAX];
	NihWatch       *watch;
	NihWatchHandle *handle;
	NihError       *err;

	TEST_FUNCTION ("nih_watch_new");
	nih_io_init ();

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


	/* Check that nih_watch_new returns a newly allocated structure with
	 * each of the members filled in; an inotify instance should have
	 * been added, and a watch on the parent stored in the watches list.
	 */
	TEST_FEATURE ("with file");
	nih_error_push_context ();
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/frodo/baggins");

		watch = nih_watch_new (NULL, filename, FALSE, FALSE, my_filter,
				       my_create_handler, my_modify_handler,
				       my_delete_handler, &watch);

		TEST_ALLOC_SIZE (watch, sizeof (NihWatch));
		TEST_ALLOC_SIZE (watch->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (watch->path, watch);
		TEST_EQ_STR (watch->path, filename);
		TEST_EQ (watch->subdirs, FALSE);
		TEST_EQ_P (watch->filter, my_filter);
		TEST_EQ_P (watch->create_handler, my_create_handler);
		TEST_EQ_P (watch->modify_handler, my_modify_handler);
		TEST_EQ_P (watch->delete_handler, my_delete_handler);
		TEST_ALLOC_SIZE (watch->created, sizeof (NihHash));
		TEST_ALLOC_PARENT (watch->created, watch);
		TEST_EQ_P (watch->data, &watch);

		TEST_GE (fcntl (watch->fd, F_GETFD), 0);

		TEST_ALLOC_SIZE (watch->io, sizeof (NihIo));
		TEST_ALLOC_PARENT (watch->io, watch);
		TEST_EQ (watch->io->type, NIH_IO_STREAM);
		TEST_EQ (watch->io->watch->fd, watch->fd);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);

		nih_free (watch);
	}
	nih_error_pop_context ();


	/* Check that if we add a sub-directory, but subdirs is FALSE, we
	 * only get a watch for that directory added.
	 */
	TEST_FEATURE ("with directory only");
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/frodo");

		watch = nih_watch_new (NULL, filename, FALSE, FALSE, my_filter,
				       my_create_handler, my_modify_handler,
				       my_delete_handler, &watch);

		TEST_ALLOC_SIZE (watch, sizeof (NihWatch));
		TEST_ALLOC_SIZE (watch->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (watch->path, watch);
		TEST_EQ_STR (watch->path, filename);
		TEST_EQ (watch->subdirs, FALSE);
		TEST_EQ_P (watch->filter, my_filter);
		TEST_EQ_P (watch->create_handler, my_create_handler);
		TEST_EQ_P (watch->modify_handler, my_modify_handler);
		TEST_EQ_P (watch->delete_handler, my_delete_handler);
		TEST_ALLOC_SIZE (watch->created, sizeof (NihHash));
		TEST_ALLOC_PARENT (watch->created, watch);
		TEST_EQ_P (watch->data, &watch);

		TEST_GE (fcntl (watch->fd, F_GETFD), 0);

		TEST_ALLOC_SIZE (watch->io, sizeof (NihIo));
		TEST_ALLOC_PARENT (watch->io, watch);
		TEST_EQ (watch->io->type, NIH_IO_STREAM);
		TEST_EQ (watch->io->watch->fd, watch->fd);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);

		nih_free (watch);
	}


	/* Check that if we add a directory with subdirs, we get a watch for
	 * each directory underneath (but not any files, or anything matching
	 * the filter).
	 */
	TEST_FEATURE ("with directory and sub-directories");
	TEST_ALLOC_FAIL {
		watch = nih_watch_new (NULL, dirname, TRUE, FALSE, my_filter,
				       my_create_handler, my_modify_handler,
				       my_delete_handler, &watch);

		TEST_ALLOC_SIZE (watch, sizeof (NihWatch));
		TEST_ALLOC_SIZE (watch->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (watch->path, watch);
		TEST_EQ_STR (watch->path, dirname);
		TEST_EQ (watch->subdirs, TRUE);
		TEST_EQ_P (watch->filter, my_filter);
		TEST_EQ_P (watch->create_handler, my_create_handler);
		TEST_EQ_P (watch->modify_handler, my_modify_handler);
		TEST_EQ_P (watch->delete_handler, my_delete_handler);
		TEST_ALLOC_SIZE (watch->created, sizeof (NihHash));
		TEST_ALLOC_PARENT (watch->created, watch);
		TEST_EQ_P (watch->data, &watch);

		TEST_GE (fcntl (watch->fd, F_GETFD), 0);

		TEST_ALLOC_SIZE (watch->io, sizeof (NihIo));
		TEST_ALLOC_PARENT (watch->io, watch);
		TEST_EQ (watch->io->type, NIH_IO_STREAM);
		TEST_EQ (watch->io->watch->fd, watch->fd);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, dirname);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/bar");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/baz");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);

		nih_free (watch);
	}


	/* Check that the create handler can be called for each file and
	 * directory found at the point we add things.
	 */
	TEST_FEATURE ("with create handler");
	TEST_ALLOC_FAIL {
		create_called = 0;
		last_watch = NULL;
		last_data = NULL;
		last_path = NULL;

		watch = nih_watch_new (NULL, dirname, TRUE, TRUE, my_filter,
				       my_create_handler, my_modify_handler,
				       my_delete_handler, &watch);

		TEST_ALLOC_SIZE (watch, sizeof (NihWatch));
		TEST_ALLOC_SIZE (watch->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (watch->path, watch);
		TEST_EQ_STR (watch->path, dirname);
		TEST_EQ (watch->subdirs, TRUE);
		TEST_EQ_P (watch->filter, my_filter);
		TEST_EQ_P (watch->create_handler, my_create_handler);
		TEST_EQ_P (watch->modify_handler, my_modify_handler);
		TEST_EQ_P (watch->delete_handler, my_delete_handler);
		TEST_ALLOC_SIZE (watch->created, sizeof (NihHash));
		TEST_ALLOC_PARENT (watch->created, watch);
		TEST_EQ_P (watch->data, &watch);

		TEST_GE (fcntl (watch->fd, F_GETFD), 0);

		TEST_ALLOC_SIZE (watch->io, sizeof (NihIo));
		TEST_ALLOC_PARENT (watch->io, watch);
		TEST_EQ (watch->io->type, NIH_IO_STREAM);
		TEST_EQ (watch->io->watch->fd, watch->fd);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, dirname);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/bar");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/baz");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);


		TEST_EQ (create_called, 4);
		TEST_EQ (last_data, &watch);
		TEST_EQ (last_watch, watch);

		strcpy (filename, dirname);
		strcat (filename, "/bar");
		strcat (filename, "::");
		strcat (filename, dirname);
		strcat (filename, "/bar/bilbo");
		strcat (filename, "::");
		strcat (filename, dirname);
		strcat (filename, "/baz");
		strcat (filename, "::");
		strcat (filename, dirname);
		strcat (filename, "/foo");

		TEST_EQ_STR (last_path, filename);
		nih_free (last_path);

		nih_free (watch);
	}


	/* Check that an error with the path given results in an error
	 * being raised and NULL returned.
	 */
	TEST_FEATURE ("with non-existant path");
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/drogo");

		watch = nih_watch_new (NULL, filename, TRUE, FALSE, my_filter,
				       my_create_handler, my_modify_handler,
				       my_delete_handler, &watch);

		TEST_EQ_P (watch, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, ENOENT);
		nih_free (err);
	}


	/* Check that an error with a sub-directory results in a warning
	 * being emitted, but the directory recursing carrying on.
	 */
	TEST_FEATURE ("with error with sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0000);

	TEST_ALLOC_FAIL {
		logger_called = 0;
		nih_log_set_logger (my_logger);

		watch = nih_watch_new (NULL, dirname, TRUE, FALSE, NULL,
				       my_create_handler, my_modify_handler,
				       my_delete_handler, &watch);

		nih_log_set_logger (nih_logger_printf);

		TEST_TRUE (logger_called);

		TEST_ALLOC_SIZE (watch, sizeof (NihWatch));
		TEST_ALLOC_SIZE (watch->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (watch->path, watch);
		TEST_EQ_STR (watch->path, dirname);
		TEST_ALLOC_SIZE (watch->created, sizeof (NihHash));
		TEST_ALLOC_PARENT (watch->created, watch);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, dirname);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/baz");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/frodo");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);

		nih_free (watch);
	}

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0755);


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


void
test_add (void)
{
	FILE           *fd;
	char            dirname[PATH_MAX], filename[PATH_MAX];
	NihWatch       *watch;
	NihWatchHandle *handle;
	NihError       *err;
	int             ret;

	TEST_FUNCTION ("nih_watch_add");
	nih_error_init ();

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

	strcpy (filename, dirname);
	strcat (filename, "/frodo/baggins");

	watch = nih_watch_new (NULL, filename, FALSE, FALSE, my_filter,
			       my_create_handler, my_modify_handler,
			       my_delete_handler, &watch);

	handle = (NihWatchHandle *)watch->watches.next;
	nih_list_remove (&handle->entry);


	/* Check that we can add a single path to an existing watch, and
	 * have a new handle added with the the appropriate details.
	 */
	TEST_FEATURE ("with file");
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/bar/bilbo");

		ret = nih_watch_add (watch, filename, TRUE);

		TEST_EQ (ret, 0);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);
	}


	/* Check that if we add a sub-directory, but subdirs is FALSE, we
	 * only get a watch handle for that directory added.
	 */
	TEST_FEATURE ("with directory only");
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/frodo");

		ret = nih_watch_add (watch, filename, FALSE);

		TEST_EQ (ret, 0);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);
	}


	/* Check that if we add a directory with subdirs, we get a watch for
	 * each directory underneath (but not any files, or anything matching
	 * the filter).
	 */
	TEST_FEATURE ("with directory and sub-directories");
	TEST_ALLOC_FAIL {
		ret = nih_watch_add (watch, dirname, TRUE);

		TEST_EQ (ret, 0);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, dirname);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/bar");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/baz");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);
	}


	/* Check that repeated call with the same path does not increase the
	 * size of the watches list.
	 */
	TEST_FEATURE ("with path already being watched");
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/frodo/baggins");

		ret = nih_watch_add (watch, filename, FALSE);

		TEST_EQ (ret, 0);

		ret = nih_watch_add (watch, filename, FALSE);

		TEST_EQ (ret, 0);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);
	}


	/* Check that an error with the path given results in an error
	 * being raised and NULL returned.
	 */
	TEST_FEATURE ("with non-existant path");
	TEST_ALLOC_FAIL {
		strcpy (filename, dirname);
		strcat (filename, "/drogo");

		ret = nih_watch_add (watch, filename, TRUE);

		TEST_LT (ret, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, ENOENT);
		nih_free (err);
	}


	/* Check that an error with a sub-directory results in a warning
	 * being emitted, but the directory recursing carrying on.
	 */
	TEST_FEATURE ("with error with sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0000);

	TEST_ALLOC_FAIL {
		logger_called = 0;
		nih_log_set_logger (my_logger);

		ret = nih_watch_add (watch, dirname, TRUE);

		nih_log_set_logger (nih_logger_printf);

		TEST_TRUE (logger_called);

		TEST_LIST_NOT_EMPTY (&watch->watches);

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (dirname) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, dirname);

		nih_list_remove (&handle->entry);

		strcpy (filename, dirname);
		strcat (filename, "/baz");

		handle = (NihWatchHandle *)watch->watches.next;
		TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
		TEST_ALLOC_PARENT (handle, watch);

		TEST_ALLOC_SIZE (handle->path, strlen (filename) + 1);
		TEST_ALLOC_PARENT (handle->path, handle);
		TEST_EQ_STR (handle->path, filename);

		nih_list_remove (&handle->entry);

		TEST_LIST_EMPTY (&watch->watches);
	}

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0755);


	nih_free (watch);


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


void
test_destroy (void)
{
	NihWatch *watch;
	int       ret, fd, caught_free;

	/* Check that the free flag is set and that the inotify descriptor
	 * is closed.
	 */
	TEST_FUNCTION ("nih_watch_destroy");
	watch = nih_watch_new (NULL, "/", FALSE, FALSE, NULL,
			       NULL, NULL, NULL, NULL);
	fd = watch->fd;

	caught_free = FALSE;
	watch->free = &caught_free;

	ret = nih_free (watch);

	TEST_EQ (ret, 0);

	TEST_LT (fcntl (fd, F_GETFD), 0);
	TEST_EQ (errno, EBADF);

	TEST_TRUE (caught_free);
}


void
test_reader (void)
{
	FILE           *fd;
	NihWatch       *watch;
	NihWatchHandle *handle, *ptr;
	char            dirname[PATH_MAX], filename[PATH_MAX];
	char            newname[PATH_MAX];
	fd_set          readfds, writefds, exceptfds;
	int             nfds = 0;

	TEST_FUNCTION ("nih_watch_reader");
	nih_error_init ();

	TEST_FILENAME (dirname);
	mkdir (dirname, 0755);

	watch = nih_watch_new (NULL, dirname, TRUE, TRUE, my_filter,
			       my_create_handler, my_modify_handler,
			       my_delete_handler, &watch);


	/* Check that creating a file within the directory being watched
	 * results in the create handler being called, and passed the full
	 * path of the created file to it.
	 */
	TEST_FEATURE ("with new file");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	create_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);


	/* Check that a new file opened on disk doesn't result in the create
	 * handler being called until the file has been closed.
	 */
	TEST_FEATURE ("with new still-open file");
	strcpy (filename, dirname);
	strcat (filename, "/meep");

	create_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	fd = fopen (filename, "w");

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FALSE (create_called);

	TEST_NE_P (nih_hash_lookup (watch->created, filename), NULL);

	fprintf (fd, "test\n");
	fclose (fd);

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);
	last_path = NULL;
	unlink (filename);

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	nih_free (last_path);


	/* Check that removing a file that was newly created but then
	 * immediately removed doesn't get a handler at all.
	 */
	TEST_FEATURE ("with removal of still-open file");
	strcpy (filename, dirname);
	strcat (filename, "/flep");

	create_called = 0;
	modify_called = 0;
	delete_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	fd = fopen (filename, "w");

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FALSE (create_called);
	TEST_FALSE (modify_called);
	TEST_FALSE (delete_called);

	TEST_NE_P (nih_hash_lookup (watch->created, filename), NULL);

	unlink (filename);

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FALSE (create_called);
	TEST_FALSE (modify_called);
	TEST_FALSE (delete_called);

	TEST_EQ_P (nih_hash_lookup (watch->created, filename), NULL);

	fclose (fd);


	/* Check that modifying that file results in the modify handler
	 * being called and passed the full path of the created file.
	 */
	TEST_FEATURE ("with modified file");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	fd = fopen (filename, "w");
	fprintf (fd, "further test\n");
	fclose (fd);

	modify_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (modify_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);


	/* Check that we can rename the file, we should get the delete
	 * handler called followed by the create handler.
	 */
	TEST_FEATURE ("with renamed file");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	strcpy (newname, dirname);
	strcat (newname, "/bar");

	rename (filename, newname);

	delete_called = 0;
	create_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	strcpy (filename, dirname);
	strcat (filename, "/foo");
	strcat (filename, "::");
	strcat (filename, dirname);
	strcat (filename, "/bar");

	TEST_TRUE (delete_called);
	TEST_TRUE (create_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);


	/* Check that deleting the file results in the delete handler
	 * being called and passed the full filename.
	 */
	TEST_FEATURE ("with deleted file");
	strcpy (filename, dirname);
	strcat (filename, "/bar");

	unlink (filename);

	delete_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);


	/* Check that if we create a file that matches the filter, the handler
	 * is not called for it.
	 */
	TEST_FEATURE ("with filtered file");
	strcpy (filename, dirname);
	strcat (filename, "/frodo");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	fd = fopen (filename, "w");
	fprintf (fd, "another test\n");
	fclose (fd);

	unlink (filename);

	create_called = 0;
	modify_called = 0;
	delete_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_FALSE (create_called);
	TEST_FALSE (modify_called);
	TEST_FALSE (delete_called);


	/* Check that we can create a new directory, and given that subdirs
	 * is TRUE, have a new watch added for that directory automatically.
	 */
	TEST_FEATURE ("with new sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/bleep");

	mkdir (filename, 0755);

	create_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);

	ptr = (NihWatchHandle *)watch->watches.next;
	nih_list_remove (&ptr->entry);

	TEST_LIST_NOT_EMPTY (&watch->watches);

	handle = (NihWatchHandle *)watch->watches.next;
	TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
	TEST_ALLOC_PARENT (handle, watch);

	TEST_EQ_STR (handle->path, filename);

	nih_list_remove (&handle->entry);

	TEST_LIST_EMPTY (&watch->watches);

	nih_list_add (&watch->watches, &ptr->entry);
	nih_list_add (&watch->watches, &handle->entry);


	/* Check that we can remove a watched sub-directory, and have it
	 * automatically handled with the handle going away.
	 */
	TEST_FEATURE ("with removal of sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/bleep");

	rmdir (filename);

	delete_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);

	ptr = (NihWatchHandle *)watch->watches.next;
	nih_list_remove (&ptr->entry);

	TEST_LIST_EMPTY (&watch->watches);

	nih_list_add (&watch->watches, &ptr->entry);


	/* Check that we can create a new directory with bad permissions,
	 * and have it warn that it cannot watch them.
	 */
	TEST_FEATURE ("with new unsearchable sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/splat");

	mkdir (filename, 0000);

	create_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	logger_called = 0;
	nih_log_set_logger (my_logger);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	nih_log_set_logger (nih_logger_printf);

	TEST_EQ (logger_called, 1);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);

	ptr = (NihWatchHandle *)watch->watches.next;
	nih_list_remove (&ptr->entry);

	TEST_LIST_EMPTY (&watch->watches);

	nih_list_add (&watch->watches, &ptr->entry);


	rmdir (filename);

	delete_called = 0;
	last_path = NULL;
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_NE_P (last_path, NULL);
	nih_free (last_path);


	/* Check that we can create a new directory, and given that subdirs
	 * and create are TRUE, have a new watch added automatically and
	 * create_handler called for all files in that directory.
	 */
	TEST_FEATURE ("with new sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/woo");

	mkdir (filename, 0755);

	strcpy (filename, dirname);
	strcat (filename, "/woo/whee");

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	create_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (create_called);
	TEST_EQ_P (last_watch, watch);

	strcpy (filename, dirname);
	strcat (filename, "/woo");
	strcat (filename, "::");
	strcat (filename, dirname);
	strcat (filename, "/woo/whee");
	TEST_EQ_STR (last_path, filename);
	TEST_EQ_P (last_data, &watch);

	nih_free (last_path);

	ptr = (NihWatchHandle *)watch->watches.next;
	nih_list_remove (&ptr->entry);

	TEST_LIST_NOT_EMPTY (&watch->watches);

	handle = (NihWatchHandle *)watch->watches.next;
	TEST_ALLOC_SIZE (handle, sizeof (NihWatchHandle));
	TEST_ALLOC_PARENT (handle, watch);

	strcpy (filename, dirname);
	strcat (filename, "/woo");

	TEST_EQ_STR (handle->path, filename);

	nih_list_remove (&handle->entry);

	TEST_LIST_EMPTY (&watch->watches);

	nih_list_add (&watch->watches, &ptr->entry);
	nih_list_add (&watch->watches, &handle->entry);


	strcpy (filename, dirname);
	strcat (filename, "/woo/whee");
	unlink (filename);

	strcpy (filename, dirname);
	strcat (filename, "/woo");
	rmdir (filename);

	delete_called = 0;
	last_path = NULL;
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);
	TEST_TRUE (delete_called);
	TEST_NE_P (last_path, NULL);
	nih_free (last_path);


	/* Check that we can handle the directory itself being deleted,
	 * the delete_handler should be called with the top-level path.
	 * It should be safe to delete the entire watch this way.
	 */
	TEST_FEATURE ("with removal of directory");
	rmdir (dirname);

	TEST_FREE_TAG (watch);

	delete_called = 0;
	last_watch = NULL;
	last_path = NULL;
	last_data = NULL;

	nfds = 0;
	FD_ZERO (&readfds);
	FD_ZERO (&writefds);
	FD_ZERO (&exceptfds);

	nih_io_select_fds (&nfds, &readfds, &writefds, &exceptfds);
	select (nfds, &readfds, &writefds, &exceptfds, NULL);
	nih_io_handle_fds (&readfds, &writefds, &exceptfds);

	TEST_TRUE (delete_called);
	TEST_EQ_P (last_watch, watch);
	TEST_EQ_STR (last_path, dirname);
	TEST_EQ_P (last_data, &watch);

	TEST_FREE (watch);


	nih_free (last_path);
}


int
main (int   argc,
      char *argv[])
{
	int fd;

	/* Make sure we have inotify before performing these tests */
	fd = inotify_init ();
	if (fd < 0) {
		printf ("SKIP: inotify not available\n");
		return 0;
	}

	test_new ();
	test_add ();
	test_destroy ();
	test_reader ();

	return 0;
}
