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
	&com_netsplit_Nih_Glue,
	NULL
};

int
my_test_method (void            *data,
		NihDBusMessage  *message,
		const char      *input,
		int32_t          flags,
		const char     **output)
{
	static int32_t last_flags = -1;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if ((flags == 1) && (last_flags != 1)) {
		last_flags = flags;

		nih_dbus_error_raise ("com.netsplit.Nih.IllegalValue",
				      "The value given was not legal");

		return -1;
	}

	if ((flags == 2) && (last_flags != 2)) {
		last_flags = flags;

		errno = ENOMEM;
		nih_error_raise_system ();
		return -1;
	}

	if ((flags == 3) && (last_flags != 3)) {
		last_flags = flags;

		errno = EBADF;
		nih_error_raise_system ();
		return -1;
	}

	last_flags = flags;
	*output = nih_strdup (message, input);

	return 0;
}

int
my_byte_to_str (void            *data,
		NihDBusMessage  *message,
		uint8_t          input,
		const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%hhu", (unsigned char)input);

	return 0;
}

int
my_str_to_byte (void           *data,
		NihDBusMessage *message,
		const char     *input,
		uint8_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint8_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_boolean_to_str (void            *data,
		   NihDBusMessage  *message,
		   int              input,
		   const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input ? "True" : "False");

	return 0;
}

int
my_str_to_boolean (void           *data,
		   NihDBusMessage *message,
		   const char     *input,
		   int            *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strcmp (input, "False")) {
		*output = 0;
	} else {
		*output = 1;
	}

	return 0;
}

int
my_int16_to_str (void            *data,
		 NihDBusMessage  *message,
		 int16_t          input,
		 const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%hd", (short)input);

	return 0;
}

int
my_str_to_int16 (void           *data,
		 NihDBusMessage *message,
		 const char     *input,
		 int16_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (int16_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_uint16_to_str (void            *data,
		  NihDBusMessage  *message,
		  uint16_t         input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%hu", (unsigned short)input);

	return 0;
}

int
my_str_to_uint16 (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  uint16_t       *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint16_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_int32_to_str (void            *data,
		 NihDBusMessage  *message,
		 int32_t          input,
		 const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%d", (int)input);

	return 0;
}

int
my_str_to_int32 (void           *data,
		 NihDBusMessage *message,
		 const char     *input,
		 int32_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (int32_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_uint32_to_str (void            *data,
		  NihDBusMessage  *message,
		  uint32_t         input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%u", (unsigned int)input);

	return 0;
}

int
my_str_to_uint32 (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  uint32_t       *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint32_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_int64_to_str (void            *data,
		 NihDBusMessage  *message,
		 int64_t          input,
		 const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%lld", (long long)input);

	return 0;
}

int
my_str_to_int64 (void           *data,
		 NihDBusMessage *message,
		 const char     *input,
		 int64_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (int64_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_uint64_to_str (void            *data,
		  NihDBusMessage  *message,
		  uint64_t         input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%llu", (unsigned long long)input);

	return 0;
}

int
my_str_to_uint64 (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  uint64_t       *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint64_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_double_to_str (void            *data,
		  NihDBusMessage  *message,
		  double           input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%f", input);

	return 0;
}

int
my_str_to_double (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  double         *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = strtod (input, NULL);

	return 0;
}

int
my_object_path_to_str (void            *data,
		       NihDBusMessage  *message,
		       const char      *input,
		       const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_str_to_object_path (void            *data,
		       NihDBusMessage  *message,
		       const char      *input,
		       const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_signature_to_str (void            *data,
		     NihDBusMessage  *message,
		     const char      *input,
		     const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_str_to_signature (void            *data,
		     NihDBusMessage  *message,
		     const char      *input,
		     const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}


int
my_emit_signal (void           *data,
		NihDBusMessage *message,
		int32_t         signum)
{
	int ret;

	switch (signum) {
	case 0:
		ret = my_test_signal (message->conn,
				      dbus_message_get_path (message->message),
				      "hello there", 0);
		break;
	case 1:
		ret = my_emit_byte (message->conn,
				    dbus_message_get_path (message->message),
				    65);
		break;
	case 2:
		ret = my_emit_boolean (message->conn,
				       dbus_message_get_path (message->message),
				       TRUE);
		break;
	case 3:
		ret = my_emit_int16 (message->conn,
				     dbus_message_get_path (message->message),
				     1701);
		break;
	case 4:
		ret = my_emit_uint16 (message->conn,
				      dbus_message_get_path (message->message),
				      1701);
		break;
	case 5:
		ret = my_emit_int32 (message->conn,
				     dbus_message_get_path (message->message),
				     1701);
		break;
	case 6:
		ret = my_emit_uint32 (message->conn,
				      dbus_message_get_path (message->message),
				      1701);
		break;
	case 7:
		ret = my_emit_int64 (message->conn,
				     dbus_message_get_path (message->message),
				     1701);
		break;
	case 8:
		ret = my_emit_uint64 (message->conn,
				      dbus_message_get_path (message->message),
				      1701);
		break;
	case 9:
		ret = my_emit_double (message->conn,
				      dbus_message_get_path (message->message),
				      3.141);
		break;
	case 10:
		ret = my_emit_string (message->conn,
				      dbus_message_get_path (message->message),
				      "test data");
		break;
	case 11:
		ret = my_emit_object_path (message->conn,
					   dbus_message_get_path (message->message),
					   "/com/netsplit/Nih");
		break;
	case 12:
		ret = my_emit_signature (message->conn,
					 dbus_message_get_path (message->message),
					 "a{sv}");
		break;
	}

	TEST_EQ (ret, 0);

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
test_method_marshal (void)
{
	DBusConnection *conn;
	DBusMessage    *message, *reply;
	DBusError       error;
	const char     *input, *output;
	dbus_unichar_t  byte_arg;
	dbus_bool_t     boolean_arg;
	dbus_int16_t    int16_arg;
	dbus_uint16_t   uint16_arg;
	dbus_int32_t    flags, int32_arg;
	dbus_uint32_t   uint32_arg;
	dbus_int64_t    int64_arg;
	dbus_uint64_t   uint64_arg;
	double          double_arg;

	TEST_GROUP ("method marshalling");
	dbus_error_init (&error);


	/* Check that we can make a D-Bus method call, passing in the
	 * expected arguments and receiving an expected reply.
	 */
	TEST_FEATURE ("with valid argument");
	conn = my_setup ();

	input = "test data";
	flags = 0;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "test data");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises a D-Bus error and
	 * returns non-zero, the error is returned as a real D-Bus error
	 * with the same name and message.
	 */
	TEST_FEATURE ("with returned D-Bus error");
	conn = my_setup ();

	input = "test data";
	flags = 1;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, "com.netsplit.Nih.IllegalValue");

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises ENOMEM and returns
	 * non-zero, the D-Bus need more memory condition is returned which
	 * will make D-Bus repeat the method handler (at which point it
	 * will work).
	 */
	TEST_FEATURE ("with out of memory error");
	conn = my_setup ();

	input = "test data";
	flags = 2;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "test data");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises a different error
	 * and returns non-zero, the generic D-Bus Failed error is returned.
	 */
	TEST_FEATURE ("with unknown error");
	conn = my_setup ();

	input = "test data";
	flags = 3;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_FAILED);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with the wrong argument
	 * type, we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with wrong argument type");
	conn = my_setup ();

	input = "test data";
	output = "not test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
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
	 * we also get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with too many arguments");
	conn = my_setup ();

	input = "test data";
	flags = 0;
	output = "not test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
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
	 * we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with missing arguments");
	conn = my_setup ();

	input = "test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");

	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we make the method call without any arguments,
	 * we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with no arguments");
	conn = my_setup ();

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we say that we're not expecting a reply, none will
	 * be generated as allowed by the D-Bus spec.
	 */
	TEST_FEATURE ("with drive-by call");
	conn = my_setup ();

	input = "test data";
	flags = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	dbus_message_set_no_reply (message, TRUE);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   500, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_NO_REPLY);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


 	/* Check that an input argument of Byte type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Byte input argument");
	conn = my_setup ();

	byte_arg = 65;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"ByteToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_BYTE, &byte_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "65");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Byte type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Byte output argument");
	conn = my_setup ();

	input = "65";
	byte_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToByte");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_BYTE, &byte_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (byte_arg, 65);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Boolean type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Boolean input argument");
	conn = my_setup ();

	boolean_arg = 1;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"BooleanToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_BOOLEAN, &boolean_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "True");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Boolean type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Boolean output argument");
	conn = my_setup ();

	input = "False";
	boolean_arg = TRUE;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToBoolean");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_BOOLEAN, &boolean_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (boolean_arg, FALSE);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Int16 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int16 input argument");
	conn = my_setup ();

	int16_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int16ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT16, &int16_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Int16 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int16 output argument");
	conn = my_setup ();

	input = "1701";
	int16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt16");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT16, &int16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int16_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of UInt16 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt16 input argument");
	conn = my_setup ();

	uint16_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"UInt16ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_UINT16, &uint16_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of UInt16 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt16 output argument");
	conn = my_setup ();

	input = "1701";
	uint16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToUInt16");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_UINT16, &uint16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint16_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Int32 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int32 input argument");
	conn = my_setup ();

	int32_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int32ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &int32_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Int32 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int32 output argument");
	conn = my_setup ();

	input = "1701";
	int32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt32");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT32, &int32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int32_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of UInt32 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt32 input argument");
	conn = my_setup ();

	uint32_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"UInt32ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_UINT32, &uint32_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of UInt32 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt32 output argument");
	conn = my_setup ();

	input = "1701";
	uint32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToUInt32");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_UINT32, &uint32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint32_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Int64 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int64 input argument");
	conn = my_setup ();

	int64_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int64ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT64, &int64_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Int64 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int64 output argument");
	conn = my_setup ();

	input = "1701";
	int64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt64");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT64, &int64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int64_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of UInt64 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt64 input argument");
	conn = my_setup ();

	uint64_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"UInt64ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_UINT64, &uint64_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of UInt64 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt64 output argument");
	conn = my_setup ();

	input = "1701";
	uint64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToUInt64");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_UINT64, &uint64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint64_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of Double type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Double input argument");
	conn = my_setup ();

	double_arg = 3.141592;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"DoubleToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_DOUBLE, &double_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "3.141592");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Double type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Double output argument");
	conn = my_setup ();

	input = "3.141";
	double_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToDouble");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_DOUBLE, &double_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (double_arg, 3.141);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of ObjectPatch type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with ObjectPath input argument");
	conn = my_setup ();

	input = "/com/netsplit/Nih";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"ObjectPathToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_OBJECT_PATH, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "/com/netsplit/Nih");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of ObjectPath type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with ObjectPath output argument");
	conn = my_setup ();

	input = "/com/netsplit/Nih";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToObjectPath");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_OBJECT_PATH, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "/com/netsplit/Nih");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of Signature type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Signature input argument");
	conn = my_setup ();

	input = "a{sv}";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"SignatureToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_SIGNATURE, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "a{sv}");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Signature type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Signature output argument");
	conn = my_setup ();

	input = "a{sv}";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToSignature");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_SIGNATURE, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "a{sv}");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);
}


void
test_signal_dispatch (void)
{
	DBusConnection *conn;
	DBusMessage    *message, *reply;
	DBusError       error;
	dbus_int32_t    signum, flags;
	const char     *str;
	dbus_unichar_t  byte_arg;
	dbus_bool_t     boolean_arg;
	dbus_int16_t    int16_arg;
	dbus_uint16_t   uint16_arg;
	dbus_int32_t    int32_arg;
	dbus_uint32_t   uint32_arg;
	dbus_int64_t    int64_arg;
	dbus_uint64_t   uint64_arg;
	double          double_arg;

	TEST_GROUP ("signal dispatching");
	dbus_error_init (&error);

	/* Check that an ordinary signal can be emitted by the server with
	 * a set of arguments, and that we can catch it with them as we
	 * expected.  No particular error conditions to check for, since the
	 * only one is out of memory.
	 */
	TEST_FEATURE ("with ordinary signal");
	conn = my_setup ();

	signum = 0;
	str = NULL;
	flags = -1;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "TestSignal"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INT32, &flags,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "hello there");
	TEST_EQ (flags, 0);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Byte argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Byte argument");
	conn = my_setup ();

	signum = 1;
	byte_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitByte"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_BYTE, &byte_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (byte_arg, 65);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Boolean argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Boolean argument");
	conn = my_setup ();

	signum = 2;
	boolean_arg = FALSE;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitBoolean"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_BOOLEAN, &boolean_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (boolean_arg, TRUE);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Int16 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Int16 argument");
	conn = my_setup ();

	signum = 3;
	int16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt16"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_INT16, &int16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int16_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a UInt16 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with UInt16 argument");
	conn = my_setup ();

	signum = 4;
	uint16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitUInt16"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_UINT16, &uint16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint16_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Int32 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Int32 argument");
	conn = my_setup ();

	signum = 5;
	int32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt32"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_INT32, &int32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int32_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a UInt32 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with UInt32 argument");
	conn = my_setup ();

	signum = 6;
	uint32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitUInt32"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_UINT32, &uint32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint32_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Int64 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Int64 argument");
	conn = my_setup ();

	signum = 7;
	int64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt64"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_INT64, &int64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int64_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a UInt64 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with UInt64 argument");
	conn = my_setup ();

	signum = 8;
	uint64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitUInt64"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_UINT64, &uint64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint64_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Double argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Double argument");
	conn = my_setup ();

	signum = 9;
	double_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitDouble"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_DOUBLE, &double_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (double_arg, 3.141);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a String argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with String argument");
	conn = my_setup ();

	signum = 10;
	str = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitString"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "test data");

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a ObjectPath argument can be emitted and
	 * that we can catch it as expected.
	 */
	TEST_FEATURE ("with ObjectPath argument");
	conn = my_setup ();

	signum = 11;
	str = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitObjectPath"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_OBJECT_PATH, &str,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "/com/netsplit/Nih");

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Signature argument can be emitted and
	 * that we can catch it as expected.
	 */
	TEST_FEATURE ("with Signature argument");
	conn = my_setup ();

	signum = 12;
	str = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitSignature"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_SIGNATURE, &str,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "a{sv}");

	dbus_message_unref (message);

	my_teardown (conn);
}


int
main (int   argc,
      char *argv[])
{
	test_method_marshal ();
	test_signal_dispatch ();

	return 0;
}
