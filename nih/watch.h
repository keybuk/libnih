/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NIH_WATCH_H
#define NIH_WATCH_H

#ifdef HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#else
# include <nih/inotify.h>
#endif /* HAVE_SYS_INOTIFY_H */

#include <sys/types.h>
#include <sys/stat.h>

#include <nih/macros.h>
#include <nih/list.h>
#include <nih/hash.h>
#include <nih/file.h>
#include <nih/io.h>


/* Predefine the typedefs as we use them in the callbacks */
typedef struct nih_watch NihWatch;

/**
 * NihCreateHandler:
 * @data: data pointer given when registered,
 * @watch: NihWatch for directory tree,
 * @path: full path to file,
 * @statbuf: stat of @path.
 *
 * A create handler is a function that is called whenever a file or other
 * object is created under or moved into a directory tree being watched.
 * @path contains the path to the file, including the directory prefix
 * which can be found in @watch.
 *
 * It is safe to remove the watch with nih_free() from this function.
 **/
typedef void (*NihCreateHandler) (void *data, NihWatch *watch,
				  const char *path, struct stat *statbuf);

/**
 * NihModifyHandler:
 * @data: data pointer given when registered,
 * @watch: NihWatch for directory tree,
 * @path: full path to file,
 * @statbuf: stat of @path.
 *
 * A modify handler is a function that is called whenever a file or other
 * object is changed within a directory tree being watched.  @path contains
 * the path to the file, including the directory prefix which can be
 * found in @watch.
 *
 * It is safe to remove the watch with nih_free() from this function.
 **/
typedef void (*NihModifyHandler) (void *data, NihWatch *watch,
				  const char *path, struct stat *statbuf);

/**
 * NihDeleteHandler:
 * @data: data pointer given when registered,
 * @watch: NihWatch for directory tree,
 * @path: full path to file.
 *
 * A delete handler is a function that is called whenever a file or other
 * object is deleted from or moved out of a directory tree being watched.
 * @path contains the path to the file, including the directory prefix
 * which can be found in @watch.
 *
 * If the directory being watched itself is deleted, or an error occurs
 * with the inotify socket (including closure), this function is called
 * with the top-level path as an argument (check watch->path).  It is
 * normal and safe to free the watch at this point.
 *
 * It is safe to remove the watch with nih_free() from this function.
 **/
typedef void (*NihDeleteHandler) (void *data, NihWatch *watch,
				  const char *path);


/**
 * NihWatch:
 * @fd: inotify instance,
 * @io: NihIo structure to watch @fd,
 * @path: full path to be watched,
 * @watches: list of watch descriptors,
 * @subdirs: include sub-directories of @path,
 * @create: call @create_handler for existing files,
 * @filter: function to filter paths watched,
 * @create_handler: function called when a path is created,
 * @modify_handler: function called when a path is modified,
 * @delete_handler: function called when a path is deleted,
 * @created: hash table of created files,
 * @data: pointer to pass to functions,
 * @free: allows free to be called within a handler.
 *
 * This structure represents an inotify instance that is watching @path,
 * and optionally sub-directories underneath it.  It can also be used to
 * just watch multiple different files calling the same functions for each.
 **/
struct nih_watch {
	int               fd;
	NihIo            *io;

	char             *path;
	NihList           watches;

	int               subdirs;
	int               create;
	NihFileFilter     filter;

	NihCreateHandler  create_handler;
	NihModifyHandler  modify_handler;
	NihDeleteHandler  delete_handler;

	NihHash          *created;

	void             *data;
	int              *free;
};

/**
 * NihWatchHandle:
 * @entry: entry in list,
 * @wd: inotify watch handle,
 * @path: path being watched.
 *
 * This structure represents an inotify watch on an individual @path with
 * a unique watch descriptor @wd.  They are stored in the watches list of
 * an NihWatch structure.
 **/
typedef struct nih_watch_handle {
	NihList  entry;

	int      wd;
	char    *path;
} NihWatchHandle;


NIH_BEGIN_EXTERN

NihWatch *nih_watch_new     (const void *parent, const char *path, int subdirs,
			     int create, NihFileFilter filter,
			     NihCreateHandler create_handler,
			     NihModifyHandler modify_handler,
			     NihDeleteHandler delete_handler, void *data)
	__attribute__ ((warn_unused_result, malloc));

int       nih_watch_add     (NihWatch *watch, const char *path, int subdirs)
	__attribute__ ((warn_unused_result));

int       nih_watch_destroy (NihWatch *watch);

NIH_END_EXTERN

#endif /* NIH_WATCH_H */
