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
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/io.h>
#include <nih/file.h>
#include <nih/logging.h>
#include <nih/error.h>


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
 * nih_dir_walk:
 * @path: path to walk,
 * @types: object types to call @visitor for,
 * @filter: path filter for both @visitor and iteration,
 * @visitor: function to call for each path,
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
 * @visitor is additionally only called for objects whose type is given
 * in @types, a bitmask of file modes and types as used by stat().
 * Leaving S_IFDIR out of @types only prevents @visitor being called for
 * directories, it does not prevent iteration into sub-directories.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_dir_walk (const char    *path,
	      mode_t         types,
	      NihFileFilter  filter,
	      NihFileVisitor visitor,
	      void          *data)
{
	DIR           *dir;
	struct dirent *ent;
	int            ret = 0;

	nih_assert (path != NULL);
	nih_assert (types != 0);
	nih_assert (visitor != NULL);

	dir = opendir (path);
	if (! dir)
		nih_return_system_error (-1);

	while ((ent = readdir (dir)) != NULL) {
		struct stat  statbuf;
		char        *subpath;

		/* Always ignore '.' and '..' */
		if ((! strcmp (ent->d_name, "."))
		    || (! strcmp (ent->d_name, "..")))
			continue;

		NIH_MUST (subpath = nih_sprintf (NULL, "%s/%s",
						 path, ent->d_name));

		/* Check the filter */
		if (filter && filter (subpath)) {
			nih_free (subpath);
			continue;
		}

		/* Not much we can do here if we can't at least stat it */
		if (stat (subpath, &statbuf) < 0) {
			nih_free (subpath);
			continue;
		}

		/* Call the handler if types match. */
		if (statbuf.st_mode & types) {
			ret = visitor (data, subpath);
			if (ret < 0) {
				nih_free (subpath);
				break;
			}
		}

		/* Iterate into sub-directories */
		if (S_ISDIR (statbuf.st_mode)) {
			ret = nih_dir_walk (subpath, types, filter,
					    visitor, data);
			if (ret < 0) {
				nih_free (subpath);
				break;
			}
		}

		nih_free (subpath);
	}

	closedir (dir);

	return ret;
}
