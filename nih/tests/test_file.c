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

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/file.h>
#include <nih/main.h>
#include <nih/logging.h>
#include <nih/error.h>
#include <nih/errors.h>


void
test_map (void)
{
	FILE     *fd;
	char      filename[PATH_MAX], text[80], *map;
	size_t    length;
	NihError *err;

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


	/* Check that if we try and map a non-existant file for reading, we
	 * get an error raised.
	 */
	TEST_FEATURE ("with non-existant file");
	length = 0;
	map = nih_file_map (filename, O_RDONLY, &length);

	TEST_EQ_P (map, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, ENOENT);
	nih_free (err);


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

	/* Check that we can unmap a file that we mapped with nih_map.
	 * Mostly just make sure it returns zero.
	 */
	TEST_FUNCTION ("nih_file_unmap");
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


typedef struct visited {
	NihList  entry;

	void    *data;
	char    *dirname;
	char    *path;
} Visited;

static NihList *visited = NULL;
static int visitor_called = 0;

static int
my_visitor (void        *data,
	    const char  *dirname,
	    const char  *path,
	    struct stat *statbuf)
{
	Visited *v;

	visitor_called++;

	TEST_ALLOC_SAFE {
		v = nih_new (visited, Visited);
		nih_list_init (&v->entry);

		v->data = data;
		v->dirname = nih_strdup (v, dirname);
		v->path = nih_strdup (v, path);

		nih_list_add (visited, &v->entry);
	}

	if (data == (void *)-1) {
		errno = EINVAL;
		nih_return_system_error (-1);
	}

	return 0;
}

static int error_called = 0;
static char *last_error_path = NULL;
static int last_error = -1;

static int
my_error_handler (void        *data,
		  const char  *dirname,
		  const char  *path,
		  struct stat *statbuf)
{
	NihError *err;

	error_called++;

	err = nih_error_get ();
	last_error = err->number;
	if (last_error_path)
		free (last_error_path);
	last_error_path = strdup (path);

 	if (data == (void *)-2) {
		nih_error_raise_again (err);
		return -1;
	}

	nih_free (err);

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

static int logger_called = 0;

static int
my_logger (NihLogLevel  priority,
	   const char  *message)
{
	logger_called++;

	return 0;
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


	/* Check that when called without a filter, the visitor is called
	 * for all paths found underneath the tree; getting passed the
	 * correct data pointer, top-level path and path name,
	 */
	TEST_FEATURE ("with no filter");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		ret = nih_dir_walk (dirname, NULL, my_visitor, NULL, &ret);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 7);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar/bilbo");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar/frodo");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/frodo");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/frodo/baggins");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}


	/* Check that a filter can be used to restrict the names of
	 * objects visited and descended into.
	 */
	TEST_FEATURE ("with filter");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, NULL, &ret);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 4);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar/bilbo");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}


	/* Check that failing to stat a file or directory in the tree with
	 * no error handler set results in a warning being emitted and us
	 * stepping over it.
	 */
	TEST_FEATURE ("with stat failure and no error handler");
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0644);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		logger_called = 0;
		nih_log_set_logger (my_logger);

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, NULL, &ret);

		nih_log_set_logger (nih_logger_printf);

		TEST_EQ (logger_called, 1);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 3);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0755);


	/* Check that failing to stat a file or directory in the tree with
	 * an error handler set results in the handler being called.
	 */
	TEST_FEATURE ("with stat failure and error handler");
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0644);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		error_called = 0;
		last_error = -1;
		last_error_path = NULL;

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, my_error_handler, &ret);

		TEST_EQ (error_called, 1);
		TEST_EQ (last_error, EACCES);

		strcpy (filename, dirname);
		strcat (filename, "/bar/bilbo");
		TEST_EQ_STR (last_error_path, filename);
		free (last_error_path);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 3);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0755);


	/* Check that the error handler can return a negative value and
	 * raised error to abort the directory walk.
	 */
	TEST_FEATURE ("with error from error handler");
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0644);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		error_called = 0;
		last_error = -1;
		last_error_path = NULL;

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, my_error_handler, (void *)-2);

		TEST_EQ (error_called, 1);
		TEST_EQ (last_error, EACCES);

		strcpy (filename, dirname);
		strcat (filename, "/bar/bilbo");
		TEST_EQ_STR (last_error_path, filename);
		free (last_error_path);

		err = nih_error_get ();
		TEST_EQ (err->number, EACCES);
		nih_free (err);

		TEST_EQ (ret, -1);
		TEST_EQ (visitor_called, 1);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, (void *)-2);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0755);


	/* Check that a complete failure to walk a sub-directory underneath
	 * the tree also results in the error handler being called.
	 */
	TEST_FEATURE ("with inability to walk a sub-directory");
	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0000);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		error_called = 0;
		last_error = -1;
		last_error_path = NULL;

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, my_error_handler, &ret);

		TEST_EQ (error_called, 1);
		TEST_EQ (last_error, EACCES);

		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (last_error_path, filename);
		free (last_error_path);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 3);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}

	strcpy (filename, dirname);
	strcat (filename, "/bar");
	chmod (filename, 0755);


	/* Check that a warning is emitted if the visitor raises an error
	 * when there is no error handler set.
	 */
	TEST_FEATURE ("with error in visitor");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		logger_called = 0;
		nih_log_set_logger (my_logger);

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, NULL, (void *)-1);

		nih_log_set_logger (nih_logger_printf);

		TEST_EQ (logger_called, 3);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 3);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, (void *)-1);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, (void *)-1);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, (void *)-1);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}


	/* Check that the error handled is called if the visitor raises
	 * an error.
	 */
	TEST_FEATURE ("with error in visitor and handler");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		error_called = 0;
		last_error = -1;
		last_error_path = NULL;

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, my_error_handler, (void *)-1);

		TEST_EQ (error_called, 3);
		free (last_error_path);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 3);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, (void *)-1);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, (void *)-1);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, (void *)-1);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}


	/* Check that we get a ENOTDIR error if we try and walk a file
	 * and there's no error handler set.
	 */
	TEST_FEATURE ("with non-directory and no error handler");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	TEST_ALLOC_FAIL {
		visitor_called = 0;

		ret = nih_dir_walk (filename, my_filter,
				    my_visitor, NULL, &ret);

		TEST_EQ (ret, -1);
		TEST_EQ (visitor_called, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, ENOTDIR);
		nih_free (err);
	}


	/* Check that we still get a ENOTDIR error if we try and walk a file
	 * and there is an error handler set.
	 */
	TEST_FEATURE ("with non-directory and error handler");
	strcpy (filename, dirname);
	strcat (filename, "/foo");

	TEST_ALLOC_FAIL {
		visitor_called = 0;
		error_called = 0;

		ret = nih_dir_walk (filename, my_filter,
				    my_visitor, my_error_handler, &ret);

		TEST_EQ (ret, -1);
		TEST_EQ (visitor_called, 0);
		TEST_EQ (error_called, 0);

		err = nih_error_get ();
		TEST_EQ (err->number, ENOTDIR);
		nih_free (err);
	}


	/* Check that we can detect the simplest kind of directory loop, and
	 * have it treated as an ordinary error while visiting.
	 */
	TEST_FEATURE ("with simple directory loop");
	strcpy (filename, dirname);
	strcat (filename, "/bar/loop");
	symlink (dirname, filename);

	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			visitor_called = 0;
			visited = nih_list_new (NULL);
		}

		error_called = 0;
		last_error = -1;
		last_error_path = NULL;

		ret = nih_dir_walk (dirname, my_filter,
				    my_visitor, my_error_handler, &ret);

		TEST_EQ (error_called, 1);
		TEST_EQ (last_error, NIH_DIR_LOOP_DETECTED);

		strcpy (filename, dirname);
		strcat (filename, "/bar/loop");
		TEST_EQ_STR (last_error_path, filename);
		free (last_error_path);

		TEST_EQ (ret, 0);
		TEST_EQ (visitor_called, 5);

		v = (Visited *)visited->next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar/bilbo");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/bar/loop");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/baz");
		TEST_EQ_STR (v->path, filename);

		v = (Visited *)v->entry.next;
		TEST_EQ (v->data, &ret);
		TEST_EQ_STR (v->dirname, dirname);
		strcpy (filename, dirname);
		strcat (filename, "/foo");
		TEST_EQ_STR (v->path, filename);

		nih_free (visited);
	}


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


int
main (int   argc,
      char *argv[])
{
	test_map ();
	test_unmap ();
	test_dir_walk ();

	return 0;
}
