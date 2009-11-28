/* libnih
 *
 * test_dbus_connection.c - test suite for nih-dbus/dbus_connection.c
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

#include <nih/test.h>
#include <nih-dbus/test_dbus.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <dbus/dbus.h>

#include <stdlib.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/timer.h>
#include <nih/signal.h>
#include <nih/child.h>
#include <nih/io.h>
#include <nih/main.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_connection.h>
#include <nih-dbus/errors.h>


static DBusConnection *client_connection = NULL;

static void
my_new_connection (DBusServer *    server,
		   DBusConnection *connection,
		   void *          data)
{
	dbus_connection_ref (connection);

	if (client_connection) {
		dbus_connection_close (client_connection);
		dbus_connection_unref (client_connection);
	}

	client_connection = connection;
}


static int             disconnected = FALSE;
static DBusConnection *last_disconnection = NULL;

static void
my_disconnect_handler (DBusConnection *connection)
{
	disconnected = TRUE;
	last_disconnection = connection;

	nih_main_loop_exit (0);
}

static void
my_new_connection_drop (DBusServer *    server,
			DBusConnection *connection,
			void *          data)
{
}

static int my_message_received = FALSE;

static DBusHandlerResult
my_message_received_function (DBusConnection *connection,
			      DBusMessage *   message,
			      void *          data)
{
	my_message_received++;

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
my_new_connection_fake (DBusServer *    server,
			DBusConnection *connection,
			void *          data)
{
	DBusMessage *signal;

	my_new_connection (server, connection, data);

	signal = dbus_message_new_signal (DBUS_PATH_LOCAL "x",
					  DBUS_INTERFACE_LOCAL "x",
					  "Disconnected");
	dbus_connection_send (client_connection, signal, NULL);
	dbus_connection_flush (client_connection);

	dbus_message_unref (signal);
}

static DBusHandlerResult
my_message_received_break_function (DBusConnection *connection,
				    DBusMessage *   message,
				    void *          data)
{
	my_message_received++;

	nih_main_loop_exit (0);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
my_message_handle_function (DBusConnection *connection,
			    DBusMessage *   message,
			    void *          data)
{
	DBusMessage *reply;

	reply = dbus_message_new_method_return (message);
	dbus_connection_send (connection, reply, NULL);

	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static int
my_method_connect_handler (DBusServer *    server,
			   DBusConnection *connection)
{
	if (client_connection) {
		dbus_connection_close (client_connection);
		dbus_connection_unref (client_connection);
	}

	client_connection = connection;

	dbus_connection_add_filter (client_connection, my_message_handle_function,
				    NULL, NULL);

	return TRUE;
}

static void
my_notify_function (DBusPendingCall *pending_call,
		    void *           data)
{
	nih_main_loop_exit (0);
}

void
test_connect (void)
{
	pid_t            dbus_pid = 0;
	int              wait_fd;
	DBusServer *     server;
	DBusConnection * conn = NULL;
	DBusConnection * last_conn;
	NihIoWatch *     io_watch = NULL;
	NihMainLoopFunc *loop_func = NULL;
	NihError *       err;
	int              fd;
	int              status;
	DBusMessage *    method_call = NULL;
	DBusPendingCall *pending_call = NULL;
	DBusMessage *    reply;
	int              ret;

	TEST_FUNCTION ("nih_dbus_connect");


	/* Check that we can create a new connection to a listening dbus
	 * server, the returned object should be hooked up to the main loop
	 * and the server should receive the connection.
	 */
	TEST_FEATURE ("with listening server");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh = nih_signal_add_handler (NULL, SIGTERM,
								     nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  NULL, NULL);
				assert (server != NULL);

				dbus_server_set_new_connection_function (server, my_new_connection,
									 NULL, NULL);

				client_connection = NULL;

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				if (client_connection) {
					dbus_connection_close (client_connection);
					dbus_connection_unref (client_connection);
				}

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}
		}

		conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
					 NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (conn, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			kill (dbus_pid, SIGTERM);

			waitpid (dbus_pid, &status, 0);
			TEST_TRUE (WIFEXITED (status));
			TEST_EQ (WEXITSTATUS (status), 0);

			dbus_shutdown ();
			continue;
		}

		TEST_NE_P (conn, NULL);
		TEST_TRUE (dbus_connection_get_is_connected (conn));

		/* Should be a single I/O watch */
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		io_watch = (NihIoWatch *)nih_io_watches->next;
		dbus_connection_get_unix_fd (conn, &fd);
		TEST_EQ (io_watch->fd, fd);
		TEST_NE_P (io_watch->data, NULL);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Should be a single main loop function. */
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
		TEST_EQ_P (loop_func->data, conn);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		dbus_connection_unref (conn);

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		dbus_shutdown ();
	}


	/* Check that if the server disconnects, our disconnect handler is
	 * called and the connection is automatically unreferenced, freeing
	 * the loop function.  Any other filter function we've placed on
	 * the connection should also be run.
	 */
	TEST_FEATURE ("with disconnection from server");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh = nih_signal_add_handler (NULL, SIGTERM,
								     nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  NULL, NULL);
				assert (server != NULL);

				dbus_server_set_new_connection_function (server, my_new_connection,
									 NULL, NULL);

				client_connection = NULL;

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				if (client_connection) {
					dbus_connection_close (client_connection);
					dbus_connection_unref (client_connection);
				}

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}

			conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
						 my_disconnect_handler);

			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));

			assert (! NIH_LIST_EMPTY (nih_io_watches));
			io_watch = (NihIoWatch *)nih_io_watches->next;
			dbus_connection_get_unix_fd (conn, &fd);
			assert (io_watch->fd == fd);

			assert (! NIH_LIST_EMPTY (nih_main_loop_functions));
			loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
			assert (loop_func->data == conn);
		}

		disconnected = FALSE;
		last_disconnection = NULL;

		my_message_received = FALSE;
		dbus_connection_add_filter (conn, my_message_received_function,
					    NULL, NULL);

		TEST_FREE_TAG (io_watch);
		TEST_FREE_TAG (loop_func);

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		nih_main_loop ();

		TEST_TRUE (disconnected);
		TEST_EQ_P (last_disconnection, conn);
		TEST_TRUE (my_message_received);

		TEST_FREE (io_watch);
		TEST_FREE (loop_func);

		dbus_shutdown ();
	}


	/* Check that a fake Disconnected signal does not trigger automatic
	 * disconnection but does call our other filter function.  We can
	 * only test a wrong path and interface because otherwise D-Bus
	 * disconnects us for sending naughty data.
	 */
	TEST_FEATURE ("with disconnection signal from wrong path");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh = nih_signal_add_handler (NULL, SIGTERM,
								     nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  NULL, NULL);
				assert (server != NULL);

				dbus_server_set_new_connection_function (server, my_new_connection_fake,
									 NULL, NULL);

				client_connection = NULL;

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				if (client_connection) {
					dbus_connection_close (client_connection);
					dbus_connection_unref (client_connection);
				}

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}

			conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
						 my_disconnect_handler);

			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));

			assert (! NIH_LIST_EMPTY (nih_io_watches));
			io_watch = (NihIoWatch *)nih_io_watches->next;
			dbus_connection_get_unix_fd (conn, &fd);
			assert (io_watch->fd == fd);

			assert (! NIH_LIST_EMPTY (nih_main_loop_functions));
			loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
			assert (loop_func->data == conn);
		}

		disconnected = FALSE;
		last_disconnection = NULL;

		my_message_received = FALSE;
		dbus_connection_add_filter (conn, my_message_received_break_function,
					    NULL, NULL);

		TEST_FREE_TAG (io_watch);
		TEST_FREE_TAG (loop_func);

		nih_main_loop ();

		TEST_FALSE (disconnected);
		TEST_TRUE (my_message_received);

		TEST_NOT_FREE (io_watch);
		TEST_NOT_FREE (loop_func);

		dbus_connection_unref (conn);

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		dbus_shutdown ();
	}


	/* Check that by using a GUID we can reuse connections to the same
	 * server, the second call to connect just returns the same
	 * connection as the first.
	 */
	TEST_FEATURE ("with multiple shared connections");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh = nih_signal_add_handler (NULL, SIGTERM,
								     nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  NULL, NULL);
				assert (server != NULL);

				dbus_server_set_new_connection_function (server, my_new_connection,
									 NULL, NULL);

				client_connection = NULL;

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				if (client_connection) {
					dbus_connection_close (client_connection);
					dbus_connection_unref (client_connection);
				}

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}

			conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus,guid=deadbeef",
						 my_disconnect_handler);

			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));

			assert (! NIH_LIST_EMPTY (nih_io_watches));
			io_watch = (NihIoWatch *)nih_io_watches->next;
			dbus_connection_get_unix_fd (conn, &fd);
			assert (io_watch->fd == fd);

			assert (! NIH_LIST_EMPTY (nih_main_loop_functions));
			loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
			assert (loop_func->data == conn);
		}

		TEST_FREE_TAG (io_watch);
		TEST_FREE_TAG (loop_func);

		last_conn = conn;

		/* Make another connection */
		conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus,guid=deadbeef",
					 my_disconnect_handler);

		if (test_alloc_failed
		    && (conn == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
		} else {
			TEST_EQ_P (conn, last_conn);
		}

		/* Still should be a single I/O watch */
		TEST_NOT_FREE (io_watch);
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		TEST_EQ_P ((NihIoWatch *)nih_io_watches->next, io_watch);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Still should be a single main loop function */
		TEST_NOT_FREE (loop_func);
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		TEST_EQ_P ((NihMainLoopFunc *)nih_main_loop_functions->next,
			   loop_func);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		/* Disconnection should free both references */
		disconnected = FALSE;
		last_disconnection = NULL;

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		nih_main_loop ();

		TEST_TRUE (disconnected);
		TEST_EQ_P (last_disconnection, last_conn);

		TEST_FREE (io_watch);
		TEST_FREE (loop_func);

		dbus_shutdown ();
	}


	/* Check that we can create a new connection to a listening dbus
	 * server, it should return a hooked up object but if the server
	 * immediately drops it, should get disconnected.
	 */
	TEST_FEATURE ("with server that drops our connection");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh = nih_signal_add_handler (NULL, SIGTERM,
								     nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  NULL, NULL);
				assert (server != NULL);

				dbus_server_set_new_connection_function (server, my_new_connection_drop,
									 NULL, NULL);

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}

			conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
						 my_disconnect_handler);

			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));

			assert (! NIH_LIST_EMPTY (nih_io_watches));
			io_watch = (NihIoWatch *)nih_io_watches->next;
			dbus_connection_get_unix_fd (conn, &fd);
			assert (io_watch->fd == fd);

			assert (! NIH_LIST_EMPTY (nih_main_loop_functions));
			loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
			assert (loop_func->data == conn);
		}

		TEST_FREE_TAG (io_watch);
		TEST_FREE_TAG (loop_func);

		disconnected = FALSE;
		last_disconnection = NULL;

		nih_main_loop ();

		TEST_TRUE (disconnected);
		TEST_EQ_P (last_disconnection, conn);

		TEST_FREE (io_watch);
		TEST_FREE (loop_func);

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		dbus_shutdown ();
	}


	/* Check that if we create a new connection to a non-listening
	 * address, no object is returned.
	 */
	TEST_FEATURE ("with non-listening server");
	TEST_ALLOC_FAIL {
		conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
					 NULL);

		TEST_EQ_P (conn, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		TEST_EQ_STR (((NihDBusError *)err)->name, DBUS_ERROR_NO_SERVER);
		nih_free (err);

		dbus_shutdown ();
	}


	/* Check that we can make a method call on the connection to the
	 * server and that we can receive its reply, all from the main
	 * loop.
	 */
	TEST_FEATURE ("with method call and reply");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh = nih_signal_add_handler (NULL, SIGTERM,
								     nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  my_method_connect_handler, NULL);
				assert (server != NULL);

				client_connection = NULL;

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				if (client_connection) {
					dbus_connection_close (client_connection);
					dbus_connection_unref (client_connection);
				}

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}
		}

		conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
					 NULL);

		if (test_alloc_failed
		    && (conn == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			kill (dbus_pid, SIGTERM);

			waitpid (dbus_pid, &status, 0);
			TEST_TRUE (WIFEXITED (status));
			TEST_EQ (WEXITSTATUS (status), 0);

			dbus_shutdown ();
			continue;
		}

		TEST_NE_P (conn, NULL);
		TEST_TRUE (dbus_connection_get_is_connected (conn));

		method_call = dbus_message_new_method_call (
			"com.netsplit.Nih.Test",
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"TestMethod");
		pending_call = NULL;

		ret = dbus_connection_send_with_reply (conn, method_call,
						       &pending_call,
						       30000);

		if (test_alloc_failed
		    && (ret == FALSE)) {
			dbus_message_unref (method_call);
			assert (! pending_call);

			dbus_connection_unref (conn);

			kill (dbus_pid, SIGTERM);

			waitpid (dbus_pid, &status, 0);
			TEST_TRUE (WIFEXITED (status));
			TEST_EQ (WEXITSTATUS (status), 0);

			dbus_shutdown ();
			continue;
		}


		dbus_message_unref (method_call);
		assert (pending_call != NULL);

		dbus_pending_call_set_notify (pending_call, my_notify_function,
					      NULL, NULL);

		nih_main_loop ();

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_NE_P (reply, NULL);

		dbus_pending_call_unref (pending_call);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);

		dbus_message_unref (reply);

		dbus_connection_unref (conn);

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		dbus_shutdown ();
	}


	/* Check that we can make a method call on the connection to the
	 * server and that it can timeout, all from the main loop.
	 */
	TEST_FEATURE ("with method call and timeout");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			TEST_CHILD_WAIT (dbus_pid, wait_fd) {
				NihSignal *sh;

				nih_signal_set_handler (SIGTERM, nih_signal_handler);
				assert (sh  = nih_signal_add_handler (NULL, SIGTERM,
								      nih_main_term_signal, NULL));

				server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							  NULL, NULL);
				assert (server != NULL);

				dbus_server_set_new_connection_function (server, my_new_connection,
									 NULL, NULL);

				client_connection = NULL;

				TEST_CHILD_RELEASE (wait_fd);

				nih_main_loop ();

				if (client_connection) {
					dbus_connection_close (client_connection);
					dbus_connection_unref (client_connection);
				}

				dbus_server_disconnect (server);
				dbus_server_unref (server);

				dbus_shutdown ();
				nih_free (sh);
				exit (0);
			}
		}

		conn = nih_dbus_connect ("unix:abstract=/com/netsplit/nih/test_dbus",
					 NULL);

		if (test_alloc_failed
		    && (conn == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			kill (dbus_pid, SIGTERM);

			waitpid (dbus_pid, &status, 0);
			TEST_TRUE (WIFEXITED (status));
			TEST_EQ (WEXITSTATUS (status), 0);

			dbus_shutdown ();
			continue;
		}

		TEST_NE_P (conn, NULL);
		TEST_TRUE (dbus_connection_get_is_connected (conn));

		method_call = dbus_message_new_method_call (
			"com.netsplit.Nih.Test",
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"TestMethod");
		pending_call = NULL;

		ret = dbus_connection_send_with_reply (conn, method_call,
						       &pending_call,
						       100);

		if (test_alloc_failed
		    && (ret == FALSE)) {
			dbus_message_unref (method_call);
			assert (! pending_call);

			dbus_connection_unref (conn);

			kill (dbus_pid, SIGTERM);

			waitpid (dbus_pid, &status, 0);
			TEST_TRUE (WIFEXITED (status));
			TEST_EQ (WEXITSTATUS (status), 0);

			dbus_shutdown ();
			continue;
		}


		dbus_message_unref (method_call);
		assert (pending_call != NULL);

		dbus_pending_call_set_notify (pending_call, my_notify_function,
					      NULL, NULL);

		nih_main_loop ();

		TEST_TRUE (dbus_pending_call_get_completed (pending_call));

		reply = dbus_pending_call_steal_reply (pending_call);
		TEST_NE_P (reply, NULL);

		dbus_pending_call_unref (pending_call);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_ERROR);
		TEST_EQ_STR (dbus_message_get_error_name (reply),
			     DBUS_ERROR_NO_REPLY);

		dbus_message_unref (reply);

		dbus_connection_unref (conn);

		kill (dbus_pid, SIGTERM);

		waitpid (dbus_pid, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

		dbus_shutdown ();
	}
}


void
test_bus (void)
{
	DBusServer *     server;
	DBusConnection * conn;
	DBusConnection * last_conn;
	NihIoWatch *     io_watch = NULL;
	NihMainLoopFunc *loop_func = NULL;
	NihError *       err;
	pid_t            pid1;
	pid_t            pid2;
	int              fd;
	int              wait_fd;
	int              status;

	TEST_FUNCTION ("nih_dbus_bus");

	conn = dbus_bus_get_private (DBUS_BUS_SESSION, NULL);
	if (! conn) {
		printf ("SKIP: session bus not available\n");
		goto system_bus;
	}
	dbus_connection_close (conn);
	dbus_connection_unref (conn);
	dbus_shutdown ();


	/* Check that we can create a connection to the D-Bus session bus,
	 * the returned object should be hooked up to the main loop.
	 */
	TEST_FEATURE ("with session bus");
	TEST_ALLOC_FAIL {
		conn = nih_dbus_bus (DBUS_BUS_SESSION, my_disconnect_handler);

		if (test_alloc_failed) {
			TEST_EQ_P (conn, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			dbus_shutdown ();
			continue;
		}

		TEST_NE_P (conn, NULL);
		TEST_TRUE (dbus_connection_get_is_connected (conn));

		/* Should be a single I/O watch */
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		io_watch = (NihIoWatch *)nih_io_watches->next;
		dbus_connection_get_unix_fd (conn, &fd);
		TEST_EQ (io_watch->fd, fd);
		TEST_NE_P (io_watch->data, NULL);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Should be a single main loop function. */
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
		TEST_EQ_P (loop_func->data, conn);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		dbus_connection_unref (conn);
		dbus_shutdown ();
	}


system_bus:
	conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, NULL);
	if (! conn) {
		printf ("SKIP: system bus not available\n");
		return;
	}
	dbus_connection_close (conn);
	dbus_connection_unref (conn);
	dbus_shutdown ();


	/* Check that we can create a connection to the D-Bus system bus,
	 * the returned object should be hooked up to the main loop.
	 */
	TEST_FEATURE ("with system bus");
	TEST_ALLOC_FAIL {
		conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

		if (test_alloc_failed) {
			TEST_EQ_P (conn, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			dbus_shutdown ();
			continue;
		}

		TEST_NE_P (conn, NULL);
		TEST_TRUE (dbus_connection_get_is_connected (conn));

		/* Should be a single I/O watch */
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		io_watch = (NihIoWatch *)nih_io_watches->next;
		dbus_connection_get_unix_fd (conn, &fd);
		TEST_EQ (io_watch->fd, fd);
		TEST_NE_P (io_watch->data, NULL);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Should be a single main loop function. */
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
		TEST_EQ_P (loop_func->data, conn);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		dbus_connection_unref (conn);
		dbus_shutdown ();
	}


	/* Check that we can share connections to a bus. */
	TEST_FEATURE ("with shared bus connection");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));

			assert (! NIH_LIST_EMPTY (nih_io_watches));
			io_watch = (NihIoWatch *)nih_io_watches->next;
			dbus_connection_get_unix_fd (conn, &fd);
			assert (io_watch->fd == fd);

			assert (! NIH_LIST_EMPTY (nih_main_loop_functions));
			loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
			assert (loop_func->data == conn);
		}

		TEST_FREE_TAG (io_watch);
		TEST_FREE_TAG (loop_func);

		last_conn = conn;

		/* Make another connection */
		conn = nih_dbus_bus (DBUS_BUS_SYSTEM, my_disconnect_handler);

		if (test_alloc_failed
		    && (conn == NULL)) {
			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);
		} else {
			TEST_EQ_P (conn, last_conn);
		}

		/* Still should be a single I/O watch */
		TEST_NOT_FREE (io_watch);
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		TEST_EQ_P ((NihIoWatch *)nih_io_watches->next, io_watch);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Still should be a single main loop function */
		TEST_NOT_FREE (loop_func);
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		TEST_EQ_P ((NihMainLoopFunc *)nih_main_loop_functions->next,
			   loop_func);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		dbus_connection_unref (conn);
		dbus_connection_unref (last_conn);
		dbus_shutdown ();
	}


	/* Check that if the bus disconnects before registration, NULL
	 * is returned along with an error.  Stock dbus tends to bail out
	 * with an exit code, so we watch very carefully for that ;-)
	 */
	TEST_FEATURE ("with disconnection before registration");
	TEST_CHILD (pid1) {
		TEST_CHILD_WAIT (pid2, wait_fd) {
			NihSignal *sh;

			nih_signal_set_handler (SIGTERM, nih_signal_handler);
			assert (sh = nih_signal_add_handler (NULL, SIGTERM,
							     nih_main_term_signal, NULL));

			server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
						  NULL, NULL);
			assert (server != NULL);

			dbus_server_set_new_connection_function (server, my_new_connection_drop,
								 NULL, NULL);

			TEST_CHILD_RELEASE (wait_fd);

			nih_main_loop ();

			dbus_server_disconnect (server);
			dbus_server_unref (server);

			dbus_shutdown ();
			nih_free (sh);
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

		kill (pid2, SIGTERM);

		waitpid (pid2, &status, 0);
		TEST_TRUE (WIFEXITED (status));
		TEST_EQ (WEXITSTATUS (status), 0);

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
	pid_t            dbus_pid;
	int              wait_fd;
	DBusServer *     server;
	DBusConnection * conn = NULL;
	NihIoWatch *     io_watch = NULL;
	NihMainLoopFunc *loop_func = NULL;
	int              ret;
	int              fd;
	int              status;

	TEST_FUNCTION ("nih_dbus_setup");
	TEST_CHILD_WAIT (dbus_pid, wait_fd) {
		NihSignal *sh;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (sh = nih_signal_add_handler (NULL, SIGTERM,
						     nih_main_term_signal, NULL));

		server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
					  NULL, NULL);
		assert (server != NULL);

		dbus_server_set_new_connection_function (server, my_new_connection,
							 NULL, NULL);

		client_connection = NULL;

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		if (client_connection) {
			dbus_connection_close (client_connection);
			dbus_connection_unref (client_connection);
		}

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();
		nih_free (sh);
		exit (0);
	}


	/* Check that we can setup a new connection for use with the
	 * nih main loop.
	 */
	TEST_FEATURE ("with new connection");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			conn = dbus_connection_open_private ("unix:abstract=/com/netsplit/nih/test_dbus",
							     NULL);
			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));
		}

		dbus_connection_set_exit_on_disconnect (conn, FALSE);

		ret = nih_dbus_setup (conn, NULL);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_connection_close (conn);
			dbus_connection_unref (conn);

			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		/* Should be a single I/O watch */
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		io_watch = (NihIoWatch *)nih_io_watches->next;
		dbus_connection_get_unix_fd (conn, &fd);
		TEST_EQ (io_watch->fd, fd);
		TEST_NE_P (io_watch->data, NULL);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Should be a single main loop function. */
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
		TEST_EQ_P (loop_func->data, conn);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		dbus_connection_close (conn);
		dbus_connection_unref (conn);

		dbus_shutdown ();
	}


	/* Check that if we try and set the same connection up again,
	 * nothing changes.
	 */
	TEST_FEATURE ("with existing connection");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			conn = dbus_connection_open_private ("unix:abstract=/com/netsplit/nih/test_dbus",
							     NULL);
			assert (conn != NULL);
			assert (dbus_connection_get_is_connected (conn));

			dbus_connection_set_exit_on_disconnect (conn, FALSE);

			ret = nih_dbus_setup (conn, NULL);
			assert (ret == 0);

			assert (! NIH_LIST_EMPTY (nih_io_watches));
			io_watch = (NihIoWatch *)nih_io_watches->next;
			dbus_connection_get_unix_fd (conn, &fd);
			assert (io_watch->fd == fd);

			assert (! NIH_LIST_EMPTY (nih_main_loop_functions));
			loop_func = (NihMainLoopFunc *)nih_main_loop_functions->next;
			assert (loop_func->data == conn);
		}

		TEST_FREE_TAG (io_watch);
		TEST_FREE_TAG (loop_func);

		ret = nih_dbus_setup (conn, NULL);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);
		} else {
			TEST_EQ (ret, 0);
		}

		/* Still should be a single I/O watch */
		TEST_NOT_FREE (io_watch);
		TEST_LIST_NOT_EMPTY (nih_io_watches);
		TEST_EQ_P ((NihIoWatch *)nih_io_watches->next, io_watch);
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		/* Still should be a single main loop function */
		TEST_NOT_FREE (loop_func);
		TEST_LIST_NOT_EMPTY (nih_main_loop_functions);
		TEST_EQ_P ((NihMainLoopFunc *)nih_main_loop_functions->next,
			   loop_func);
		TEST_EQ_P (loop_func->entry.next, nih_main_loop_functions);

		dbus_connection_close (conn);
		dbus_connection_unref (conn);

		dbus_shutdown ();
	}


	kill (dbus_pid, SIGTERM);

	waitpid (dbus_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);
}


static int             connected = FALSE;
static int             drop_connection = FALSE;
static DBusConnection *last_connection = NULL;

static int
my_connect_handler (DBusServer *    server,
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
	DBusServer *    other_server = NULL;
	DBusServer *    server = NULL;
	DBusConnection *conn = NULL;
	DBusConnection *server_conn = NULL;
	NihIoWatch *    io_watch;
	NihError *      err;

	TEST_FUNCTION ("nih_dbus_server");

	/* Check that we can create a new D-Bus server instance and that
	 * it is hooked up to the main loop with an IoWatch.
	 */
	TEST_FEATURE ("with new server");
	TEST_ALLOC_FAIL {
		server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
					  NULL, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (server, NULL);

			err = nih_error_get ();
			TEST_EQ (err->number, ENOMEM);
			nih_free (err);

			dbus_shutdown ();
			continue;
		}

		TEST_NE_P (server, NULL);

		TEST_LIST_NOT_EMPTY (nih_io_watches);
		io_watch = (NihIoWatch *)nih_io_watches->next;
		TEST_EQ_P (io_watch->entry.next, nih_io_watches);

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();
	}


	/* Check that a connection to that server results in the connect
	 * handler being called, and that if that returns TRUE, the
	 * connection remains open.
	 */
	TEST_FEATURE ("with connection to server");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
						  my_connect_handler,
						  NULL);
			assert (server != NULL);
		}

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

		dbus_connection_close (conn);
		dbus_connection_unref (conn);

		dbus_connection_close (server_conn);
		dbus_connection_unref (server_conn);

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();
	}


	/* Check that if the client disconnects, the server connection
	 * disconnect handler is called and unreferenced.
	 */
	TEST_FEATURE ("with disconnect by client");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
						  my_connect_handler,
						  my_disconnect_handler);
			assert (server != NULL);

			conn = dbus_connection_open_private ("unix:abstract=/com/netsplit/nih/test_dbus", NULL);
			assert (conn != NULL);

			connected = FALSE;
			last_connection = NULL;

			nih_main_loop ();

			assert (connected);
			server_conn = last_connection;
		}

		disconnected = FALSE;
		last_disconnection = NULL;

		dbus_connection_close (conn);
		dbus_connection_unref (conn);

		nih_main_loop ();

		TEST_TRUE (disconnected);
		TEST_EQ_P (last_disconnection, server_conn);

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();
	}


	/* Check that if the connect handler returns FALSE, the connection
	 * is abandoned and the client disconnected.
	 */
	TEST_FEATURE ("with decline by connect handler");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
						  my_connect_handler,
						  my_disconnect_handler);
			assert (server != NULL);
		}

		conn = dbus_connection_open_private ("unix:abstract=/com/netsplit/nih/test_dbus", NULL);

		TEST_NE_P (conn, NULL);

		connected = FALSE;
		last_connection = NULL;
		drop_connection = TRUE;

		disconnected = FALSE;
		last_disconnection = NULL;

		nih_main_loop ();

		TEST_TRUE (connected);

		while (dbus_connection_read_write_dispatch (conn, -1))
			;

		TEST_FALSE (dbus_connection_get_is_connected (conn));

		/* Disconnect handler should not be called */
		TEST_FALSE (disconnected);
		TEST_EQ_P (last_disconnection, NULL);

		dbus_connection_unref (conn);

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();
	}


	/* Check that creating a server on an address which is already in
	 * use returns no object and the error.
	 */
	TEST_FEATURE ("with address in use");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			other_server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
							NULL, NULL);
			assert (other_server != NULL);
		}

		server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
					  NULL, NULL);

		TEST_EQ_P (server, NULL);

		err = nih_error_get ();
		TEST_EQ (err->number, NIH_DBUS_ERROR);
		TEST_ALLOC_SIZE (err, sizeof (NihDBusError));
		TEST_EQ_STR (((NihDBusError *)err)->name, DBUS_ERROR_ADDRESS_IN_USE);
		nih_free (err);

		dbus_server_disconnect (other_server);
		dbus_server_unref (other_server);

		dbus_shutdown ();
	}
}


int
main (int   argc,
      char *argv[])
{
	nih_timer_init ();
	nih_signal_init ();
	nih_child_init ();
	nih_io_init ();
	nih_main_loop_init ();
	nih_error_init ();

	test_connect ();
	test_bus ();
	test_setup ();
	test_server ();

	return 0;
}
