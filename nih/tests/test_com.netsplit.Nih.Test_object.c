/* libnih
 *
 * test_com.netsplit.Nih.Test_object.c - test suite for auto-generated
 * object bindings.
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

#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/signal.h>
#include <nih/main.h>
#include <nih/error.h>
#include <nih/errors.h>

#include <nih/dbus.h>

#include "com.netsplit.Nih.Test_object.h"


const static NihDBusInterface *my_interfaces[] = {
	&com_netsplit_Nih_Test,
	NULL
};

int
my_str_to_int (void           *data,
	       NihDBusMessage *message,
	       const char     *input,
	       int32_t        *output)
{
	intmax_t         i;
	static intmax_t  last_i = -1;
	char            *endptr;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	i = strtoimax (input, &endptr, 10);
	if (*endptr || (i < INT32_MIN) || (i > INT32_MAX)) {
		last_i = -1;

		nih_dbus_error_raise ("com.netsplit.Nih.IllegalValue",
				      "The value given was not legal");

		return -1;
	}

	if ((i == 123) && (last_i != 123)) {
		last_i = i;

		errno = ENOMEM;
		nih_error_raise_system ();
		return -1;
	}

	if (i == 999) {
		last_i = i;

		errno = EBADF;
		nih_error_raise_system ();
		return -1;
	}

	*output = (int32_t)i;
	last_i = i;

	return 0;
}

int
my_int_to_str (void           *data,
	       NihDBusMessage *message,
	       int32_t         input,
	       const char    **output)
{
	static int32_t last_input = -1;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if ((input == 123) && (last_input != 123)) {
		last_input = input;

		errno = ENOMEM;
		nih_error_raise_system ();
		return -1;
	}

	if (input == 999) {
		last_input = input;

		errno = EBADF;
		nih_error_raise_system ();
		return -1;
	}

	*output = nih_sprintf (message, "%d", (int)input);
	last_input = input;

	return 0;
}


static DBusConnection *server_conn = NULL;

static int
my_connect_handler (DBusServer     *server,
		    DBusConnection *conn)
{
	NihDBusObject *object;

	assert (server_conn == NULL);
	server_conn = conn;

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      my_interfaces, NULL);
	assert (object != NULL);

	return TRUE;
}

static pid_t server_pid;

static DBusConnection *
my_setup (void)
{
	DBusConnection *conn;
	int             wait_fd;

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		DBusServer *server;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test",
					  my_connect_handler, NULL);
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

	conn = dbus_connection_open ("unix:abstract=/com/netsplit/nih/test",
				     NULL);
	assert (conn != NULL);

	return conn;
}

static void
my_teardown (DBusConnection *conn)
{
	int status;

	kill (server_pid, SIGTERM);

	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	dbus_connection_unref (conn);

	dbus_shutdown ();
}


void
test_str_to_int (void)
{
	DBusConnection *conn;
	DBusMessage    *message, *reply;
	DBusError       error;
	const char     *input;
	dbus_int32_t    output;

	TEST_FUNCTION ("StrToInt");
	dbus_error_init (&error);


	/* Check that a D-Bus method call with expected arguments results
	 * in an expected reply.
	 */
	TEST_FEATURE ("with valid argument");
	conn = my_setup ();

	input = "42";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT32, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ (output, 42);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that the D-Bus method handler may raise a D-Bus error,
	 * in which case it's converted to a real D-Bus error return on
	 * the wire and replies accordingly.
	 */
	TEST_FEATURE ("with invalid argument");
	conn = my_setup ();

	input = "foo42";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, "com.netsplit.Nih.IllegalValue");

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler returns ENOMEM as a raised
	 * error, D-Bus tries the call again.
	 */
	TEST_FEATURE ("with out of memory error");
	conn = my_setup ();

	input = "123";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT32, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ (output, 123);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises another error type,
	 * the method fails with the generic D-Bus error.
	 */
	TEST_FEATURE ("with unknown error");
	conn = my_setup ();

	input = "999";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_FAILED);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with the wrong argument
	 * type, we get a proper error back.
	 */
	TEST_FEATURE ("with wrong argument type");
	conn = my_setup ();

	output = 42;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);
	
	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with too many arguments,
	 * we get a proper error back.
	 */
	TEST_FEATURE ("with too many arguments");
	conn = my_setup ();

	input = "42";
	output = 42;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call without enough arguments,
	 * we get a proper error back.
	 */
	TEST_FEATURE ("with missing arguments");
	conn = my_setup ();

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we do a drive-by D-Bus method call and don't want
	 * a reply, none is generated.
	 */
	TEST_FEATURE ("with drive-by call");
	conn = my_setup ();

	input = "42";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);
	dbus_message_set_no_reply (message, TRUE);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   500, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_NO_REPLY);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);
}

void
test_int_to_str (void)
{
	DBusConnection *conn;
	DBusMessage    *message, *reply;
	DBusError       error;
	dbus_int32_t    input;
	const char     *output;

	TEST_FUNCTION ("IntToStr");
	dbus_error_init (&error);


	/* Check that a D-Bus method call with expected arguments results
	 * in an expected reply.
	 */
	TEST_FEATURE ("with valid argument");
	conn = my_setup ();

	input = 42;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "42");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler returns ENOMEM as a raised
	 * error, D-Bus tries the call again.
	 */
	TEST_FEATURE ("with out of memory error");
	conn = my_setup ();

	input = 123;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "123");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises another error type,
	 * the method fails with the generic D-Bus error.
	 */
	TEST_FEATURE ("with unknown error");
	conn = my_setup ();

	input = 999;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_FAILED);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with the wrong argument
	 * type, we get a proper error back.
	 */
	TEST_FEATURE ("with wrong argument type");
	conn = my_setup ();

	output = "42";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with too many arguments,
	 * we get a proper error back.
	 */
	TEST_FEATURE ("with too many arguments");
	conn = my_setup ();

	input = 42;
	output = "42";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &input,
				  DBUS_TYPE_STRING, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call without enough arguments,
	 * we get a proper error back.
	 */
	TEST_FEATURE ("with missing arguments");
	conn = my_setup ();

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we do a drive-by D-Bus method call and don't want
	 * a reply, none is generated.
	 */
	TEST_FEATURE ("with drive-by call");
	conn = my_setup ();

	input = 42;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"IntToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &input,
				  DBUS_TYPE_INVALID);
	dbus_message_set_no_reply (message, TRUE);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   500, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_NO_REPLY);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);
}


/**
 * server to accept connections.
 *
 * handler function to check the arguments and return a suitable reply.
 *
 * client to send the request and check the answers.
 **/

int
main (int   argc,
      char *argv[])
{
	test_str_to_int ();
	test_int_to_str ();

	return 0;
}
