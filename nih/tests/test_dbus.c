/* libnih
 *
 * test_dbus.c - test suite for nih/dbus.c
 *
 * Copyright © 2008 Scott James Remnant <scott@netsplit.com>.
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

#include <nih/test.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <dbus/dbus.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/timer.h>
#include <nih/io.h>
#include <nih/main.h>
#include <nih/error.h>
#include <nih/errors.h>

#include <nih/dbus.h>


void
test_error_raise (void)
{
	NihError     *error;
	NihDBusError *err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise");
	TEST_ALLOC_SAFE {
		nih_dbus_error_raise ("foo", "bar");
		error = nih_error_get ();

		TEST_ALLOC_PARENT (error, NULL);
		TEST_ALLOC_SIZE (error, sizeof (NihDBusError));
		TEST_EQ (error->number, NIH_DBUS_ERROR);

		err = (NihDBusError *)error;
		TEST_EQ_STR (err->name, "foo");
		TEST_ALLOC_PARENT (err->name, err);
		TEST_EQ_STR (err->error.message, "bar");
		TEST_ALLOC_PARENT (err->error.message, err);

		nih_free (error);
	}
}

void
test_error_raise_printf (void)
{
	NihError     *error;
	NihDBusError *err;

	/* Make sure that an NIH_DBUS_ERROR is raised with the name and
	 * formatted message we give.
	 */
	TEST_FUNCTION ("nih_dbus_error_raise_printf");
	TEST_ALLOC_SAFE {
		nih_dbus_error_raise_printf ("foo", "hello %d this is a %s",
					     123, "test");
		error = nih_error_get ();

		TEST_ALLOC_PARENT (error, NULL);
		TEST_ALLOC_SIZE (error, sizeof (NihDBusError));
		TEST_EQ (error->number, NIH_DBUS_ERROR);

		err = (NihDBusError *)error;
		TEST_EQ_STR (err->name, "foo");
		TEST_ALLOC_PARENT (err->name, err);
		TEST_EQ_STR (err->error.message, "hello 123 this is a test");
		TEST_ALLOC_PARENT (err->error.message, err);

		nih_free (error);
	}
}


static int connected = FALSE;
static DBusConnection *last_connection = NULL;
static int drop_connection = FALSE;

static void
my_new_connection (DBusServer     *server,
		   DBusConnection *connection,
		   void           *data)
{
	connected = TRUE;

	if (! drop_connection) {
		dbus_connection_ref (connection);
		last_connection = connection;
	}

	nih_main_loop_exit (0);
}

static int disconnected = FALSE;

static void
my_disconnect_handler (DBusConnection *connection)
{
	disconnected = TRUE;
	last_connection = connection;

	nih_main_loop_exit (0);
}

void
test_connect (void)
{
	DBusServer      *server;
	DBusConnection  *conn, *server_conn, *last_conn;
	NihIoWatch      *io_watch;
	NihMainLoopFunc *loop_func;
	NihError        *err;
	int              fd;

	TEST_FUNCTION ("nih_dbus_connect");
	nih_timer_init ();
	nih_io_init ();
	nih_main_loop_init ();

	server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
				  NULL, NULL);
	assert (server != NULL);

	dbus_server_set_new_connection_function (server, my_new_connection,
						 NULL, NULL);


	/* Check that we can create a new connection to a listening dbus
	 * server, the returned object should be hooked up to the main loop
	 * and the server should receive the connection.
	 */
	TEST_FEATURE ("with listening server");
	conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
				 my_disconnect_handler);

 	TEST_NE_P (conn, NULL);

	connected = FALSE;
	last_connection = NULL;
	drop_connection = FALSE;

	nih_main_loop ();

	TEST_TRUE (dbus_connection_get_is_connected (conn));

	TEST_TRUE (connected);
	TEST_NE_P (last_connection, NULL);
	server_conn = last_connection;

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	/* Step over the server io_watch to find the connection one */
	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_NE_P (io_watch->entry.next, nih_io_watches);
	io_watch = (NihIoWatch *)io_watch->entry.next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be a single main loop function. */
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

	TEST_EQ_P (loop_func->data, conn);


	/* Check that if the server disconnects, our disconnect handler is
	 * called and the connection is automatically unreferenced, freeing
	 * the loop function.
	 */
	TEST_FEATURE ("with disconnection from server");
	disconnected = FALSE;
	last_connection = NULL;

	TEST_FREE_TAG (loop_func);

	dbus_connection_close (server_conn);
	dbus_connection_unref (server_conn);

	nih_main_loop ();

	TEST_TRUE (disconnected);
	TEST_EQ_P (last_connection, conn);

	TEST_FREE (loop_func);


	/* Check that by using a GUID we can reuse connections to the same
	 * server, the second call to connect just returns the same
	 * connection as the first.
	 */
	TEST_FEATURE ("with multiple shared connections");
	conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus,guid=deadbeef",
				 my_disconnect_handler);

	TEST_NE_P (conn, NULL);

	connected = FALSE;
	last_connection = NULL;
	drop_connection = FALSE;

	nih_main_loop ();

	TEST_TRUE (dbus_connection_get_is_connected (conn));

	TEST_TRUE (connected);
	TEST_NE_P (last_connection, NULL);
	server_conn = last_connection;

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	/* Step over the server io_watch to find the connection one */
	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_NE_P (io_watch->entry.next, nih_io_watches);
	io_watch = (NihIoWatch *)io_watch->entry.next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be a single main loop function. */
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

	TEST_EQ_P (loop_func->data, conn);

	last_conn = conn;

	TEST_FREE_TAG (loop_func);

	conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus,guid=deadbeef",
				 my_disconnect_handler);

	TEST_EQ_P (conn, last_conn);

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	/* Still should be just one IoWatch after the server one */
	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_NE_P (io_watch->entry.next, nih_io_watches);
	io_watch = (NihIoWatch *)io_watch->entry.next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should not be a new main loop function. */
	TEST_NOT_FREE (loop_func);
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	TEST_EQ_P (nih_main_loop_functions->next, &loop_func->entry);
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);
	TEST_EQ_P (loop_func->data, conn);

	/* Disconnection should free both references */
	disconnected = FALSE;
	last_connection = NULL;

	dbus_connection_close (server_conn);
	dbus_connection_unref (server_conn);

	nih_main_loop ();

	TEST_TRUE (disconnected);
	TEST_EQ_P (last_connection, conn);

	TEST_FREE (loop_func);


	/* Check that we can create a new connection to a listening dbus
	 * server, it should return a hooked up object but if the server
	 * immediately drops it, should get disconnected.
	 */
	TEST_FEATURE ("with server that drops our connection");
	conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
				 my_disconnect_handler);

	TEST_NE_P (conn, NULL);

	connected = FALSE;
	last_connection = NULL;
	drop_connection = TRUE;

	disconnected = FALSE;
	last_connection = NULL;

	nih_main_loop ();

	TEST_TRUE (connected);
	TEST_NE_P (last_connection, NULL);
	server_conn = last_connection;

	if (! disconnected)
		nih_main_loop ();

	TEST_TRUE (disconnected);
	TEST_EQ_P (last_connection, conn);


	/* Check that if we create a new connection to a non-listening
	 * address, no object is returned.
	 */
	TEST_FEATURE ("with non-listening server");
	conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_foo",
				 NULL);

	TEST_EQ_P (conn, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
	TEST_EQ_STR (((NihDBusError *)err)->name, DBUS_ERROR_NO_SERVER);
	nih_free (err);

	dbus_server_disconnect (server);
	dbus_server_unref (server);

	dbus_shutdown ();
}

void
test_bus (void)
{
	DBusServer      *server;
	DBusConnection  *conn, *last_conn;
	NihIoWatch      *io_watch;
	NihMainLoopFunc *loop_func;
	NihError        *err;
	pid_t            pid1, pid2;
	int              fd, wait_fd, status;

	TEST_FUNCTION ("nih_dbus_bus");


	/* Check that we can create a connection to the D-Bus session bus,
	 * the returned object should be hooked up to the main loop.
	 */
	TEST_FEATURE ("with session bus");
	conn = nih_dbus_bus (DBUS_BUS_SESSION, my_disconnect_handler);
	if (! conn) {
		NihError *err;

		err = nih_error_get ();
		nih_free (err);

		printf ("SKIP: session bus not available\n");
		goto system_bus;
	}

	TEST_NE_P (conn, NULL);

	TEST_TRUE (dbus_connection_get_is_connected (conn));

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be a single main loop function. */
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

	TEST_EQ_P (loop_func->data, conn);

	dbus_connection_unref (conn);
system_bus:
	dbus_shutdown ();


	/* Check that we can create a connection to the D-Bus system bus,
	 * the returned object should be hooked up to the main loop.
	 */
	TEST_FEATURE ("with system bus");
	conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

	TEST_NE_P (conn, NULL);

	TEST_TRUE (dbus_connection_get_is_connected (conn));

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be a single main loop function. */
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

	TEST_EQ_P (loop_func->data, conn);

	dbus_connection_unref (conn);
	dbus_shutdown ();


	/* Check that we can share connections to a bus. */
	TEST_FEATURE ("with shared bus connection");
	conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

	TEST_NE_P (conn, NULL);

	TEST_TRUE (dbus_connection_get_is_connected (conn));

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be a single main loop function. */
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);
	TEST_EQ_P (loop_func->data, conn);

	last_conn = conn;
	TEST_FREE_TAG (loop_func);

	conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

	TEST_EQ_P (conn, last_conn);

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be the same main loop function. */
	TEST_NOT_FREE (loop_func);
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	TEST_EQ_P (nih_main_loop_functions->next, &loop_func->entry);
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);
	TEST_EQ_P (loop_func->data, conn);

	dbus_connection_unref (conn);
	dbus_connection_unref (last_conn);
	dbus_shutdown ();


	/* Check that if the bus disconnects before registration, NULL
	 * is returned along with an error.  Stock dbus tends to bail out
	 * with an exit code, so we watch very carefully for that ;-)
	 */
	TEST_FEATURE ("with disconnection before registration");
	TEST_CHILD (pid1) {
		TEST_CHILD_WAIT (pid2, wait_fd) {
			server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
						  NULL, NULL);
			assert (server != NULL);

			dbus_server_set_new_connection_function (server,
								 my_new_connection,
								 NULL, NULL);

			connected = FALSE;
			last_connection = NULL;
			drop_connection = TRUE;

			TEST_CHILD_RELEASE (wait_fd);

			nih_main_loop ();

			dbus_server_disconnect (server);
			dbus_server_unref (server);

			dbus_shutdown ();

			exit (0);
		}

		setenv ("DBUS_SYSTEM_BUS_ADDRESS",
			"unix:abstract=/com/netsplit/nih/test_dbus", TRUE);

		conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

		TEST_EQ_P (conn, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		TEST_EQ_STR (((NihDBusError *)err)->name, DBUS_ERROR_NO_REPLY);
		nih_free (err);

		dbus_shutdown ();

		unsetenv ("DBUS_SYSTEM_BUS_ADDRESS");

		waitpid (pid2, NULL, 0);

		exit (123);
	}

	assert (waitpid (pid1, &status, 0) == pid1);
	if ((! WIFEXITED (status)) || (WEXITSTATUS (status) != 123))
		TEST_FAILED ("unexpected exit(), unpatched D-Bus?");


	/* Check that if the bus is not available, NULL is returned and
	 * an error.
	 */
	TEST_FEATURE ("with no bus");
	setenv ("DBUS_SYSTEM_BUS_ADDRESS",
		"unix:abstract=/com/netsplit/nih/test_foo", TRUE);

	conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

	TEST_EQ_P (conn, NULL);

	err = nih_error_get ();
	TEST_EQ (err->number, NIH_DBUS_ERROR);
	TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
	TEST_EQ_STR (((NihDBusError *)err)->name, DBUS_ERROR_NO_SERVER);
	nih_free (err);

	dbus_shutdown ();

	unsetenv ("DBUS_SYSTEM_BUS_ADDRESS");
}


void
test_setup (void)
{
	DBusConnection  *conn;
	NihIoWatch      *io_watch;
	NihMainLoopFunc *loop_func;
	int              ret, fd;

	TEST_FUNCTION ("nih_dbus_setup");

	/* Check that we can setup a new connection for use with the
	 * nih main loop.
	 */
	TEST_FEATURE ("with new connection");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	ret = nih_dbus_setup (conn, NULL);

	TEST_EQ (ret, 0);

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be a single main loop function. */
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

	TEST_EQ_P (loop_func->data, conn);


	/* Check that if we try and set the same connection up again,
	 * nothing changes.
	 */
	TEST_FEATURE ("with existing connection");
	TEST_FREE_TAG (loop_func);

	ret = nih_dbus_setup (conn, NULL);

	TEST_EQ (ret, 0);

	TEST_LIST_NOT_EMPTY (nih_io_watches);

	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);

	dbus_connection_get_unix_fd (conn, &fd);
	TEST_EQ (io_watch->fd, fd);
	TEST_NE_P (io_watch->data, NULL);

	/* Should be the same main loop function. */
	TEST_NOT_FREE (loop_func);
	TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
	TEST_EQ_P (nih_main_loop_functions->next, &loop_func->entry);
	TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);
	TEST_EQ_P (loop_func->data, conn);

	dbus_connection_unref (conn);
	dbus_shutdown ();
}


static int
my_connect_handler (DBusServer     *server,
		    DBusConnection *connection)
{
	connected = TRUE;

	if (! drop_connection)
		last_connection = connection;

	nih_main_loop_exit (0);

	return drop_connection ? FALSE : TRUE;
}

void
test_server (void)
{
	DBusServer     *server;
	DBusConnection *conn, *server_conn;
	NihIoWatch     *io_watch;

	TEST_FUNCTION ("nih_dbus_server");

	/* Check that we can create a new D-Bus server instance and that
	 * it is hooked up to the main loop with an IoWatch.
	 */
	TEST_FEATURE ("with new server");
	server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
				  my_connect_handler,
				  my_disconnect_handler);

	TEST_NE_P (server, NULL);

	TEST_LIST_NOT_EMPTY (nih_io_watches);
	io_watch = (NihIoWatch *)nih_io_watches->next;
	TEST_EQ_P (io_watch->entry.next, nih_io_watches);


	/* Check that a connection to that server results in the connect
	 * handler being called, and that if that returns TRUE, the
	 * connection remains open.
	 */
	TEST_FEATURE ("with connection to server");
	conn = dbus_connection_open_private ("unix:abstract=/com/netsplit/nih/test_dbus", NULL);

 	TEST_NE_P (conn, NULL);

	connected = FALSE;
	last_connection = NULL;
	drop_connection = FALSE;

	nih_main_loop ();

	TEST_TRUE (dbus_connection_get_is_connected (conn));

	TEST_TRUE (connected);
	TEST_NE_P (last_connection, NULL);
	server_conn = last_connection;


	/* Check that if the client disconnects, the server connection
	 * disconnect handler is called and unreferenced.
	 */
	TEST_FEATURE ("with disconnect by client");
	disconnected = FALSE;
	last_connection = NULL;

	dbus_connection_close (conn);
	dbus_connection_unref (conn);

	nih_main_loop ();

	TEST_TRUE (disconnected);
	TEST_EQ_P (last_connection, server_conn);


	/* Check that if the connect handler returns FALSE, the connection
	 * is abandoned and the client disconnected.
	 */
	TEST_FEATURE ("with decline by connect handler");
	conn = dbus_connection_open_private ("unix:abstract=/com/netsplit/nih/test_dbus", NULL);

 	TEST_NE_P (conn, NULL);

	connected = FALSE;
	last_connection = NULL;
	drop_connection = TRUE;

	nih_main_loop ();

	TEST_TRUE (connected);

	while (dbus_connection_read_write_dispatch (conn, -1))
		;

	TEST_FALSE (dbus_connection_get_is_connected (conn));

	dbus_connection_unref (conn);


	dbus_server_disconnect (server);
	dbus_server_unref (server);

	dbus_shutdown ();
}


static int foo_called = FALSE;
static NihDBusObject *last_object = NULL;
static NihDBusMessage *last_message = NULL;
static DBusConnection *last_message_conn = NULL;

static DBusHandlerResult
foo_marshal (NihDBusObject  *object,
	     NihDBusMessage *message)
{
	foo_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->conn;

	TEST_FREE_TAG (message);

	nih_main_loop_exit (0);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static int bar_called = FALSE;

static DBusHandlerResult
bar_marshal (NihDBusObject  *object,
	     NihDBusMessage *message)
{
	bar_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->conn;

	TEST_FREE_TAG (message);

	nih_main_loop_exit (0);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static const NihDBusArg foo_args[] = {
	{ "str", "s", NIH_DBUS_ARG_IN },
	{ "len", "u", NIH_DBUS_ARG_IN },
	{ "count", "u", NIH_DBUS_ARG_OUT },
	{ NULL }
};

static const NihDBusArg bar_args[] = {
	{ "wibble", "d", NIH_DBUS_ARG_IN },
	{ NULL }
};

static const NihDBusArg baz_args[] = {
	{ NULL }
};

static const NihDBusArg signal_args[] = {
	{ "msg", "s", NIH_DBUS_ARG_IN },
	{ NULL }
};

static const NihDBusMethod interface_a_methods[] = {
	{ "Foo", foo_marshal, foo_args },
	{ "Bar", bar_marshal, bar_args },
	{ NULL }
};

static const NihDBusSignal interface_a_signals[] = {
	{ "Alert", signal_args },
	{ "Panic", signal_args },
	{ NULL }
};

static const NihDBusMethod interface_b_methods[] = {
	{ "Bar", foo_marshal, bar_args },
	{ "Baz", foo_marshal, baz_args },
	{ NULL }
};

static const NihDBusProperty interface_b_props[] = {
	{ "Colour", "s", NIH_DBUS_READWRITE },
	{ "Size",   "u", NIH_DBUS_READ },
	{ "Poke",   "d", NIH_DBUS_WRITE },
	{ NULL }
};

static const NihDBusInterface interface_a = {
	"Nih.TestA",
	interface_a_methods,
	interface_a_signals,
	NULL
};

static const NihDBusInterface interface_b = {
	"Nih.TestB",
	interface_b_methods,
	NULL,
	interface_b_props
};

static const NihDBusInterface *no_interfaces[] = {
	NULL
};

static const NihDBusInterface *one_interface[] = {
	&interface_a,
	NULL
};

static const NihDBusInterface *both_interfaces[] = {
	&interface_a,
	&interface_b,
	NULL
};

void
test_object_new (void)
{
	DBusConnection *conn;
	NihDBusObject  *object;

	/* Check that we can register a new object, having the filled in
	 * structure returned for us with the object registered against
	 * the connection at the right path.
	 */
	TEST_FUNCTION ("nih_dbus_object_new");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	assert (conn != NULL);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	TEST_ALLOC_FAIL {
		void *data;

		object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
					      both_interfaces, &object);

		if (test_alloc_failed) {
			TEST_EQ_P (object, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (object, sizeof (NihDBusObject));

		TEST_ALLOC_PARENT (object->path, object);
		TEST_EQ_STR (object->path, "/com/netsplit/Nih");

		TEST_EQ_P (object->conn, conn);
		TEST_EQ_P (object->data, &object);
		TEST_EQ_P (object->interfaces, both_interfaces);
		TEST_EQ (object->registered, TRUE);

		TEST_TRUE (dbus_connection_get_object_path_data (
				   conn, "/com/netsplit/Nih", &data));
		TEST_EQ_P (data, object);

		nih_free (object);
	}

	dbus_connection_unref (conn);

	dbus_shutdown ();
}

void
test_object_destroy (void)
{
	DBusConnection *conn;
	NihDBusObject  *object;
	void           *data;

	/* Check that a registered D-Bus object is unregistered from the
	 * bus when it is destroyed.
	 */
	TEST_FUNCTION ("nih_dbus_object_destroy");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	assert (conn != NULL);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      both_interfaces, &object);
	assert (object != NULL);
	assert (dbus_connection_get_object_path_data (
			conn, "/com/netsplit/Nih", &data));
	assert (data == object);

	nih_free (object);

	TEST_TRUE (dbus_connection_get_object_path_data (
			   conn, "/com/netsplit/Nih", &data));
	TEST_EQ_P (data, NULL);

	dbus_connection_unref (conn);

	dbus_shutdown ();
}

void
test_object_unregister (void)
{
	DBusConnection *conn;
	NihDBusObject  *object;

	/* Check that when a D-Bus connection is destroyed, any registered
	 * D-Bus objects go as well.
	 */
	TEST_FUNCTION ("nih_dbus_object_unregister");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	assert (conn != NULL);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      both_interfaces, &object);
	assert (object != NULL);

	TEST_FREE_TAG (object);

	dbus_connection_unref (conn);

	dbus_shutdown ();

	TEST_FREE (object);
}


static void
pending_call_complete (DBusPendingCall *pending,
		       void            *data)
{
	nih_main_loop_exit (0);
}

void
test_object_message (void)
{
	DBusServer      *server;
	DBusConnection  *conn, *server_conn;
	NihDBusObject   *object, *child1, *child2;
	DBusMessage     *message;
	DBusPendingCall *pending;
	const char      *xml;

	TEST_FUNCTION ("nih_dbus_object_message");
	nih_timer_init ();

	server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
				  &my_connect_handler, NULL);

	connected = FALSE;
	last_connection = NULL;
	drop_connection = FALSE;

	conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
				 NULL);
	assert (conn != NULL);

	nih_main_loop ();

	assert (connected);
	assert (last_connection != NULL);

	server_conn = last_connection;


	/* Check that the handler for a known method is called with the
	 * object passed in along with a message structure containing
	 * both the message and connection (which will be freed before
	 * returning.
	 */
	TEST_FEATURE ("with registered method");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      one_interface, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			NULL, "/com/netsplit/Nih", "Nih.TestA", "Foo");
		assert (message != NULL);

		assert (dbus_connection_send (conn, message, NULL));

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_TRUE (foo_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);
	}

	nih_free (object);


	/* Check that the first of two handlers for a method without a
	 * specified interface is called.
	 */
	TEST_FEATURE ("with method registered to multiple interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      both_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			NULL, "/com/netsplit/Nih", NULL, "Bar");
		assert (message != NULL);

		assert (dbus_connection_send (conn, message, NULL));

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_FALSE (foo_called);
		TEST_TRUE (bar_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);
	}

	nih_free (object);


	/* Check that an unknown method on a known interface results in
	 * an error being returned to the caller.
	 */
	TEST_FEATURE ("with unknown method on known interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      both_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
							"Nih.TestB", "Wibble");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_is_error (message, DBUS_ERROR_UNKNOWN_METHOD));

		dbus_message_unref (message);
		dbus_pending_call_unref (pending);
	}

	nih_free (object);


	/* Check that an unknown method on an unknown interface results in
	 * an error being returned to the caller.
	 */
	TEST_FEATURE ("with unknown method on unknown interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      both_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
							"Nih.TestC", "Wibble");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_is_error (message, DBUS_ERROR_UNKNOWN_METHOD));

		dbus_message_unref (message);
		dbus_pending_call_unref (pending);
	}

	nih_free (object);


	/* Check that an unknown method with no specified interface results in
	 * an error being returned to the caller.
	 */
	TEST_FEATURE ("with unknown method with no interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      both_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
							NULL, "Wibble");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_is_error (message, DBUS_ERROR_UNKNOWN_METHOD));

		dbus_message_unref (message);
		dbus_pending_call_unref (pending);
	}

	nih_free (object);


	/* Check that a method call when no interfaces are specified results
	 * in an error being returned to the caller.
	 */
	TEST_FEATURE ("with method call and no interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
							"Nih.TestA", "Foo");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_is_error (message, DBUS_ERROR_UNKNOWN_METHOD));

		dbus_message_unref (message);
		dbus_pending_call_unref (pending);
	}

	nih_free (object);


	/* Check that the Introspect message is handled internally with
	 * an accurate portrayal of the interfaces and their properties
	 * returned.
	 */
	TEST_FEATURE ("with introspect method");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      both_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			NULL, "/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE, "Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_has_signature (message, "s"));

		TEST_TRUE (dbus_message_get_args (message, NULL,
						  DBUS_TYPE_STRING, &xml,
						  DBUS_TYPE_INVALID));

		TEST_EQ_STRN (xml, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
		xml += strlen (DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);

		TEST_EQ_STRN (xml, "<node name=\"/com/netsplit/Nih\">\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <interface name=\""
			      DBUS_INTERFACE_INTROSPECTABLE "\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Introspect\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  </interface>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <interface name=\"Nih.TestA\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Foo\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"str\" type=\"s\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"len\" type=\"u\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"count\" type=\"u\" direction=\"out\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Bar\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"wibble\" type=\"d\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <signal name=\"Alert\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"msg\" type=\"s\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </signal>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <signal name=\"Panic\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"msg\" type=\"s\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </signal>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  </interface>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <interface name=\"Nih.TestB\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Bar\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"wibble\" type=\"d\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Baz\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <property name=\"Colour\" type=\"s\" access=\"readwrite\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <property name=\"Size\" type=\"u\" access=\"read\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <property name=\"Poke\" type=\"d\" access=\"write\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  </interface>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <interface name=\""
			      DBUS_INTERFACE_PROPERTIES "\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Get\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Set\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"value\" type=\"v\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"GetAll\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"props\" type=\"a{sv}\" direction=\"out\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  </interface>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "</node>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STR (xml, "");

		dbus_message_unref (message);
		dbus_pending_call_cancel (pending);
		dbus_pending_call_unref (pending);
	}

	nih_free (object);


	/* Check that the Introspect message works when there are no
	 * interfaces.
	 */
	TEST_FEATURE ("with introspect method and no interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			NULL, "/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE, "Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_has_signature (message, "s"));

		TEST_TRUE (dbus_message_get_args (message, NULL,
						  DBUS_TYPE_STRING, &xml,
						  DBUS_TYPE_INVALID));

		TEST_EQ_STRN (xml, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
		xml += strlen (DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);

		TEST_EQ_STRN (xml, "<node name=\"/com/netsplit/Nih\">\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Introspect\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  </interface>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "</node>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STR (xml, "");

		dbus_message_unref (message);
		dbus_pending_call_cancel (pending);
		dbus_pending_call_unref (pending);
	}

	nih_free (object);


	/* Check that the Introspect message contains node entries for
	 * children, but doesn't bother to flesh them out.
	 */
	TEST_FEATURE ("with introspect method and children");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);
	child1 = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih/Frodo",
				      one_interface, &server_conn);
	child2 = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih/Bilbo",
				      both_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			NULL, "/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE, "Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send_with_reply (
					conn, message,
					&pending, -1));
			assert (dbus_pending_call_set_notify (
					pending, pending_call_complete,
					NULL, NULL));
		}

		dbus_message_unref (message);

		nih_main_loop ();

		TEST_TRUE (dbus_pending_call_get_completed (pending));

		message = dbus_pending_call_steal_reply (pending);
		TEST_NE_P (message, NULL);

		TEST_TRUE (dbus_message_has_signature (message, "s"));

		TEST_TRUE (dbus_message_get_args (message, NULL,
						  DBUS_TYPE_STRING, &xml,
						  DBUS_TYPE_INVALID));

		TEST_EQ_STRN (xml, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
		xml += strlen (DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);

		TEST_EQ_STRN (xml, "<node name=\"/com/netsplit/Nih\">\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <method name=\"Introspect\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    </method>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  </interface>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "  <node name=\"Bilbo\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "  <node name=\"Frodo\"/>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STRN (xml, "</node>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STR (xml, "");

		dbus_message_unref (message);
		dbus_pending_call_cancel (pending);
		dbus_pending_call_unref (pending);
	}

	nih_free (child2);
	nih_free (child1);
	nih_free (object);


	dbus_connection_close (server_conn);
	dbus_connection_unref (server_conn);

	dbus_connection_unref (conn);

	dbus_server_disconnect (server);
	dbus_server_unref (server);

	dbus_shutdown ();
}


static void
my_return_error_cb (NihDBusMessage *message,
		    NihTimer       *timer)
{
	int ret;

	TEST_NE_P (message, NULL);
	TEST_NOT_FREE (message);

	ret = nih_dbus_message_error (message,
				      "com.netsplit.Nih.Test.MyError",
				      "this is a %s %d", "test", 1234);

	TEST_EQ (ret, 0);

	TEST_FREE (message);
}

static DBusHandlerResult
my_return_error (NihDBusObject  *object,
		 NihDBusMessage *message)
{
	NIH_MUST (nih_timer_add_timeout (NULL, 1,
					 (NihTimerCb)my_return_error_cb,
					 message));

	TEST_FREE_TAG (message);

	/* async */
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusConnection *server_conn = NULL;

static int
my_error_connect (DBusServer     *server,
		  DBusConnection *conn)
{
	NihDBusObject *object;

	const static NihDBusMethod com_netsplit_Nih_Glue_methods[] = {
		{ "ReturnError", my_return_error, NULL },
		{ NULL }
	};
	const static NihDBusInterface com_netsplit_Nih_Glue = {
		"com.netsplit.Nih.Glue",
		com_netsplit_Nih_Glue_methods, NULL, NULL
	};
	const static NihDBusInterface *my_interfaces[] = {
		&com_netsplit_Nih_Glue,
		NULL,
	};

	assert (server_conn == NULL);
	server_conn = conn;

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      my_interfaces, NULL);
	assert (object != NULL);

	return TRUE;
}

void
test_message_error (void)
{
	DBusConnection *conn;
	DBusMessage    *message, *reply;
	DBusError       error;
	pid_t           server_pid;
	int             wait_fd = -1, status;

	/* Check that an error returned outside the handler with the
	 * nih_dbus_message_error() function is returned to the sender
	 * with the right details.
	 */
	TEST_FUNCTION ("nih_dbus_message_error");
	TEST_CHILD_WAIT (server_pid, wait_fd) {
		DBusServer *server;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test",
					  my_error_connect, NULL);
		assert (server != NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		if (server_conn) {
			dbus_connection_close (server_conn);
			dbus_connection_unref (server_conn);
		}

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();

		exit (0);
	}


	dbus_error_init (&error);

	conn = dbus_connection_open ("unix:abstract=/com/netsplit/nih/test",
				     NULL);
	assert (conn != NULL);


	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"ReturnError");

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, "com.netsplit.Nih.Test.MyError");
	TEST_EQ_STR (error.message, "this is a test 1234");

	dbus_error_free (&error);

	dbus_message_unref (message);


	kill (server_pid, SIGTERM);

	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	dbus_connection_unref (conn);

	dbus_shutdown ();
}


void
test_proxy_new (void)
{
	DBusConnection *conn;
	NihDBusProxy   *proxy;

	/* Check that we can create a proxy for a remote object with all of
	 * the right details filled in.
	 */
	TEST_FUNCTION ("nih_dbus_proxy_new");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	assert (conn != NULL);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	TEST_ALLOC_FAIL {
		proxy = nih_dbus_proxy_new (NULL, conn, "com.netsplit.Nih",
					    "/com/netsplit/Nih");

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->conn, conn);

		nih_free (proxy);
	}

	dbus_connection_unref (conn);

	dbus_shutdown ();
}


void
test_path (void)
{
	char *path;

	TEST_FUNCTION ("nih_dbus_path");

	/* Check that a root path with no additional elements is simply
	 * returned duplicated, the root should not be escaped.
	 */
	TEST_FEATURE ("with root only");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih");

		nih_free (path);
	}


	/* Check that a root path with a single additional element has that
	 * appended separated by a slash.
	 */
	TEST_FEATURE ("with single additional element");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih", "test", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih/test");

		nih_free (path);
	}


	/* Check that a root path with multiple additional elements have them
	 * appended separated by slashes.
	 */
	TEST_FEATURE ("with multiple additional elements");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih",
				      "test", "frodo", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih/test/frodo");

		nih_free (path);
	}


	/* Check that if one of the additional elements requires escaping,
	 * it is appended in the escaped form.
	 */
	TEST_FEATURE ("with element requiring escaping");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih",
				      "test", "foo/bar.baz", "frodo", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, ("/com/netsplit/Nih"
				    "/test/foo_2fbar_2ebaz/frodo"));

		nih_free (path);
	}


	/* Check that when multiple elements require escaping, they are
	 * all escaped; also check that an underscore requires escaping to
	 * ensure path uniqueness.
	 */
	TEST_FEATURE ("with multiple elements requiring escaping");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih",
				      "test_thing", "foo/bar.baz",
				      "frodo", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, ("/com/netsplit/Nih"
				    "/test_5fthing/foo_2fbar_2ebaz/frodo"));

		nih_free (path);
	}


	/* Check that if one of the additional elements is empty, it
	 * is replaced with an underscore.
	 */
	TEST_FEATURE ("with empty element");
	TEST_ALLOC_FAIL {
		path = nih_dbus_path (NULL, "/com/netsplit/Nih", "", NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (path, NULL);
			continue;
		}

		TEST_EQ_STR (path, "/com/netsplit/Nih/_");

		nih_free (path);
	}
}


int
main (int   argc,
      char *argv[])
{
	test_error_raise ();

	test_connect ();
	test_bus ();
	test_setup ();
	test_server ();
	test_object_new ();
	test_object_destroy ();
	test_object_unregister ();
	test_object_message ();
	test_message_error ();
	test_proxy_new ();
	test_path ();

	return 0;
}
