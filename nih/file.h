/* libnih
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

#ifndef NIH_FILE_H
#define NIH_FILE_H

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <nih/macros.h>


/**
 * NihFileFilter:
 * @path: path to file.
 *
 * A file filter is a function that can be called to determine whether
 * a particular path should be ignored because of its filename,
 *
 * Returns: TRUE if the path should be ignored, FALSE otherwise.
 **/
typedef int (*NihFileFilter) (const char *path);

/**
 * NihFileVisitor:
 * @data: data pointer given to nih_dir_walk(),
 * @path: path to file.
 *
 * A file visitor is a function that is called for each file, directory or
 * other object visited by nih_dir_walk() that is does not match the
 * filter given to that function but does match the types argument.
 *
 * Returns: zero on success, negative value on raised error.
 **/
typedef int (*NihFileVisitor) (void *data, const char *path);


NIH_BEGIN_EXTERN

void *        nih_file_map          (const char *path, int flags,
				     size_t *length)
	__attribute__ ((warn_unused_result));
int           nih_file_unmap        (void *map, size_t length);

int           nih_dir_walk          (const char *path, mode_t types,
				     NihFileFilter filter,
				     NihFileVisitor visitor, void *data)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_FILE_H */
