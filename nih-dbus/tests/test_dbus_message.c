/* libnih
 *
 * test_dbus_message.c - test suite for nih-dbus/dbus_message.c
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
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
#include <nih-dbus/test_dbus.h>

#include <dbus/dbus.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdlib.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/timer.h>
#include <nih/signal.h>
#include <nih/main.h>

#include <nih-dbus/dbus_connection.h>
#include <nih-dbus/dbus_object.h>

#include <nih-dbus/dbus_message.h>


void
test_message_new (void)
{
	NihDBusMessage *msg;
	pid_t           dbus_pid;
	DBusConnection *conn;
	DBusMessage    *message;

	/* Check that we can create a new DBus message structure, and that
	 * it references the connection and message.
	 */
	TEST_FUNCTION ("nih_dbus_message_new");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (conn);

	message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

	TEST_ALLOC_FAIL {
		msg = nih_dbus_message_new (NULL, conn, message);

		if (test_alloc_failed) {
			TEST_EQ_P (msg, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (msg, sizeof (NihDBusMessage));
		TEST_EQ_P (msg->conn, conn);
		TEST_EQ_P (msg->message, message);

		nih_free (msg);
	}

	dbus_message_unref (message);

	TEST_DBUS_CLOSE (conn);
	TEST_DBUS_END (dbus_pid);
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

	/* must reference the message */
	nih_ref (message, object);

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


int
main (int   argc,
      char *argv[])
{
	test_message_new ();
	test_message_error ();

	return 0;
}
