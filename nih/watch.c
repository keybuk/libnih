/* libnih
 *
 * watch.c - watching of files and directories with inotify
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


#ifdef HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#else
# include <nih/inotify.h>
#endif /* HAVE_SYS_INOTIFY_H */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>
#include <nih/io.h>
#include <nih/file.h>
#include <nih/watch.h>
#include <nih/logging.h>
#include <nih/error.h>


/**
 * INOTIFY_EVENTS:
 *
 * The standard set of inotify events we use for watching any path; if
 * people want a different set, they can use inotify_add_watch() directly.
 **/
#define INOTIFY_EVENTS (IN_CREATE | IN_DELETE | IN_CLOSE_WRITE \
			| IN_MOVE | IN_MOVE_SELF)


/* Prototypes for static functions */
static NihWatchHandle *nih_watch_handle_by_wd   (NihWatch *watch, int wd);
static NihWatchHandle *nih_watch_handle_by_path (NihWatch *watch,
						 const char *path);
static int             nih_watch_add_visitor    (NihWatch *watch,
						 const char *dirname,
					         const char *path,
						 struct stat *statbuf)
	__attribute__ ((warn_unused_result));
static void            nih_watch_reader      (NihWatch *watch, NihIo *io,
					      const char *buf, size_t len);
static void            nih_watch_handle      (NihWatch *watch,
					      NihWatchHandle *handle,
					      uint32_t events, uint32_t cookie,
					      const char *name);


/**
 * nih_watch_new:
 * @parent: parent of new structure,
 * @path: full path to be watched,
 * @subdirs: include sub-directories of @path,
 * @create: call @create_handler for existing files,
 * @filter: function to filter paths watched,
 * @create_handler: function called when a path is created,
 * @modify_handler; function called when a path is modified,
 * @delete_handler: function called when a path is deleted,
 * @data: pointer to pass to functions.
 *
 * Watches @path for changes, which may be either a single file or a
 * directory.  If @path is a directory, then sub-directories can be included
 * by setting @subdirs to TRUE; both existing and newly created
 * sub-directories will be automatically watched.
 *
 * Additionally, the set of files and directories within @path can be
 * limited by passing a @filter function which will recieve the paths and
 * may return TRUE to indicate that the path received should not be watched.
 *
 * When a file is created within @path, or moved from outside this location
 * into it, @create_handler will be called.  If @path is removed, or a file
 * is removed within @path, or moved to a location outside it,
 * @delete_handler will be called.  Finally if @path is modified, or a file
 * within it is modified, @modify_handler will be called.
 *
 * If @create is TRUE, @create_handler will also be called for all of the
 * files that exist under @path when the watch is first added.  This only
 * occurs if the watch can be added.
 *
 * This is a very high level wrapped around the inotify API; lower levels
 * can be obtained using the inotify API itself and some of the helper
 * functions used by this one.
 *
 * The returned watch structure is allocated with nih_alloc(), and contains
 * open inotify descriptors and child structures including NihIo.  It should
 * be freed using nih_watch_free().
 *
 * If @parent is not NULL, it should be a pointer to another allocated
 * block which will be used as the parent for this block.  When @parent
 * is freed, the returned string will be freed too.  If you have clean-up
 * that would need to be run, you can assign a destructor function using
 * the nih_alloc_set_destructor() function.
 *
 * Returns: new NihWatch structure, or NULL on raised error.
 **/
NihWatch *
nih_watch_new (const void       *parent,
	       const char       *path,
	       int               subdirs,
	       int               create,
	       NihFileFilter     filter,
	       NihCreateHandler  create_handler,
	       NihModifyHandler  modify_handler,
	       NihDeleteHandler  delete_handler,
	       void             *data)
{
	NihWatch *watch;

	nih_assert (path != NULL);

	/* Allocate the NihWatch structure */
	NIH_MUST (watch = nih_new (parent, NihWatch));
	NIH_MUST (watch->path = nih_strdup (watch, path));

	watch->subdirs = subdirs;
	watch->create = create;
	watch->filter = filter;

	watch->create_handler = create_handler;
	watch->modify_handler = modify_handler;
	watch->delete_handler = delete_handler;
	watch->data = data;

	watch->free = NULL;


	/* Open an inotify instance file descriptor */
	watch->fd = inotify_init ();
	if (watch->fd < 0) {
		nih_error_raise_system ();
		nih_free (watch);
		return NULL;
	}

	/* Add the path (and subdirs) to the list of watches */
	nih_list_init (&watch->watches);

	if (nih_watch_add (watch, path, subdirs) < 0) {
		close (watch->fd);
		nih_free (watch);
		return NULL;
	}


	/* Create an NihIo to handle incoming events.  This can't be
	 * an nih_alloc child because we need to be able to lazy close it.
	 */
	while (! (watch->io = nih_io_reopen (NULL, watch->fd, NIH_IO_STREAM,
					     (NihIoReader)nih_watch_reader,
					     NULL, NULL, watch))) {
		NihError *err;

		err = nih_error_get ();
		if (err->number != ENOMEM) {
			close (watch->fd);
			nih_free (watch);
			return NULL;
		}

		nih_free (err);
	}

	return watch;
}


/**
 * nih_watch_handle_by_wd:
 * @watch: watch to search,
 * @wd: inotify watch descriptor.
 *
 * Searches @watch for the path that @wd is handling.
 *
 * Returns: NihWatchHandle for @wd, or NULL if none known.
 **/
static NihWatchHandle *
nih_watch_handle_by_wd (NihWatch *watch,
			int       wd)
{
	nih_assert (watch != NULL);
	nih_assert (wd >= 0);

	NIH_LIST_FOREACH (&watch->watches, iter) {
		NihWatchHandle *handle = (NihWatchHandle *)iter;

		if (handle->wd == wd)
			return handle;
	}

	return NULL;
}

/**
 * nih_watch_handle_by_path:
 * @watch: watch to search,
 * @path: path being watched.
 *
 * Searches @watch for the watch descriptor that is handling @path.
 *
 * Returns: NihWatchHandle for @path, or NULL if none known.
 **/
static NihWatchHandle *
nih_watch_handle_by_path (NihWatch   *watch,
			  const char *path)
{
	nih_assert (watch != NULL);
	nih_assert (path != NULL);

	NIH_LIST_FOREACH (&watch->watches, iter) {
		NihWatchHandle *handle = (NihWatchHandle *)iter;

		if (! strcmp (handle->path, path))
			return handle;
	}

	return NULL;
}


/**
 * nih_watch_add:
 * @watch: NihWatch to change,
 * @path: path to be watched,
 * @subdirs: also watch sub-directories?
 *
 * Adds a new @path to be watched to the existing @watch structure, the
 * same handlers will be called.  @path need not be related to the path
 * originally given to @watch.
 *
 * If @subdirs is TRUE, and @path is a directory, then sub-directories of
 * the path are also watched.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
nih_watch_add (NihWatch   *watch,
	       const char *path,
	       int         subdirs)
{
	NihWatchHandle *handle;

	nih_assert (watch != NULL);
	nih_assert (path != NULL);

	/* Allocate the NihWatchHandle structure */
	NIH_MUST (handle = nih_new (watch, NihWatchHandle));
	NIH_MUST (handle->path = nih_strdup (handle, path));

	nih_list_init (&handle->entry);

	/* Get a watch descriptor for the path */
	handle->wd = inotify_add_watch (watch->fd, path, INOTIFY_EVENTS);
	if (handle->wd < 0) {
		nih_error_raise_system ();
		nih_free (handle);
		return -1;
	}

	/* Check for duplicates in the list */
	if (nih_watch_handle_by_wd (watch, handle->wd)) {
		nih_free (handle);
		return 0;
	}

	nih_list_add (&watch->watches, &handle->entry);

	/* Recurse into sub-directories, attempting to add a watch for each
	 * one; errors within the walk are warned automatically, so if this
	 * fails, it means we literally couldn't watch the top-level.
	 */
	if (subdirs && (nih_dir_walk (path, watch->filter,
				      (NihFileVisitor)nih_watch_add_visitor,
				      NULL, watch) < 0)) {
		NihError *err;

		err = nih_error_get ();
		if (err->number != ENOTDIR) {
			nih_error_raise_again (err);
			nih_list_free (&handle->entry);
			return -1;
		}
	}

	return 0;
}

/**
 * nih_watch_add_visitor:
 * @watch: watch to add to,
 * @dirname: top-level being watched,
 * @path: path to add,
 * @statbuf: stat of @path.
 *
 * Callback function for nih_dir_walk(), used by nih_watch_add() to add
 * sub-directories.  Just calls nih_watch_add() with subdirs as FALSE for
 * each directory found.
 *
 * If the create member of @watch is TRUE, it also calls the create handle
 * for each path found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
static int
nih_watch_add_visitor (NihWatch    *watch,
		       const char  *dirname,
		       const char  *path,
		       struct stat *statbuf)
{
	nih_assert (watch != NULL);
	nih_assert (dirname != NULL);
	nih_assert (path != NULL);
	nih_assert (statbuf != NULL);

	if (watch->create && watch->create_handler)
		watch->create_handler (watch->data, watch, path, statbuf);

	if (S_ISDIR (statbuf->st_mode)) {
		int ret;

		ret = nih_watch_add (watch, path, FALSE);
		if (ret < 0)
			return ret;
	}

	return 0;
}


/**
 * nih_watch_free:
 * @watch: NihWatch to be freed.
 *
 * Closes the inotify descriptor and frees both the NihIo and NihWatch
 * instances handling it.
 *
 * Returns: value returned from destructor.
 **/
int
nih_watch_free (NihWatch *watch)
{
	nih_assert (watch != NULL);

	if (watch->free) {
		*watch->free = TRUE;
		return 0;
	}

	nih_io_close (watch->io);
	return nih_free (watch);
}


/**
 * nih_watch_reader:
 * @watch: NihWatch for descriptor,
 * @io: NihIo with data to be read,
 * @buf: buffer data is available in,
 * @len: bytes in @buf.
 *
 * This function is called whenever there is data to be read on the inotify
 * file descriptor associated with @watch.  Each event in the buffer is read,
 * including the following name, and handled by calling one of the functions
 * in @watch.
 **/
static void
nih_watch_reader (NihWatch   *watch,
		  NihIo      *io,
		  const char *buf,
		  size_t      len)
{
	int lazy_free;

	nih_assert (watch != NULL);
	nih_assert (io != NULL);
	nih_assert (buf != NULL);
	nih_assert (len > 0);

	lazy_free = FALSE;
	if (! watch->free)
		watch->free = &lazy_free;

	while (len > 0) {
		NihWatchHandle       *handle;
		struct inotify_event *event;
		size_t                sz;

		/* Wait until there's a complete event waiting
		 * (should always be true, but better to be safe than sorry)
		 */
		sz = sizeof (struct inotify_event);
		if (len < sz)
			goto finish;

		/* Never read an event without its name (again should always
		 * be true
		 */
		event = (struct inotify_event *) buf;
		sz += event->len;
		if (len < sz)
			goto finish;

		/* Find the handle for this watch */
		handle = nih_watch_handle_by_wd (watch, event->wd);
		if (handle)
			nih_watch_handle (watch, handle, event->mask,
					  event->cookie, event->name);

		/* Remove the event from the front of the buffer, and
		 * decrease our own length counter.
		 */
		nih_io_buffer_shrink (io->recv_buf, sz);
		len -= sz;
	}

finish:
	if (watch->free == &lazy_free)
		watch->free = NULL;
	if (lazy_free)
		nih_watch_free (watch);
}

/**
 * nih_watch_handle:
 * @watch: NihWatch for descriptor,
 * @handle: NihWatchHandle for individual path,
 * @events: inotify events mask,
 * @cookie: unique cookie for renames,
 * @name: name of path under @handle.
 *
 * This handler is called when an event occurs for an individual watch
 * handle, it deals with the event and ensures that the @watch handlers
 * are called.
 **/
static void
nih_watch_handle (NihWatch       *watch,
		  NihWatchHandle *handle,
		  uint32_t        events,
		  uint32_t        cookie,
		  const char     *name)
{
	struct stat  statbuf;
	char        *path;

	nih_assert (watch != NULL);
	nih_assert (handle != NULL);

	/* First check whether this event is being caused by the actual
	 * path being watched by the handle being deleted or moved.  In
	 * either case, we drop the watch because we've lost the path.
	 */
	if ((events & IN_IGNORED) || (events & IN_MOVE_SELF)) {
		if (watch->delete_handler)
			watch->delete_handler (watch->data, watch,
					       handle->path);

		nih_debug ("Ceasing watch on %s", handle->path);
		nih_list_free (&handle->entry);
		return;
	}


	/* Every other event must come with a name. */
	if ((! name) || strchr (name, '/'))
		return;

	NIH_MUST (path = nih_sprintf (watch, "%s/%s", handle->path, name));

	/* Check the filter */
	if (watch->filter && watch->filter (path))
		goto finish;

	/* Handle it differently depending on the events mask */
	if ((events & IN_CREATE) || (events & IN_MOVED_TO)) {
		if (stat (path, &statbuf) < 0)
			goto finish;

		if (watch->create_handler)
			watch->create_handler (watch->data, watch,
					       path, &statbuf);

		/* See if it's a sub-directory, and we're handling those
		 * ourselves.  Add a watch to the directory and any
		 * sub-directories within it.
		 */
		if (watch->subdirs && S_ISDIR (statbuf.st_mode)) {
			if (nih_watch_add (watch, path, TRUE) < 0) {
				NihError *err;

				err = nih_error_get ();
				nih_warn ("%s: %s: %s", path,
					  _("Unable to watch directory"),
					  err->message);
				nih_free (err);
			}
		}

	} else if (events & IN_CLOSE_WRITE) {
		if (stat (path, &statbuf) < 0)
			goto finish;

		if (watch->modify_handler)
			watch->modify_handler (watch->data, watch,
					       path, &statbuf);

	} else if ((events & IN_DELETE) || (events & IN_MOVED_FROM)) {
		NihWatchHandle *path_handle;

		if (watch->delete_handler)
			watch->delete_handler (watch->data, watch, path);

		/* If there's a watch for that path, we act as if it got
		 * IN_IGNORED or IN_MOVE_SELF; just in case it's a symlink
		 * that's getting removed or something.
		 */
		path_handle = nih_watch_handle_by_path (watch, path);
		if (path_handle) {
			nih_debug ("Ceasing watch on %s", path_handle->path);
			nih_list_free (&path_handle->entry);
		}
	}

finish:
	nih_free (path);
}
