/* libnih
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

#ifndef NIH_FILE_H
#define NIH_FILE_H

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <nih/macros.h>


/**
 * NihFileFilter:
 * @data: data pointer,
 * @path: path to file,
 * @is_dir: TRUE if @path is a directory.
 *
 * A file filter is a function that can be called to determine whether
 * a particular path should be ignored because of its filename.
 *
 * Returns: TRUE if the path should be ignored, FALSE otherwise.
 **/
typedef int (*NihFileFilter) (void *data, const char *path, int is_dir);

/**
 * NihFileVisitor:
 * @data: data pointer given to nih_dir_walk(),
 * @dirname: top-level path being walked,
 * @path: path to file,
 * @statbuf: stat of @path.
 *
 * A file visitor is a function that can be called for a filesystem object
 * visited by nih_dir_walk() that does not match the filter given to that
 * function.
 *
 * Returns: zero on success, negative value on raised error.
 **/
typedef int (*NihFileVisitor) (void *data, const char *dirname,
			       const char *path, struct stat *statbuf);

/**
 * NihFileErrorHandler:
 * @data: data pointer given to nih_dir_walk(),
 * @dirname: top-level path being walked,
 * @path: path to file,
 * @statbuf: stat of @path.
 *
 * A file error handler is a function called whenever the visitor function
 * returns a raised error, or the attempt to walk @path fails.  Note that
 * @statbuf might be invalid if it was stat() that failed.
 *
 * This function should handle the error and return zero; alternatively
 * it may raise the error again (or a different error) and return a negative
 * value to abort the tree walk.
 *
 * Returns: zero on success, negative value on raised error.
 **/
typedef int (*NihFileErrorHandler) (void *data, const char *dirname,
				    const char *path, struct stat *statbuf);


NIH_BEGIN_EXTERN

char *nih_file_read         (const void *parent, const char *path,
			     size_t *length)
	__attribute__ ((warn_unused_result, malloc));

void *nih_file_map          (const char *path, int flags, size_t *length)
	__attribute__ ((warn_unused_result));
int   nih_file_unmap        (void *map, size_t length);

int   nih_file_is_hidden    (const char *path);
int   nih_file_is_backup    (const char *path);
int   nih_file_is_swap      (const char *path);
int   nih_file_is_rcs       (const char *path);
int   nih_file_is_packaging (const char *path);
int   nih_file_ignore       (void *data, const char *path);

int   nih_dir_walk          (const char *path, NihFileFilter filter,
			     NihFileVisitor visitor, NihFileErrorHandler error,
			     void *data)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_FILE_H */
