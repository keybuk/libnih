/* libnih
 *
 * file.c - file and directory utility functions
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/io.h>
#include <nih/file.h>
#include <nih/logging.h>
#include <nih/error.h>
#include <nih/errors.h>


/**
 * NihDirEntry:
 * @entry: list header,
 * @dev: device number,
 * @ino: inode number.
 *
 * This structure is used to detect directory loops, and is stored in a stack
 * as we recurse down the directory tree.
 **/
typedef struct nih_dir_entry {
	NihList entry;
	dev_t   dev;
	ino_t   ino;
} NihDirEntry;


/* Prototypes for static functions */
static char **nih_dir_walk_scan  (const char *path, NihFileFilter filter,
				  void *data)
	__attribute__ ((warn_unused_result, malloc));
static int    nih_dir_walk_visit (const char *dirname, NihList *dirs,
				  const char *path, NihFileFilter filter,
				  NihFileVisitor visitor,
				  NihFileErrorHandler error, void *data)
	__attribute__ ((warn_unused_result));


/**
 * nih_file_read:
 * @parent: parent object for new string,
 * @path: path to read,
 * @length: pointer to store file length in.
 *
 * Opens the file at @path and reads the contents into memory, returning
 * a newly allocated string.  If the file is particularly large, it may
 * not be possible to read into memory at all, and you'll need to use
 * nih_file_map() instead.
 *
 * The returned data will NOT be NULL terminated.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
nih_file_read (const void *parent,
	       const char *path,
	       size_t     *length)
{
	struct stat  statbuf;
	FILE        *fp;
	char        *file = NULL;

	nih_assert (path != NULL);
	nih_assert (length != NULL);

	fp = fopen (path, "r");
	if (! fp)
		nih_return_system_error (NULL);

	if (fstat (fileno (fp), &statbuf) < 0)
		goto error;

	if ((size_t)statbuf.st_size > SIZE_MAX) {
		errno = EFBIG;
		goto error;
	}

	*length = statbuf.st_size;

	file = nih_alloc (parent, statbuf.st_size);
	if (! file)
		goto error;

	if (fread (file, 1, statbuf.st_size, fp) != (size_t)statbuf.st_size) {
		errno = EILSEQ;
		goto error;
	}

	fclose (fp);
	return file;
error:
	nih_error_raise_system ();
	if (file)
		nih_free (file);
	fclose (fp);

	return NULL;
}

/**
 * nih_file_map:
 * @path: path to open,
 * @flags: open mode,
 * @length: pointer to store file length.
 *
 * Opens the file at @path and maps it into memory, returning the mapped
 * pointer and the length of the file (required to unmap it later).  The
 * file is opened with the @flags given.
 *
 * Returns: memory mapped file or NULL on raised error.
 **/
void *
nih_file_map (const char *path,
	      int         flags,
	      size_t     *length)
{
	struct stat  statbuf;
	char        *map;
	int          fd, prot;

	nih_assert (path != NULL);
	nih_assert (length != NULL);

	nih_assert (((flags & O_ACCMODE) == O_RDONLY)
		    || ((flags & O_ACCMODE) == O_RDWR));

	fd = open (path, flags);
	if (fd < 0)
		nih_return_system_error (NULL);

	prot = PROT_READ;
	if ((flags & O_ACCMODE) == O_RDWR)
		prot |= PROT_WRITE;

	if (fstat (fd, &statbuf) < 0)
		goto error;

	if ((size_t)statbuf.st_size > SIZE_MAX) {
		errno = EFBIG;
		goto error;
	}

	*length = statbuf.st_size;

	map = mmap (NULL, *length, prot, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED)
		goto error;

	close (fd);
	return map;
error:
	nih_error_raise_system ();
	close (fd);
	return NULL;
}

/**
 * nih_file_unmap:
 * @map: memory mapped file,
 * @length: length of file.
 *
 * Unmap a file previously mapped with nih_file_map().
 *
 * Returns: zero on success, NULL on raised error.
 **/
int
nih_file_unmap (void   *map,
		size_t  length)
{
	nih_assert (map != NULL);

	if (munmap (map, length) < 0)
		nih_return_system_error (-1);

	return 0;
}


/**
 * nih_file_is_hidden:
 * @path: path to check.
 *
 * Determines whether @path represents a hidden file, matching it against
 * common patterns for that type of file.
 *
 * Returns: TRUE if it matches, FALSE otherwise.
 **/
int
nih_file_is_hidden (const char *path)
{
	const char *ptr;
	size_t      len;

	nih_assert (path != NULL);

	ptr = strrchr (path, '/');
	if (ptr)
		path = ptr + 1;

	len = strlen (path);

	/* Matches .*; standard hidden pattern */
	if ((len >= 1) && (path[0] == '.'))
		return TRUE;

	return FALSE;
}

/**
 * nih_file_is_backup:
 * @path: path to check.
 *
 * Determines whether @path represents a backup file, matching it against
 * common patterns for that type of file.
 *
 * Returns: TRUE if it matches, FALSE otherwise.
 **/
int
nih_file_is_backup (const char *path)
{
	const char *ptr;
	size_t      len;

	nih_assert (path != NULL);

	ptr = strrchr (path, '/');
	if (ptr)
		path = ptr + 1;

	len = strlen (path);
	ptr = path + len;

	/* Matches *~; standard backup style */
	if ((len >= 1) && (ptr[-1] == '~'))
		return TRUE;

	/* Matches *.bak; common backup extension */
	if ((len >= 4) && (! strcmp (&ptr[-4], ".bak")))
		return TRUE;

	/* Matches *.BAK; as above, but on case-insensitive filesystems */
	if ((len >= 4) && (! strcmp (&ptr[-4], ".BAK")))
		return TRUE;

	/* Matches #*#; used by emacs for unsaved files */
	if ((len >= 2) && (path[0] == '#') && (ptr[-1] == '#'))
		return TRUE;

	return FALSE;
}

/**
 * nih_file_is_swap:
 * @path: path to check.
 *
 * Determines whether @path represents an editor swap file, matching it
 * against common patterns for that type of file.
 *
 * Returns: TRUE if it matches, FALSE otherwise.
 **/
int
nih_file_is_swap (const char *path)
{
	const char *ptr;
	size_t      len;

	nih_assert (path != NULL);

	ptr = strrchr (path, '/');
	if (ptr)
		path = ptr + 1;

	len = strlen (path);
	ptr = path + len;

	/* Matches *.swp; used by vi */
	if ((len >= 4) && (! strcmp (&ptr[-4], ".swp")))
		return TRUE;

	/* Matches *.swo; used by vi */
	if ((len >= 4) && (! strcmp (&ptr[-4], ".swo")))
		return TRUE;

	/* Matches *.swn; used by vi */
	if ((len >= 4) && (! strcmp (&ptr[-4], ".swn")))
		return TRUE;

	/* Matches .#*; used by emacs */
	if ((len >= 2) && (! strncmp (path, ".#", 2)))
		return TRUE;

	return FALSE;
}

/**
 * nih_file_is_rcs:
 * @path: path to check.
 *
 * Determines whether @path represents a file or directory used by a
 * common revision control system, matching it against common patterns
 * for known RCSs.
 *
 * Returns: TRUE if it matches, FALSE otherwise.
 **/
int
nih_file_is_rcs (const char *path)
{
	const char *ptr;
	size_t      len;

	nih_assert (path != NULL);

	ptr = strrchr (path, '/');
	if (ptr)
		path = ptr + 1;

	len = strlen (path);
	ptr = path + len;

	/* Matches *,v; used by rcs and cvs */
	if ((len >= 2) && (! strcmp (&ptr[-2], ",v")))
		return TRUE;

	/* RCS; used by rcs */
	if (! strcmp (path, "RCS"))
		return TRUE;

	/* CVS; used by cvs */
	if (! strcmp (path, "CVS"))
		return TRUE;

	/* CVS.adm; used by cvs */
	if (! strcmp (path, "CVS.adm"))
		return TRUE;

	/* SCCS; used by sccs */
	if (! strcmp (path, "SCCS"))
		return TRUE;

	/* .bzr; used by bzr */
	if (! strcmp (path, ".bzr"))
		return TRUE;

	/* .bzr.log; used by bzr */
	if (! strcmp (path, ".bzr.log"))
		return TRUE;

	/* .hg; used by hg */
	if (! strcmp (path, ".hg"))
		return TRUE;

	/* .git; used by git */
	if (! strcmp (path, ".git"))
		return TRUE;

	/* .svn; used by subversion */
	if (! strcmp (path, ".svn"))
		return TRUE;

	/* BitKeeper; used by BitKeeper */
	if (! strcmp (path, "BitKeeper"))
		return TRUE;

	/* .arch-ids; used by tla */
	if (! strcmp (path, ".arch-ids"))
		return TRUE;

	/* .arch-inventory; used by tla */
	if (! strcmp (path, ".arch-inventory"))
		return TRUE;

	/* {arch}; used by tla */
	if (! strcmp (path, "{arch}"))
		return TRUE;

	/* _darcs; used by darcs */
	if (! strcmp (path, "_darcs"))
		return TRUE;

	return FALSE;
}

/**
 * nih_file_is_packaging:
 * @path: path to check.
 *
 * Determines whether @path represents a file or directory used by a
 * common package manager, matching it against common patterns.
 *
 * Returns: TRUE if it matches, FALSE otherwise.
 **/
int
nih_file_is_packaging (const char *path)
{
	const char *ptr;

	nih_assert (path != NULL);

	ptr = strrchr (path, '/');
	if (ptr)
		path = ptr + 1;

	/* Matches *.dpkg-*; used by dpkg */
	ptr = strrchr (path, '.');
	if (ptr && (! strncmp (ptr, ".dpkg-", 6)))
		return TRUE;

	/* Matches *.rpm{save,orig,new}; used by rpm */
	if (ptr && (! strncmp (ptr, ".rpmsave", 9)))
		return TRUE;
	if (ptr && (! strncmp (ptr, ".rpmorig", 9)))
		return TRUE;
	if (ptr && (! strncmp (ptr, ".rpmnew", 8)))
		return TRUE;

	/* Matches *;[a-fA-F0-9]{8}; used by rpm */
	ptr = strrchr (path, ';');
	if (ptr && (strspn (ptr + 1, "abcdefABCDEF0123456789") == 8)
	    && (! ptr[9]))
		return TRUE;

	return FALSE;
}

/**
 * nih_file_ignore:
 * @data: data pointer,
 * @path: path to check.
 *
 * Determines whether @path should normally be ignored when walking a
 * directory tree.  Files ignored are those that are hidden, represent
 * backup files, editor swap files and both files and directories used
 * by revision control systems.
 *
 * Returns: TRUE if it should be ignored, FALSE otherwise.
 **/
int
nih_file_ignore (void       *data,
		 const char *path)
{
	if (nih_file_is_hidden (path))
		return TRUE;

	if (nih_file_is_backup (path))
		return TRUE;

	if (nih_file_is_swap (path))
		return TRUE;

	if (nih_file_is_rcs (path))
		return TRUE;

	if (nih_file_is_packaging (path))
		return TRUE;

	return FALSE;
}


/**
 * nih_dir_walk:
 * @path: path to walk,
 * @filter: path filter,
 * @visitor: function to call for each path,
 * @error: function to call on error,
 * @data: data to pass to @visitor.
 *
 * Iterates the directory tree starting at @path, calling @visitor for
 * each file, directory or other object found.  Sub-directories are
 * descended into, and the same @visitor called for those.
 *
 * @visitor is not called for @path itself.
 *
 * @filter can be used to restrict both the sub-directories iterated and
 * the objects that @visitor is called for.  It is passed the full path
 * of the object, and if it returns TRUE, the object is ignored.
 *
 * If @visitor returns a negative value, or there's an error obtaining
 * the listing for a particular sub-directory, then the @error function
 * will be called.  This function should handle the error and return zero,
 * or raise an error again and return a negative value which causes the
 * entire walk to be aborted.  If @error is NULL, then a warning is emitted
 * instead.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_dir_walk (const char          *path,
	      NihFileFilter        filter,
	      NihFileVisitor       visitor,
	      NihFileErrorHandler  error,
	      void                *data)
{
	nih_local NihList  *dirs = NULL;
	struct stat         statbuf;
	nih_local char    **paths = NULL;
	char              **subpath;
	int                 ret = 0;

	nih_assert (path != NULL);
	nih_assert (visitor != NULL);

	paths = nih_dir_walk_scan (path, filter, data);
	if (! paths)
		return -1;


	dirs = NIH_MUST (nih_list_new (NULL));

	if (stat (path, &statbuf) == 0) {
		NihDirEntry *entry;

		entry = NIH_MUST (nih_new (dirs, NihDirEntry));
		nih_list_init (&entry->entry);
		nih_alloc_set_destructor (entry, nih_list_destroy);
		entry->dev = statbuf.st_dev;
		entry->ino = statbuf.st_ino;
		nih_list_add (dirs, &entry->entry);
	}

	for (subpath = paths; *subpath; subpath++) {
		ret = nih_dir_walk_visit (path, dirs, *subpath, filter,
					  visitor, error, data);
		if (ret < 0)
			break;
	}

	return ret;
}

/**
 * nih_dir_walk_sort:
 * @a: pointer to first path,
 * @b: pointer to second path.
 *
 * This function wraps the strcoll() function allowing it to be called
 * from qsort().
 *
 * Returns: zero if strings are equal, otherwise integer less than zero
 * if @a is less than @b or integer greater than zero if @a is greater
 * than @b.
 **/
static int
nih_dir_walk_sort (const void *a,
		   const void *b)
{
	const char * const *path_a;
	const char * const *path_b;

	nih_assert (a != NULL);
	nih_assert (b != NULL);

	path_a = a;
	path_b = b;

	return strcoll (*path_a, *path_b);
}

/**
 * nih_dir_walk_scan:
 * @path: path to scan,
 * @filter: path filter,
 * @data: data to pass to @filter.
 *
 * Reads the list of files in @path, removing ".", ".." and any for which
 * @filter return TRUE.
 *
 * Returns: NULL-terminated array of full paths to sub-paths or NULL on
 * raised error.
 **/
static char **
nih_dir_walk_scan (const char    *path,
		   NihFileFilter  filter,
		   void          *data)
{
	DIR            *dir;
	struct dirent  *ent;
	char          **paths;
	size_t          npaths;

	nih_assert (path != NULL);

	dir = opendir (path);
	if (! dir)
		nih_return_system_error (NULL);

	npaths = 0;
	paths = NIH_MUST (nih_str_array_new (NULL));

	while ((ent = readdir (dir)) != NULL) {
		nih_local char *subpath = NULL;

		/* Always ignore '.' and '..' */
		if ((! strcmp (ent->d_name, "."))
		    || (! strcmp (ent->d_name, "..")))
			continue;

		subpath = NIH_MUST (nih_sprintf (NULL, "%s/%s",
						 path, ent->d_name));

		if (filter && filter (data, subpath, ent->d_type == DT_DIR))
			continue;

		NIH_MUST (nih_str_array_addp (&paths, NULL, &npaths, subpath));
	}

	closedir (dir);

	qsort (paths, npaths, sizeof (char *), nih_dir_walk_sort);

	return paths;
}


/**
 * nih_dir_walk_visit:
 * @dirname: top-level being walked,
 * @dirs: stack of visited directories,
 * @path: path being visited,
 * @filter: path filter,
 * @visitor: function to call for each path,
 * @error: function to call on error,
 * @data: data to pass to @visitor.
 *
 * Visits an individual @path found while iterating the directory tree
 * started at @dirname.  Ensures that @visitor is called for @path, and
 * if @path is a directory, it is descended into and the same @visitor
 * called for each of those.
 *
 * @filter can be used to restrict both the sub-directories iterated and
 * the objects that @visitor is called for.  It is passed the full path
 * of the object, and if it returns TRUE, the object is ignored.
 *
 * If @visitor returns a negative value, or there's an error obtaining
 * the listing for a particular sub-directory, then the @error function
 * will be called.  This function should handle the error and return zero,
 * or raise an error again and return a negative value which causes the
 * entire walk to be aborted.  If @error is NULL, then a warning is emitted
 * instead.
 *
 * Returns: zero on success, negative value on raised error.
 **/
static int
nih_dir_walk_visit (const char          *dirname,
		    NihList             *dirs,
		    const char          *path,
		    NihFileFilter        filter,
		    NihFileVisitor       visitor,
		    NihFileErrorHandler  error,
		    void                *data)
{
	struct stat statbuf;

	nih_assert (dirname != NULL);
	nih_assert (dirs != NULL);
	nih_assert (path != NULL);
	nih_assert (visitor != NULL);

	/* Not much we can do here if we can't at least stat it */
	if (stat (path, &statbuf) < 0) {
		nih_error_raise_system ();
		goto error;
	}

	/* Call the handler */
	if (visitor (data, dirname, path, &statbuf) < 0)
		goto error;

	/* Iterate into sub-directories; first checking for directory loops.
	 */
	if (S_ISDIR (statbuf.st_mode)) {
		nih_local NihDirEntry  *entry = NULL;
		nih_local char        **paths = NULL;
		char                  **subpath;
		int                     ret = 0;

		NIH_LIST_FOREACH (dirs, iter) {
			NihDirEntry *entry = (NihDirEntry *)iter;

			if ((entry->dev == statbuf.st_dev)
			    && (entry->ino == statbuf.st_ino)) {
				nih_error_raise (NIH_DIR_LOOP_DETECTED,
						 _(NIH_DIR_LOOP_DETECTED_STR));
				goto error;
			}
		}

		/* Grab the directory contents */
		paths = nih_dir_walk_scan (path, filter, data);
		if (! paths)
			goto error;

		/* Record the device and inode numbers in the stack so that
		 * we can detect directory loops.
		 */
		entry = NIH_MUST (nih_new (NULL, NihDirEntry));
		nih_list_init (&entry->entry);
		nih_alloc_set_destructor (entry, nih_list_destroy);
		entry->dev = statbuf.st_dev;
		entry->ino = statbuf.st_ino;
		nih_list_add (dirs, &entry->entry);

		/* Iterate the paths found.  If these calls return a negative
		 * value, it means that an error handler decided to abort the
		 * walk; so just abort right now.
		 */
		for (subpath = paths; *subpath; subpath++) {
			ret = nih_dir_walk_visit (dirname, dirs, *subpath,
						  filter, visitor, error,
						  data);
			if (ret < 0)
				break;
		}

		if (ret < 0)
			return ret;
	}

	return 0;

error:
	if (error) {
		return error (data, dirname, path, &statbuf);
	} else {
		NihError *err;

		err = nih_error_get ();
		nih_warn ("%s: %s", path, err->message);
		nih_free (err);

		return 0;
	}
}
