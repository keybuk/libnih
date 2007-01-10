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

#include <nih/macros.h>
#include <nih/list.h>

#include <fcntl.h>


/* Predefine the typedefs as we use them in the callbacks */
typedef struct nih_file_watch NihFileWatch;
typedef struct nih_dir_watch  NihDirWatch;

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


NIH_BEGIN_EXTERN

NihFileWatch *nih_file_add_watch    (const void *parent, const char *path,
				     uint32_t events, NihFileWatcher watcher,
				     void *data);
void          nih_file_remove_watch (NihFileWatch *watch);

void *        nih_file_map          (const char *path, int flags,
				     size_t *length);
int           nih_file_unmap        (void *map, size_t length);

NIH_END_EXTERN

#endif /* NIH_FILE_H */
