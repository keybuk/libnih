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
	TEST_EQ_STR (((NihDBusError *)err)->name,
		     "org.freedesktop.DBus.Error.NoServer");
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
	conn = nih_dbus_bus (DBUS_BUS_SESSION, my_disconnect_handler);

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

	conn = nih_dbus_bus (DBUS_BUS_SESSION, my_disconnect_handler);

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
		TEST_EQ_STR (((NihDBusError *)err)->name,
			     "org.freedesktop.DBus.Error.NoReply");
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
	TEST_EQ_STR (((NihDBusError *)err)->name,
		     "org.freedesktop.DBus.Error.NoServer");
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


int
main (int   argc,
      char *argv[])
{
	test_error_raise ();

	test_connect ();
	test_bus ();
	test_setup ();
	test_server ();

	return 0;
}
