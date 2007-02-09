/* libnih
 *
 * file.c - file and directory utility functions
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
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
static char **nih_dir_walk_scan  (const char *path, NihFileFilter filter)
	__attribute__ ((warn_unused_result, malloc));
static int    nih_dir_walk_visit (const char *dirname, NihList *dirs,
				  const char *path, NihFileFilter filter,
				  NihFileVisitor visitor,
				  NihFileErrorHandler error, void *data)
	__attribute__ ((warn_unused_result));


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


	return FALSE;
}

/**
 * nih_file_ignore:
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
nih_file_ignore (const char *path)
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
	NihList      *dirs;
	struct stat   statbuf;
	char        **paths, **subpath;
	int           ret = 0;

	nih_assert (path != NULL);
	nih_assert (visitor != NULL);

	paths = nih_dir_walk_scan (path, filter);
	if (! paths)
		return -1;


	NIH_MUST (dirs = nih_list_new (NULL));

	if (stat (path, &statbuf) == 0) {
		NihDirEntry *entry;

		NIH_MUST (entry = nih_new (dirs, NihDirEntry));
		nih_list_init (&entry->entry);
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

	nih_free (dirs);
	nih_free (paths);

	return ret;
}

/**
 * nih_dir_walk_scan:
 * @path: path to scan,
 * @filter: path filter.
 *
 * Reads the list of files in @path, removing ".", ".." and any for which
 * @filter return TRUE.
 *
 * Returns: NULL-terminated array of full paths to sub-paths or NULL on
 * raised error.
 **/
static char **
nih_dir_walk_scan (const char    *path,
		   NihFileFilter  filter)
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
	NIH_MUST (paths = nih_str_array_new (NULL));

	while ((ent = readdir (dir)) != NULL) {
		char  *subpath;

		/* Always ignore '.' and '..' */
		if ((! strcmp (ent->d_name, "."))
		    || (! strcmp (ent->d_name, "..")))
			continue;

		NIH_MUST (subpath = nih_sprintf (paths, "%s/%s",
						 path, ent->d_name));

		if (filter && filter (subpath)) {
			nih_free (subpath);
			continue;
		}

		NIH_MUST (nih_str_array_addp (&paths, NULL, &npaths, subpath));
	}

	closedir (dir);

	qsort (paths, npaths, sizeof (char *), alphasort);

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
		NihDirEntry  *entry;
		char        **paths, **subpath;
		int           ret = 0;

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
		paths = nih_dir_walk_scan (path, filter);
		if (! paths)
			goto error;

		/* Record the device and inode numbers in the stack so that
		 * we can detect directory loops.
		 */
		NIH_MUST (entry = nih_new (dirs, NihDirEntry));
		nih_list_init (&entry->entry);
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

		nih_list_free (&entry->entry);
		nih_free (paths);

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
