/* libnih
 *
 * test_file.c - test suite for nih/file.c
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

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <limits.h>
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
test_read (void)
{
	FILE     *fd;
	char      filename[PATH_MAX], *file;
	size_t    length;
	NihError *err;

	TEST_FUNCTION ("nih_file_read");
	nih_error_init ();


	/* Check that we can read a file into memory, and that the memory
	 * contents match the file.
	 */
	TEST_FEATURE ("with existing file");
	TEST_FILENAME (filename);

	fd = fopen (filename, "w");
	fprintf (fd, "test\n");
	fclose (fd);

	TEST_ALLOC_FAIL {
		length = 0;
		file = nih_file_read (NULL, filename, &length);

		if (test_alloc_failed) {
			TEST_EQ_P (file, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			continue;
		}

		TEST_ALLOC_SIZE (file, 5);

		TEST_NE_P (file, NULL);
		TEST_EQ (length, 5);
		TEST_EQ_MEM (file, "test\n", 5);

		nih_free (file);
	}

	unlink (filename);


	/* Check that if we try and read a non-existant file, we get an
	 * error raised.
	 */
	TEST_FEATURE ("with non-existant file");
	length = 0;
	file = nih_file_read (NULL, filename, &length);

	TEST_EQ_P (file, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, ENOENT);
	nih_free (err);
}


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

	if (! fgets (text, sizeof (text), fd))
		TEST_FAILED ("unexpected eof on file");
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


void
test_is_hidden (void)
{
	int ret;

	TEST_FUNCTION ("nih_file_is_hidden");


	/* Check that a plain file beginning with a dot is hidden. */
	TEST_FEATURE ("with plain dot file");
	ret = nih_file_is_hidden (".foo");

	TEST_TRUE (ret);


	/* Check that a path with a file beginning with a dot is hidden. */
	TEST_FEATURE ("with path to dot file");
	ret = nih_file_is_hidden ("/path/to/.foo");

	TEST_TRUE (ret);


	/* Check that a path containing a dot directory is not hidden,
	 * since we're already walking it.
	 */
	TEST_FEATURE ("with hidden path to non-dot file");
	ret = nih_file_is_hidden ("/path/.to/foo");

	TEST_FALSE (ret);


	/* Check that a plain non-dot file is not hidden. */
	TEST_FEATURE ("with plain non-dot file");
	ret = nih_file_is_hidden ("foo");

	TEST_FALSE (ret);


	/* Check that a file containing a dot is not hidden. */
	TEST_FEATURE ("with ordinary file");
	ret = nih_file_is_hidden ("foo.txt");

	TEST_FALSE (ret);
}

void
test_is_backup (void)
{
	int ret;

	TEST_FUNCTION ("nih_file_is_backup");


	/* Check that a plain file ending with a tilde is a backup file. */
	TEST_FEATURE ("with plain backup file");
	ret = nih_file_is_backup ("foo~");

	TEST_TRUE (ret);


	/* Check that a path with a file ending with a tilde is a
	 * backup file.
	 */
	TEST_FEATURE ("with path to backup file");
	ret = nih_file_is_backup ("/path/to/foo~");

	TEST_TRUE (ret);


	/* Check that a path containing a tilde directory is not backup,
	 * since we're already walking it.
	 */
	TEST_FEATURE ("with backup path to non-backup file");
	ret = nih_file_is_backup ("/path/to~/foo");

	TEST_FALSE (ret);


	/* Check that a file containing a tilde is not backup. */
	TEST_FEATURE ("with file containing tilde");
	ret = nih_file_is_backup ("foo~txt");

	TEST_FALSE (ret);


	/* Check that a plain non-backup file is not matched. */
	TEST_FEATURE ("with plain non-backup file");
	ret = nih_file_is_backup ("foo");

	TEST_FALSE (ret);


	/* Check that a file ending with .bak is a backup file. */
	TEST_FEATURE ("with dos-style backup file");
	ret = nih_file_is_backup ("foo.bak");

	TEST_TRUE (ret);


	/* Check that a file ending with .BAK is a backup file. */
	TEST_FEATURE ("with dos/fat-style backup file");
	ret = nih_file_is_backup ("foo.BAK");

	TEST_TRUE (ret);


	/* Check that an emacs-style backup file is matched. */
	TEST_FEATURE ("with emacs-style backup file");
	ret = nih_file_is_backup ("#foo#");

	TEST_TRUE (ret);


	/* Check that a file beginning with a # is not matched. */
	TEST_FEATURE ("with file beginning with hash");
	ret = nih_file_is_backup ("#foo");

	TEST_FALSE (ret);


	/* Check that a file ending with a # is not matched. */
	TEST_FEATURE ("with file ending with hash");
	ret = nih_file_is_backup ("foo#");

	TEST_FALSE (ret);
}

void
test_is_swap (void)
{
	int ret;

	TEST_FUNCTION ("nih_file_is_swap");


	/* Check that a plain file beginning with .# is a swap file. */
	TEST_FEATURE ("with emacs-style swap file");
	ret = nih_file_is_swap (".#foo");

	TEST_TRUE (ret);


	/* Check that a path with a file beginning with .# is a swap file. */
	TEST_FEATURE ("with path to emacs-style swap file");
	ret = nih_file_is_swap ("/path/to/.#foo");

	TEST_TRUE (ret);


	/* Check that a path containing an emacs-style swap directory is
	 * not swap, since we're already walking it.
	 */
	TEST_FEATURE ("with emacs-style swap path to non-swap file");
	ret = nih_file_is_swap ("/path/.#to/foo");

	TEST_FALSE (ret);


	/* Check that a file containing the signature is not swap. */
	TEST_FEATURE ("with file containing .#");
	ret = nih_file_is_swap ("foo.#txt");

	TEST_FALSE (ret);


	/* Check that a plain non-swap file is not matched. */
	TEST_FEATURE ("with plain non-swap file");
	ret = nih_file_is_swap ("foo");

	TEST_FALSE (ret);


	/* Check that a file ending with .swp is a swap file. */
	TEST_FEATURE ("with vi-style .swp file");
	ret = nih_file_is_swap ("foo.swp");

	TEST_TRUE (ret);


	/* Check that a file ending with .swo is a swap file. */
	TEST_FEATURE ("with vi-style .swo file");
	ret = nih_file_is_swap ("foo.swo");

	TEST_TRUE (ret);


	/* Check that a file ending with .swn is a swap file. */
	TEST_FEATURE ("with vi-style .swn file");
	ret = nih_file_is_swap ("foo.swn");

	TEST_TRUE (ret);
}

void
test_is_rcs (void)
{
	int ret;

	TEST_FUNCTION ("nih_file_is_rcs");


	/* Check that a plain file ending with ,v is an RCS file. */
	TEST_FEATURE ("with rcs-style file");
	ret = nih_file_is_rcs ("foo,v");

	TEST_TRUE (ret);


	/* Check that a path with a file ending with ,v an RCS file. */
	TEST_FEATURE ("with path to rcs-style file");
	ret = nih_file_is_rcs ("/path/to/foo,v");

	TEST_TRUE (ret);


	/* Check that a path containing an RCS-style directory is not matched,
	 * since we're already walking it.
	 */
	TEST_FEATURE ("with rcs-style path to non-rcs file");
	ret = nih_file_is_rcs ("/path/to,v/foo");

	TEST_FALSE (ret);


	/* Check that a file containing the signature is not rcs. */
	TEST_FEATURE ("with file containing ,v");
	ret = nih_file_is_rcs ("foo,vtxt");

	TEST_FALSE (ret);


	/* Check that a plain non-rcs file is not matched. */
	TEST_FEATURE ("with plain non-rcs file");
	ret = nih_file_is_rcs ("foo");

	TEST_FALSE (ret);


	/* Check that an RCS directory is matched. */
	TEST_FEATURE ("with rcs directory name");
	ret = nih_file_is_rcs ("RCS");

	TEST_TRUE (ret);


	/* Check that a CVS directory is matched. */
	TEST_FEATURE ("with cvs directory name");
	ret = nih_file_is_rcs ("CVS");

	TEST_TRUE (ret);


	/* Check that a CVS admin directory is matched. */
	TEST_FEATURE ("with cvs admin directory name");
	ret = nih_file_is_rcs ("CVS.adm");

	TEST_TRUE (ret);


	/* Check that an SCCS directory is matched. */
	TEST_FEATURE ("with sccs directory name");
	ret = nih_file_is_rcs ("SCCS");

	TEST_TRUE (ret);


	/* Check that a bazaar directory is matched. */
	TEST_FEATURE ("with bzr directory name");
	ret = nih_file_is_rcs (".bzr");

	TEST_TRUE (ret);


	/* Check that a bazaar log file is matched. */
	TEST_FEATURE ("with bzr log filename");
	ret = nih_file_is_rcs (".bzr.log");

	TEST_TRUE (ret);


	/* Check that a mercurial directory is matched. */
	TEST_FEATURE ("with mercurial directory name");
	ret = nih_file_is_rcs (".hg");

	TEST_TRUE (ret);


	/* Check that a git directory is matched. */
	TEST_FEATURE ("with git directory name");
	ret = nih_file_is_rcs (".git");

	TEST_TRUE (ret);


	/* Check that a subversion directory is matched. */
	TEST_FEATURE ("with subversion directory name");
	ret = nih_file_is_rcs (".svn");

	TEST_TRUE (ret);


	/* Check that a BitKeeper directory is matched. */
	TEST_FEATURE ("with BitKeeper directory name");
	ret = nih_file_is_rcs ("BitKeeper");

	TEST_TRUE (ret);


	/* Check that an arch ids file is matched. */
	TEST_FEATURE ("with arch ids filename");
	ret = nih_file_is_rcs (".arch-ids");

	TEST_TRUE (ret);


	/* Check that an arch inventory file is matched. */
	TEST_FEATURE ("with arch inventory filename");
	ret = nih_file_is_rcs (".arch-inventory");

	TEST_TRUE (ret);


	/* Check that an arch directory is matched. */
	TEST_FEATURE ("with arch directory name");
	ret = nih_file_is_rcs ("{arch}");

	TEST_TRUE (ret);


	/* Check that a darcs directory is matched. */
	TEST_FEATURE ("with darcs directory name");
	ret = nih_file_is_rcs ("_darcs");

	TEST_TRUE (ret);
}

void
test_is_packaging (void)
{
	int ret;

	TEST_FUNCTION ("nih_file_is_packaging");


	/* Check that a plain file ending with a dpkg name is packaging. */
	TEST_FEATURE ("with plain dpkg file");
	ret = nih_file_is_packaging ("foo.dpkg-new");

	TEST_TRUE (ret);


	/* Check that a path with a file ending with a dpkg name
	 * is packaging
	 */
	TEST_FEATURE ("with path to dpkg file");
	ret = nih_file_is_packaging ("/path/to/foo.dpkg-bak");

	TEST_TRUE (ret);


	/* Check that a path containing a dpkg directory is not packaging,
	 * since we're already walking it.
	 */
	TEST_FEATURE ("with dpkg path to non-packaging file");
	ret = nih_file_is_packaging ("/path/to.dpkg-bak/foo");

	TEST_FALSE (ret);


	/* Check that a plain file is not packaging. */
	TEST_FEATURE ("with plain file");
	ret = nih_file_is_packaging ("foo.txt");

	TEST_FALSE (ret);
}

void
test_ignore (void)
{
	int ret;

	TEST_FUNCTION ("nih_test_ignore");


	/* Check that a hidden file is to be ignored. */
	TEST_FEATURE ("with hidden file");
	ret = nih_file_ignore (NULL, ".foo");

	TEST_TRUE (ret);


	/* Check that a backup file is to be ignored. */
	TEST_FEATURE ("with backup file");
	ret = nih_file_ignore (NULL, "foo~");

	TEST_TRUE (ret);


	/* Check that a swap file is to be ignored. */
	TEST_FEATURE ("with swap file");
	ret = nih_file_ignore (NULL, "foo.swp");

	TEST_TRUE (ret);


	/* Check that an RCS file is to be ignored. */
	TEST_FEATURE ("with rcs file");
	ret = nih_file_ignore (NULL, "CVS");

	TEST_TRUE (ret);


	/* Check that a packaging file is to be ignored. */
	TEST_FEATURE ("with packaging file");
	ret = nih_file_ignore (NULL, "foo.dpkg-new");

	TEST_TRUE (ret);


	/* Check that an ordinary file isn't ignored. */
	TEST_FEATURE ("with ordinary file");
	ret = nih_file_ignore (NULL, "foo.txt");

	TEST_FALSE (ret);
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
		nih_alloc_set_destructor (v, nih_list_destroy);

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
		return -1;
	}

	nih_free (err);

	return 0;
}


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
	assert0 (symlink (dirname, filename));

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
	strcat (filename, "/bar/loop");
	unlink (filename);


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
	test_read ();
	test_map ();
	test_unmap ();
	test_is_hidden ();
	test_is_backup ();
	test_is_swap ();
	test_is_rcs ();
	test_is_packaging ();
	test_ignore ();
	test_dir_walk ();

	return 0;
}
