/* libnih
 *
 * file.c - file watching
 *
 * Copyright © 2006 Scott James Remnant <scott@netsplit.com>.
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


#ifdef HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#else
# include <nih/inotify.h>
#endif /* HAVE_SYS_INOTIFY_H */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/io.h>
#include <nih/file.h>
#include <nih/logging.h>
#include <nih/error.h>


/* Prototypes for static functions */
static void nih_file_reader (void *data, NihIo *io,
			     const char *buf, size_t len);


/**
 * file_watches:
 *
 * List of all file watches, in no particular order.  Each item is an
 * NihFileWatch structure.
 **/
static NihList *file_watches = NULL;

/**
 * inotify_fd:
 *
 * inotify file descriptor we use for all watches.
 **/
static int inotify_fd = -1;


/**
 * nih_file_init:
 *
 * Initialise the file watches list and inotify file descriptor.
 **/
static inline void
nih_file_init (void)
{
	if (! file_watches)
		NIH_MUST (file_watches = nih_list_new (NULL));

	if (inotify_fd == -1) {
		inotify_fd = inotify_init ();
		if (inotify_fd < 0)
			return;

		NIH_MUST (nih_io_reopen (NULL, inotify_fd, NIH_IO_STREAM,
					 nih_file_reader, NULL, NULL, NULL));
	}
}


/**
 * nih_file_add_watch:
 * @parent: parent of watch,
 * @path: path to watch,
 * @events: events to watch for,
 * @watcher: function to call,
 * @data: data to pass to @watcher.
 *
 * Begins watching @path for the list of @events given which should be a
 * bitmask as described in inotify(7).  When any of the listed events
 * occur, @watcher is called.
 *
 * The watch structure is allocated using nih_alloc() and stored in a linked
 * list, a default destructor is set that removes the watch from the list
 * and terminates the inotify watch.  Removal of the watch can be performed
 * by freeing it.
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: new NihFileWatch structure or NULL on raised error.
 **/
NihFileWatch *
nih_file_add_watch (const void     *parent,
		    const char     *path,
		    uint32_t        events,
		    NihFileWatcher  watcher,
		    void           *data)
{
	NihFileWatch *watch;

	nih_assert (path != NULL);
	nih_assert (events != 0);

	nih_file_init ();
	if (inotify_fd < 0)
		nih_return_system_error (NULL);

	watch = nih_new (parent, NihFileWatch);
	if (! watch)
		nih_return_system_error (NULL);

	nih_list_init (&watch->entry);
	nih_alloc_set_destructor (watch, (NihDestructor)nih_file_remove_watch);

	watch->wd = inotify_add_watch (inotify_fd, path, events);
	if (watch->wd < 0) {
		nih_error_raise_system ();
		nih_free (watch);
		return NULL;
	}

	watch->path = nih_strdup (watch, path);
	watch->events = events;

	watch->watcher = watcher;
	watch->data = data;

	nih_list_add (file_watches, &watch->entry);

	return watch;
}

/**
 * nih_file_remove_watch:
 * @watch: watch to remove.
 *
 * Remove the watch on the path and events mask associated with the @wwatch
 * given and remove it from the list of watches.
 *
 * The structure itself is not freed.
 **/
void
nih_file_remove_watch (NihFileWatch *watch)
{
	nih_assert (watch != NULL);

	if (watch->wd < 0)
		return;

	inotify_rm_watch (inotify_fd, watch->wd);
	watch->wd = -1;

	nih_list_remove (&watch->entry);
}


/**
 * nih_file_reader:
 * @data: ignored,
 * @io: watch on file descriptor,
 * @buf: buffer bytes available,
 * @len: length of @buf.
 *
 * This function is called whenever data has been read from the inotify
 * file descriptor and is waiting to be processed.  It reads data from the
 * buffer in inotify_event sized chunks, also reading the name afterwards
 * if expected.
 **/
static void
nih_file_reader (void       *data,
		 NihIo      *io,
		 const char *buf,
		 size_t      len)
{
	struct inotify_event *event;
	size_t                sz;

	nih_assert (io != NULL);
	nih_assert (buf != NULL);
	nih_assert (len > 0);

	while (len > 0) {
		/* Wait until there's a complete event waiting
		 * (should always be true, but better to be safe than sorry)
		 */
		sz = sizeof (struct inotify_event);
		if (len < sz)
			return;

		/* Never read an event without its name (again should always be
		 * true
		 */
		event = (struct inotify_event *) buf;
		sz += event->len;
		if (len < sz)
			return;

		/* Read the data (allocates the event structure, etc.)
		 * Force this, otherwise we won't get called until the
		 * next inotify event.
		 */
		NIH_MUST (event = (struct inotify_event *)nih_io_read (
				  NULL, io, &sz));
		len -= sz;

		/* Handle it */
		NIH_LIST_FOREACH_SAFE (file_watches, iter) {
			NihFileWatch *watch = (NihFileWatch *)iter;

			if (watch->wd != event->wd)
				continue;

			watch->watcher (watch->data, watch, event->mask,
					event->len ? event->name : NULL);
		}

		nih_free (event);
	}
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
