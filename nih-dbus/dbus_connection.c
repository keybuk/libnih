/* libnih
 *
 * dbus_connection.c - D-Bus client, bus and server connection handling
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


#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/timer.h>
#include <nih/io.h>
#include <nih/main.h>
#include <nih/logging.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>

#include "dbus_connection.h"


/* Prototypes for static functions */
static dbus_bool_t       nih_dbus_add_watch         (DBusWatch *watch,
						     void *data);
static void              nih_dbus_remove_watch      (DBusWatch *watch,
						     void *data);
static void              nih_dbus_watch_toggled     (DBusWatch *watch,
						     void *data);
static void              nih_dbus_watcher           (DBusWatch *watch,
						     NihIoWatch *io_watch,
						     NihIoEvents events);
static dbus_bool_t       nih_dbus_add_timeout       (DBusTimeout *timeout,
						     void *data);
static void              nih_dbus_remove_timeout    (DBusTimeout *timeout,
						     void *data);
static void              nih_dbus_timeout_toggled   (DBusTimeout *timeout,
						     void *data);
static void              nih_dbus_timer             (DBusTimeout *timeout,
						     NihTimer *timer);
static void              nih_dbus_wakeup_main       (void *data);
static void              nih_dbus_callback          (DBusConnection *connection,
						     NihMainLoopFunc *loop);
static DBusHandlerResult nih_dbus_connection_disconnected (DBusConnection *connection,
							   DBusMessage *message,
							   NihDBusDisconnectHandler handler);
static void              nih_dbus_new_connection    (DBusServer *server,
						     DBusConnection *connection,
						     void *data);


/**
 * main_loop_slot:
 *
 * Slot we use to store the main loop function in the connection.
 **/
static dbus_int32_t main_loop_slot = -1;

/**
 * connect_handler_slot:
 *
 * Slot we use to store the connection handler in the server.
 **/
static dbus_int32_t connect_handler_slot = -1;

/**
 * disconnect_handler_slot:
 *
 * Slot we use to store the disconnect handler in the server.
 **/
static dbus_int32_t disconnect_handler_slot = -1;


/**
 * nih_dbus_connect:
 * @address: address of D-Bus bus or server,
 * @disconnect_handler: function to call on disconnection.
 *
 * Establishes a connection to the D-Bus bus or server at @address
 * (specified in D-Bus's own address syntax) and sets up the connection
 * within libnih's own main loop so that messages will be received, sent
 * and dispatched automatically.
 *
 * The returned connection object IS NOT allocated with nih_alloc() and
 * is instead allocated and managed by the D-Bus library, it may not be
 * used as a context for other allocations.  Instead you should use
 * D-Bus data slots and free functions to attach other data to this.
 *
 * The connection object is shared and will persist as long as the
 * server maintains the connection.  You may prematurely terminate the
 * connection with dbus_connection_unref().
 *
 * Returns: new D-Bus connection object or NULL on raised error.
 **/
DBusConnection *
nih_dbus_connect (const char *             address,
		  NihDBusDisconnectHandler disconnect_handler)
{
	DBusConnection *connection;
	DBusError       error;

	nih_assert (address != NULL);

	dbus_error_init (&error);

	connection = dbus_connection_open (address, &error);
	if (! connection) {
		if (! strcmp (error.name, DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise (ENOMEM, strerror (ENOMEM));
		} else {
			nih_dbus_error_raise (error.name, error.message);
		}
		dbus_error_free (&error);

		return NULL;
	}

	if (nih_dbus_setup (connection, disconnect_handler) < 0) {
		dbus_connection_unref (connection);

		nih_return_no_memory_error (NULL);
	}

	return connection;
}

/**
 * nih_dbus_bus:
 * @bus: D-Bus bus type to connect to,
 * @disconnect_handler: function to call on disconnection.
 *
 * Establishes a connection to the given D-Bus @bus and sets up
 * the connection within libnih's own main loop so that messages will be
 * received, sent and dispatched automatically.
 *
 * Unlike the ordinary D-Bus API, this connection will not cause the exit()
 * function to be called should the bus go away.
 *
 * The returned connection object IS NOT allocated with nih_alloc() and
 * is instead allocated and managed by the D-Bus library, it may not be
 * used as a context for other allocations.  Instead you should use
 * D-Bus data slots and free functions to attach other data to this.
 *
 * The connection object is shared and will persist as long as the
 * server maintains the connection.  You may prematurely terminate the
 * connection with dbus_connection_unref().
 *
 * Returns: new D-Bus connection object or NULL on raised error.
 **/
DBusConnection *
nih_dbus_bus (DBusBusType              bus,
	      NihDBusDisconnectHandler disconnect_handler)
{
	DBusConnection *connection;
	DBusError       error;

	dbus_error_init (&error);

	connection = dbus_bus_get (bus, &error);
	if (! connection) {
		if (! strcmp (error.name, DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise (ENOMEM, strerror (ENOMEM));
		} else {
			nih_dbus_error_raise (error.name, error.message);
		}
		dbus_error_free (&error);

		return NULL;
	}

	dbus_connection_set_exit_on_disconnect (connection, FALSE);

	if (nih_dbus_setup (connection, disconnect_handler) < 0) {
		dbus_connection_unref (connection);

		nih_return_no_memory_error (NULL);
	}

	return connection;
}

/**
 * nih_dbus_setup:
 * @connection: D-Bus connection to setup,
 * @disconnect_handler: function to call on disconnection.
 *
 * Sets up the given connection @connection so that it may use libnih's own
 * main loop meaning that messages will be received, sent and dispatched
 * automatically.
 *
 * This will also set up a handler for the disconnected signal that will
 * automatically unreference the connection after calling the given
 * @disconnect_handler.
 *
 * It's safe to call this function multiple times for a single @connection,
 * for example for setting an additional @disconnect_handler for a shared
 * connection.
 *
 * Returns: zero on success, negative value on insufficient memory.
 **/
int
nih_dbus_setup (DBusConnection *         connection,
		NihDBusDisconnectHandler disconnect_handler)
{
	NihMainLoopFunc *loop;

	nih_assert (connection != NULL);

	/* Allocate a data slot for storing the main loop function; if
	 * this is set for the structure, we've already set it up before
	 * and this is being shared so we can skip down to just adding
	 * the new disconnect handler.
	 */
 	if (! dbus_connection_allocate_data_slot (&main_loop_slot))
		return -1;

	if (! dbus_connection_get_data (connection, main_loop_slot)) {
		/* Allow the connection to watch its file descriptors */
		if (! dbus_connection_set_watch_functions (connection,
							   nih_dbus_add_watch,
							   nih_dbus_remove_watch,
							   nih_dbus_watch_toggled,
							   NULL, NULL))
			goto error;

		/* Allow the connection to set up timers */
		if (! dbus_connection_set_timeout_functions (connection,
							     nih_dbus_add_timeout,
							     nih_dbus_remove_timeout,
							     nih_dbus_timeout_toggled,
							     NULL, NULL))
			goto error;

		/* Allow the connection to wake up the main loop */
		dbus_connection_set_wakeup_main_function (connection,
							  nih_dbus_wakeup_main,
							  NULL, NULL);

		/* Add the main loop function and store it in the data slot,
		 * this means it will be automatically freed.  Until this
		 * succeeds, all of the above functions will be reset each
		 * time.
		 */
		loop = nih_main_loop_add_func (NULL, (NihMainLoopCb)nih_dbus_callback,
					       connection);
		if (! loop)
			goto error;

		if (! dbus_connection_set_data (connection, main_loop_slot, loop,
						(DBusFreeFunction)nih_discard)) {
			nih_free (loop);
			goto error;
		}
	}

	/* Add the filter for the disconnect handler (which may be NULL,
	 * but even then we have to unreference it).  If this fails, and
	 * we call again, we'll act as though it's a shared connection
	 * which has the right effect.
	 */
	if (! dbus_connection_add_filter (
		    connection, (DBusHandleMessageFunction)nih_dbus_connection_disconnected,
		    disconnect_handler, NULL))
		return -1;

	return 0;

error:
	/* Unwind setup of a non-shared connection so that next time we call,
	 * we're not in a strange half-done state.
	 */
	dbus_connection_set_watch_functions (connection,
					     NULL, NULL, NULL,
					     NULL, NULL);
	dbus_connection_set_timeout_functions (connection,
					       NULL, NULL, NULL,
					       NULL, NULL);
	dbus_connection_set_wakeup_main_function (connection,
						  NULL,
						  NULL, NULL);

	return -1;
}


/**
 * nih_dbus_server:
 * @address: intended address of D-Bus server,
 * @connect_handler: function to call on new connections,
 * @disconnect_handler: function to call on disconnection of connections.
 *
 * Creates a listening D-Bus server at @address (specified in D-Bus's own
 * address syntax) and sets up the server within libnih's own main loop
 * so that socket events will be handled automatically.
 *
 * New connections are accepted if the @connect_handler returns TRUE and
 * they too set up within libnih's own main loop so that messages will be
 * received, sent and dispatched.  If those connections are disconnected,
 * @disconnect_handler will be called for them and they will be
 * automatically unreferenced.
 *
 * The returned server object and any created connection objects ARE NOT
 * allocated with nih_alloc() and are instead allocated and managed by the
 * D-Bus library, they may not be used as a context for other allocations.
 * Instead you should use D-Bus data slots and free functions to attach
 * other data to them.
 *
 * Both the server object and any created connection objects are private,
 * you may close and unreference them when you are finished with them.
 *
 * Returns: new D-Bus server object or NULL on raised error.
 **/
DBusServer *
nih_dbus_server (const char *             address,
		 NihDBusConnectHandler    connect_handler,
		 NihDBusDisconnectHandler disconnect_handler)
{
	DBusServer *server;
	DBusError   error;

	nih_assert (address != NULL);

	dbus_error_init (&error);

	server = dbus_server_listen (address, &error);
	if (! server) {
		if (! strcmp (error.name, DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise (ENOMEM, strerror (ENOMEM));
		} else {
			nih_dbus_error_raise (error.name, error.message);
		}
		dbus_error_free (&error);

		return NULL;
	}

	/* Allocate a slot to store the connect handler */
	if (! dbus_server_allocate_data_slot (&connect_handler_slot))
		goto error;

	if (! dbus_server_set_data (server, connect_handler_slot,
				    connect_handler, NULL))
		goto error;

	/* Allocate a slot to store the disconnect handler */
	if (! dbus_server_allocate_data_slot (&disconnect_handler_slot))
		goto error;

	if (! dbus_server_set_data (server, disconnect_handler_slot,
				    disconnect_handler, NULL))
		goto error;

	/* Allow the server to watch its file descriptors */
	if (! dbus_server_set_watch_functions (server,
					       nih_dbus_add_watch,
					       nih_dbus_remove_watch,
					       nih_dbus_watch_toggled,
					       NULL, NULL))
		goto error;

	/* Allow the server to set up timers */
	if (! dbus_server_set_timeout_functions (server,
						 nih_dbus_add_timeout,
						 nih_dbus_remove_timeout,
						 nih_dbus_timeout_toggled,
						 NULL, NULL))
		goto error;

	/* Set the function to be called for new connectoins */
	dbus_server_set_new_connection_function (server,
						 nih_dbus_new_connection,
						 NULL, NULL);

	return server;

error:
	dbus_server_disconnect (server);
	dbus_server_unref (server);

	nih_return_no_memory_error (NULL);
}


/**
 * nih_dbus_add_watch:
 * @watch: D-Bus watch to be added,
 * @data: not used.
 *
 * Called by D-Bus to register the given file descriptor @watch in our main
 * loop; we create an NihIoWatch structure for it with events matching the
 * watch's flags - even if the watch is not enabled (in which case we remove
 * it from the watch list).
 *
 * The NihIoWatch is stored in the watch's data member.
 *
 * Returns: TRUE if the watch could be added, FALSE on insufficient memory.
 **/
static dbus_bool_t
nih_dbus_add_watch (DBusWatch *watch,
		    void *     data)
{
	NihIoWatch *io_watch;
	int         fd;
	int         flags;
	NihIoEvents events = NIH_IO_EXCEPT;

	nih_assert (watch != NULL);
	nih_assert (dbus_watch_get_data (watch) == NULL);

	fd = dbus_watch_get_unix_fd (watch);
	nih_assert (fd >= 0);

	flags = dbus_watch_get_flags (watch);
	if (flags & DBUS_WATCH_READABLE)
		events |= NIH_IO_READ;
	if (flags & DBUS_WATCH_WRITABLE)
		events |= NIH_IO_WRITE;

	io_watch = nih_io_add_watch (NULL, fd, events,
				     (NihIoWatcher)nih_dbus_watcher, watch);
	if (! io_watch)
		return FALSE;

	dbus_watch_set_data (watch, io_watch, (DBusFreeFunction)nih_discard);

	if (! dbus_watch_get_enabled (watch))
		nih_list_remove (&io_watch->entry);

	return TRUE;
}

/**
 * nih_dbus_remove_watch:
 * @watch: D-Bus watch to be removed,
 * @data: not used.
 *
 * Called by D-Bus to unregister the given file descriptor @watch from our
 * main loop; we take the NihIoWatch structure from the watch's data member
 * and free it.
 **/
static void
nih_dbus_remove_watch (DBusWatch *watch,
		       void *     data)
{
	NihIoWatch *io_watch;

	nih_assert (watch != NULL);

	io_watch = dbus_watch_get_data (watch);
	nih_assert (io_watch != NULL);

	/* Only remove it from the list, D-Bus will call nih_free for us
	 * when we set the data to NULL.
	 **/
	nih_list_remove (&io_watch->entry);

	dbus_watch_set_data (watch, NULL, NULL);
}

/**
 * nih_dbus_watch_toggled:
 * @watch: D-Bus watch to be toggled,
 * @data: not used.
 *
 * Called by D-Bus because the given file descriptor @watch has been enabled
 * or disabled; we take the NihIoWatch structure from the watch's data member
 * and either add it to or remove it from the watches list.
 **/
static void
nih_dbus_watch_toggled (DBusWatch *watch,
			void *     data)
{
	NihIoWatch *io_watch;
	int         flags;
	NihIoEvents events = NIH_IO_EXCEPT;

	nih_assert (watch != NULL);

	io_watch = dbus_watch_get_data (watch);
	nih_assert (io_watch != NULL);

	/* D-Bus may toggle the watch in an attempt to change the flags */
	flags = dbus_watch_get_flags (watch);
	if (flags & DBUS_WATCH_READABLE)
		events |= NIH_IO_READ;
	if (flags & DBUS_WATCH_WRITABLE)
		events |= NIH_IO_WRITE;

	if (dbus_watch_get_enabled (watch)) {
		nih_list_add (nih_io_watches, &io_watch->entry);
	} else {
		nih_list_remove (&io_watch->entry);
	}
}

/**
 * nih_dbus_watcher:
 * @watch: D-Bus watch event occurred for,
 * @io_watch: NihIoWatch for which an event occurred,
 * @events: events that occurred.
 *
 * Called because an event has occurred on @io_watch that we need to pass
 * onto the underlying @watch.
 **/
static void
nih_dbus_watcher (DBusWatch * watch,
		  NihIoWatch *io_watch,
		  NihIoEvents events)
{
	int flags = 0;

	nih_assert (watch != NULL);
	nih_assert (io_watch != NULL);

	if (events & NIH_IO_READ)
		flags |= DBUS_WATCH_READABLE;
	if (events & NIH_IO_WRITE)
		flags |= DBUS_WATCH_WRITABLE;
	if (events & NIH_IO_EXCEPT)
		flags |= DBUS_WATCH_ERROR;

	dbus_watch_handle (watch, flags);
}


/**
 * nih_dbus_add_timeout:
 * @timeout: D-Bus timeout to be added,
 * @data: not used.
 *
 * Called by D-Bus to register the given @timeout in our main loop; we create
 * a periodic NihTimer structure for it with the correct interval even if
 * the timeout is not enabled (in which case we remove it from the timer
 * list).
 *
 * The NihTimer is stored in the timeout's data member.
 *
 * Returns: TRUE if the timeout could be added, FALSE on insufficient memory.
 **/
static dbus_bool_t
nih_dbus_add_timeout (DBusTimeout *timeout,
		      void *       data)
{
	NihTimer *timer;
	int       interval;

	nih_assert (timeout != NULL);
	nih_assert (dbus_timeout_get_data (timeout) == NULL);

	interval = dbus_timeout_get_interval (timeout);

	timer = nih_timer_add_periodic (NULL, (interval - 1) / 1000 + 1,
					(NihTimerCb)nih_dbus_timer, timeout);
	if (! timer)
		return FALSE;

	dbus_timeout_set_data (timeout, timer, (DBusFreeFunction)nih_discard);

	if (! dbus_timeout_get_enabled (timeout))
		nih_list_remove (&timer->entry);

	return TRUE;
}

/**
 * nih_dbus_remove_timeout:
 * @timeout: D-Bus timeout to be removed,
 * @data: not used.
 *
 * Called by D-Bus to unregister the given @timeout from our main loop; we
 * take the NihTimer structure from the timeout's data member and free it.
 **/
static void
nih_dbus_remove_timeout (DBusTimeout *timeout,
			 void *       data)
{
	NihTimer *timer;

	nih_assert (timeout != NULL);

	timer = dbus_timeout_get_data (timeout);
	nih_assert (timer != NULL);

	/* Only remove it from the list, D-Bus will call nih_free for us
	 * when we set the data to NULL.
	 */
	nih_list_remove (&timer->entry);

	dbus_timeout_set_data (timeout, NULL, NULL);
}

/**
 * nih_dbus_timeout_toggled:
 * @timeout: D-Bus timeout to be toggled,
 * @data: not used.
 *
 * Called by D-Bus because the @timeout has been enabled or disabled; we
 * take the NihTimer structure from the timeout's data member and either
 * add it to or remove it from the timers list.
 **/
static void
nih_dbus_timeout_toggled (DBusTimeout *timeout,
			  void *       data)
{
	NihTimer *      timer;
	int             interval;
	struct timespec now;

	nih_assert (timeout != NULL);

	timer = dbus_timeout_get_data (timeout);
	nih_assert (timer != NULL);

	/* D-Bus may toggle the timer in an attempt to change the timeout */
	interval = dbus_timeout_get_interval (timeout);

	nih_assert (clock_gettime (CLOCK_MONOTONIC, &now) == 0);

	timer->period = (interval - 1) / 1000 + 1;
	timer->due = now.tv_sec + timer->period;

	if (dbus_timeout_get_enabled (timeout)) {
		nih_list_add (nih_timers, &timer->entry);
	} else {
		nih_list_remove (&timer->entry);
	}
}

/**
 * nih_dbus_timer:
 * @timeout: D-Bus timeout event occurred for,
 * @timer: timer that triggered the call.
 *
 * Called because @timer has elapsed and we need to pass that onto the
 * underlying @timeout.
 **/
static void
nih_dbus_timer (DBusTimeout *timeout,
		NihTimer *   timer)
{
	nih_assert (timeout != NULL);
	nih_assert (timer != NULL);

	dbus_timeout_handle (timeout);
}


/**
 * nih_dbus_wakeup_main:
 * @data: not used.
 *
 * Called by D-Bus to wakeup the main loop.
 **/
static void
nih_dbus_wakeup_main (void *data)
{
	nih_main_loop_interrupt ();
}

/**
 * nih_dbus_callback:
 * @connection: D-Bus connection,
 * @loop: loop callback structure.
 *
 * Called on each iteration of our main loop to dispatch any remaining items
 * of data from the given D-Bus connection @conn so that messages will be
 * handled automatically.
 **/
static void
nih_dbus_callback (DBusConnection * connection,
		   NihMainLoopFunc *loop)
{
	nih_assert (connection != NULL);
	nih_assert (loop != NULL);

	while (dbus_connection_dispatch (connection) == DBUS_DISPATCH_DATA_REMAINS)
		;
}


/**
 * nih_dbus_connection_disconnected:
 * @connection: D-Bus connection,
 * @message: D-Bus message received,
 * @handler: Disconnection handler.
 *
 * Called as a filter function to determine whether @connection has been
 * disconnected, and if so, call the user disconnect @handler function.
 *
 * Once the handler has been called, the connection will be automatically
 * unreferenced.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_connection_disconnected (DBusConnection *         connection,
				  DBusMessage *            message,
				  NihDBusDisconnectHandler handler)
{
	nih_assert (connection != NULL);
	nih_assert (message != NULL);

	if (! dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL,
				      "Disconnected"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (! dbus_message_has_path (message, DBUS_PATH_LOCAL))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	/* Ok, it's really the disconnected signal, call the handler. */
	nih_error_push_context ();
	if (handler)
		handler (connection);
	nih_error_pop_context ();

	dbus_connection_unref (connection);

	/* Lie.  We want other filter functions for this to be called so
	 * we unreference for each copy we hold.
	 */
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/**
 * nih_dbus_new_connection:
 * @server: D-Bus server,
 * @connection: new D-Bus connection,
 * @data: not used.
 *
 * Called by D-Bus because a new connection @connection has been made to
 * @server; we call the connect handler if set, and if that returns TRUE
 * (or not set), we reference the connection so it is not dropped and set
 * it up with our main loop.
 **/
static void
nih_dbus_new_connection (DBusServer *    server,
			 DBusConnection *connection,
			 void *          data)
{
	NihDBusConnectHandler    connect_handler;
	NihDBusDisconnectHandler disconnect_handler;

	nih_assert (server != NULL);
	nih_assert (connection != NULL);

	/* Call the connect handler if set, if it returns FALSE, drop the
	 * connection.
	 */
	connect_handler = dbus_server_get_data (server, connect_handler_slot);
	if (connect_handler) {
		int ret;

		nih_error_push_context ();
		ret = connect_handler (server, connection);
		nih_error_pop_context ();

		if (! ret)
			return;
	}

	/* We're keeping the connection, reference it and hook it up to the
	 * main loop.
	 */
	dbus_connection_ref (connection);
	disconnect_handler = dbus_server_get_data (server,
						   disconnect_handler_slot);
	NIH_ZERO (nih_dbus_setup (connection, disconnect_handler));
}
