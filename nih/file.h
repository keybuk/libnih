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

#ifdef HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#else
# include <nih/inotify.h>
#endif /* HAVE_SYS_INOTIFY_H */

#include <sys/types.h>
#include <sys/stat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include <fcntl.h>


/* Predefine the typedefs as we use them in the callbacks */
typedef struct nih_file_watch NihFileWatch;
typedef struct nih_dir_watch  NihDirWatch;

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

/**
 * NihFileWatcher:
 * @data: data pointer given when registered,
 * @watch: NihFileWatch for which an event occurred,
 * @events: event mask that occurred,
 * @cookie: cookie to identify renames,
 * @name: optional name.
 *
 * A file watcher is a function that is called whenever a file event that
 * is being watched for occurrs.  The @event mask given is as described in
 * inotify(7), if watching a directory then @name will contain the name of
 * the child that was changed.
 *
 * The name of the file or directory being watched can be obtained from the
 * @watch structure.
 *
 * It is safe to remove a watch with nih_file_remove_watch() from this
 * function.
 **/
typedef void (*NihFileWatcher) (void *data, NihFileWatch *watch,
				uint32_t events, uint32_t cookie,
				const char *name);

/**
 * NihCreateHandler:
 * @data: data pointer given when registered,
 * @watch: NihDirWatch for directory tree,
 * @path: full path to file.
 *
 * A create handler is a function that is called whenever a file or other
 * object is created under or moved into a directory tree being watched.
 * @path contains the path to the file, including the directory prefix
 * which can be found in @watch.
 *
 * It is safe to remove the watch with nih_free() from this function.
 **/
typedef void (*NihCreateHandler) (void *data, NihDirWatch *watch,
				  const char *path);

/**
 * NihModifyHandler:
 * @data: data pointer given when registered,
 * @watch: NihDirWatch for directory tree,
 * @path: full path to file.
 *
 * A modify handler is a function that is called whenever a file or other
 * object is changed within a directory tree being watched.  @path contains
 * the path to the file, including the directory prefix which can be
 * found in @watch.
 *
 * It is safe to remove the watch with nih_free() from this function.
 **/
typedef void (*NihModifyHandler) (void *data, NihDirWatch *watch,
				  const char *path);

/**
 * NihDeleteHandler:
 * @data: data pointer given when registered,
 * @watch: NihDirWatch for directory tree,
 * @path: full path to file.
 *
 * A delete handler is a function that is called whenever a file or other
 * object is deleted from or moved out of a directory tree being watched.
 * @path contains the path to the file, including the directory prefix
 * which can be found in @watch.
 *
 * If the directory being watched itself is deleted, the function is called
 * with NULL for @path.  The watch is automatically freed once the function
 * returns.
 *
 * It is safe to remove the watch with nih_free() from this function.
 **/
typedef void (*NihDeleteHandler) (void *data, NihDirWatch *watch,
				  const char *path);


/**
 * NihFileWatch:
 * @entry: list header,
 * @wd: watch descriptor,
 * @path: path being watched,
 * @events: events being watched for,
 * @watcher: function called when events occur,
 * @data: pointer passed to @watcher.
 *
 * This structure represents an inotify watch on a single @path.  A single
 * inotify file descriptor is shared amongst all watches, so watches on
 * multiple files should have multiple NihFileWatch entries (optionally
 * with the same watcher function).
 *
 * @events is a bit mask of events as described in inotify(7).
 *
 * The watch may be terminated and freed by the using nih_file_remove_watch()
 * function.
 **/
struct nih_file_watch {
	NihList         entry;

	int             wd;
	char           *path;
	uint32_t        events;

	NihFileWatcher  watcher;
	void           *data;
};

/**
 * NihDirWatch:
 * @path: top-level path being watched,
 * @subdirs: whether sub-directories are also watched,
 * @filter: function called to filer path names,
 * @create_handler: function called when a file is created,
 * @modify_handler: function called when a file is changed,
 * @delete_handler: function called when a file is deleted,
 * @data: data passed to handler functions.
 *
 * This structure represents an inotify watch on a directory tree
 * beginning at @path.  The watches themselves are represented by
 * NihFileWatch structures which are children of this structure,
 * so the watch may be terminated by using nih_free().
 **/
struct nih_dir_watch {
	char             *path;
	int               subdirs;

	NihFileFilter     filter;

	NihCreateHandler  create_handler;
	NihModifyHandler  modify_handler;
	NihDeleteHandler  delete_handler;

	void             *data;
};


NIH_BEGIN_EXTERN

NihFileWatch *nih_file_add_watch    (const void *parent, const char *path,
				     uint32_t events, NihFileWatcher watcher,
				     void *data);
void          nih_file_remove_watch (NihFileWatch *watch);

int           nih_dir_walk          (const char *path, mode_t types,
				     NihFileFilter filter,
				     NihFileVisitor visitor, void *data);

NihDirWatch * nih_dir_add_watch     (const void *parent, const char *path,
				     int subdirs, NihFileFilter filter,
				     NihCreateHandler create_handler,
				     NihModifyHandler modify_handler,
				     NihDeleteHandler delete_handler,
				     void *data);

void *        nih_file_map          (const char *path, int flags,
				     size_t *length);
int           nih_file_unmap        (void *map, size_t length);

NIH_END_EXTERN

#endif /* NIH_FILE_H */
