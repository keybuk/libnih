/* libnih
 *
 * test_com.netsplit.Nih.Test_object.c - test suite for auto-generated
 * object bindings.
 *
 * Copyright © 2010 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2010 Canonical Ltd.
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

#include <dbus/dbus.h>

#include <sys/types.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/error.h>

#include <nih-dbus/dbus_object.h>

#include "tests/com.netsplit.Nih.Test_object.h"
#include "tests/com.netsplit.Nih.Test_impl.h"


void
test_ordinary_method (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_OrdinaryMethod_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"OrdinaryMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"OrdinaryMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.OrdinaryMethod.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"OrdinaryMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"OrdinaryMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/she/needs/more/of/ze/punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"OrdinaryMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"OrdinaryMethod");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_nameless_method (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_NamelessMethod_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"NamelessMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"NamelessMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.NamelessMethod.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"NamelessMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"NamelessMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/she/needs/more/of/ze/punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"NamelessMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"NamelessMethod");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_async_method (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;
	int             ret;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_AsyncMethod_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		async_method_input = NULL;
		async_method_message = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_EQ_STR (async_method_input, "she needs more of ze punishment");
		TEST_ALLOC_SIZE (async_method_message, sizeof (NihDBusMessage));
		TEST_ALLOC_PARENT (async_method_message, async_method_input);

		ret = my_test_async_method_reply (async_method_message,
						  async_method_input);

		if (test_alloc_failed
		    && (ret < 0)) {
			nih_free (async_method_message);
			nih_free (async_method_input);
			nih_free (object);
			continue;
		}

		TEST_EQ (ret, 0);

		nih_free (async_method_message);
		nih_free (async_method_input);

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.AsyncMethod.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be sent after the function returns
	 * using nih_dbus_message_error().
	 */
	TEST_FEATURE ("with error after function return");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		async_method_input = NULL;
		async_method_message = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_EQ_STR (async_method_input, "she needs more of ze punishment");
		TEST_ALLOC_SIZE (async_method_message, sizeof (NihDBusMessage));
		TEST_ALLOC_PARENT (async_method_message, async_method_input);

		TEST_ALLOC_SAFE {
			assert0 (nih_dbus_message_error (async_method_message,
							 "com.netsplit.Nih.Test.AsyncMethod.Fail",
							 "The method failed in some way"));
		}

		nih_free (async_method_message);
		nih_free (async_method_input);

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.AsyncMethod.Fail"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/she/needs/more/of/ze/punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"AsyncMethod");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_byte_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	uint8_t         byte_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_ByteToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ByteToStr");

		dbus_message_iter_init_append (method_call, &iter);

		byte_value = 97;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "97");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ByteToStr");

		dbus_message_iter_init_append (method_call, &iter);

		byte_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.ByteToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ByteToStr");

		dbus_message_iter_init_append (method_call, &iter);

		byte_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ByteToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ByteToStr");

		dbus_message_iter_init_append (method_call, &iter);

		byte_value = 97;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
						&byte_value);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ByteToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_byte (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	uint8_t         byte_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToByte_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToByte");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_BYTE_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &byte_value);
		TEST_EQ (byte_value, 97);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToByte");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToByte.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToByte");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToByte");

		dbus_message_iter_init_append (method_call, &iter);

		byte_value = 97;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToByte");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		byte_value = 97;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToByte");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_boolean_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	int             boolean_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_BooleanToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"BooleanToStr");

		dbus_message_iter_init_append (method_call, &iter);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "True");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"BooleanToStr");

		dbus_message_iter_init_append (method_call, &iter);

		boolean_value = FALSE;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.BooleanToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"BooleanToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"BooleanToStr");

		dbus_message_iter_init_append (method_call, &iter);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"BooleanToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_boolean (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	int             boolean_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToBoolean_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToBoolean");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_BOOLEAN_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &boolean_value);
		TEST_EQ (boolean_value, TRUE);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToBoolean");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToBoolean.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToBoolean");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToBoolean");

		dbus_message_iter_init_append (method_call, &iter);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToBoolean");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToBoolean");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_int16_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	int16_t         int16_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_Int16ToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int16_value = -42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "-42");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int16_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int16ToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int16_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int16_value = -42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
						&int16_value);

		str_value = "-42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	int16_t         int16_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToInt16_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_INT16_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &int16_value);
		TEST_EQ (int16_value, -42);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToInt16.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt16");

		dbus_message_iter_init_append (method_call, &iter);

		int16_value = -42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		int16_value = -42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt16");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_uint16_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	uint16_t        uint16_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_UInt16ToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint16_value = 42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "42");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint16_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt16ToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint16_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint16_value = 42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
						&uint16_value);

		str_value = "42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt16ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_uint16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	uint16_t        uint16_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToUInt16_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_UINT16_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &uint16_value);
		TEST_EQ (uint16_value, 42);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToUInt16.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt16");

		dbus_message_iter_init_append (method_call, &iter);

		uint16_value = 42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt16");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		uint16_value = 42;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt16");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_int32_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	int32_t         int32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_Int32ToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "-1048576");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int32_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32ToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int32_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	int32_t         int32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToInt32_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_INT32_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, -1048576);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToInt32.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32");

		dbus_message_iter_init_append (method_call, &iter);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_uint32_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_UInt32ToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "1048576");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt32ToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		str_value = "1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt32ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_uint32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	uint32_t        uint32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToUInt32_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_UINT32_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &uint32_value);
		TEST_EQ (uint32_value, 1048576);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToUInt32.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt32");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt32");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt32");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_int64_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	int64_t         int64_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_Int64ToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "-4815162342");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int64_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int64ToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int64_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
						&int64_value);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	int64_t         int64_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToInt64_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_INT64_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &int64_value);
		TEST_EQ (int64_value, -4815162342L);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToInt64.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt64");

		dbus_message_iter_init_append (method_call, &iter);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt64");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_uint64_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	uint64_t        uint64_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_UInt64ToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint64_value = 4815162342L;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "4815162342");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint64_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt64ToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint64_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		uint64_value = 4815162342;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
						&uint64_value);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UInt64ToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_uint64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	uint64_t        uint64_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToUInt64_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_UINT64_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &uint64_value);
		TEST_EQ (uint64_value, 4815162342);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToUInt64.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt64");

		dbus_message_iter_init_append (method_call, &iter);

		uint64_value = 4815162342;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt64");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		uint64_value = 4815162342L;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUInt64");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_double_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	double          double_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_DoubleToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DoubleToStr");

		dbus_message_iter_init_append (method_call, &iter);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "3.141597");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DoubleToStr");

		dbus_message_iter_init_append (method_call, &iter);

		double_value = 0;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.DoubleToStr.ZeroInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DoubleToStr");

		dbus_message_iter_init_append (method_call, &iter);

		double_value = 4;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DoubleToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "3.141597";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DoubleToStr");

		dbus_message_iter_init_append (method_call, &iter);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		str_value = "3.141597";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DoubleToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_double (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	double          double_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToDouble_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDouble");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "3.141597";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_DOUBLE_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &double_value);
		TEST_EQ (double_value, 3.141597);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDouble");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToDouble.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDouble");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDouble");

		dbus_message_iter_init_append (method_call, &iter);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDouble");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "3.141597";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDouble");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_object_path_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_ObjectPathToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ObjectPathToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ObjectPathToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.ObjectPathToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ObjectPathToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ObjectPathToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ObjectPathToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"ObjectPathToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_object_path (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToObjectPath_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToObjectPath");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_OBJECT_PATH_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToObjectPath");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToObjectPath.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToObjectPath");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToObjectPath");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToObjectPath");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToObjectPath");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_signature_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_SignatureToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"SignatureToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "a(ib)");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"SignatureToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.SignatureToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"SignatureToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "inva(x)id";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"SignatureToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"SignatureToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
						&str_value);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"SignatureToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_signature (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToSignature_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToSignature");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_SIGNATURE_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "a(ib)");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToSignature");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToSignature.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToSignature");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToSignature");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToSignature");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToSignature");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_struct_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	const char *    str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StructToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "Joe 34");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StructToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a structure member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra structure entry is given.
	 */
	TEST_FEATURE ("with extra member");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		str_value = "Male";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_struct (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	char *          str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToStruct_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStruct");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe 34";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_STRUCT_END_CHAR_AS_STRING));

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStruct");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToStruct.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStruct");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStruct");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStruct");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe 34";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStruct");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_int32_array_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	int32_t         int32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_Int32ArrayToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "4 8 15 16 23 42");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32ArrayToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if array elements of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "4";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "8";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "15";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "16";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "23";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4 8 15 16 23 42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;

		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "4 8 15 16 23 42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int32_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	int32_t         int32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToInt32Array_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32Array");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4 8 15 16 23 42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT32_AS_STRING));

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 4);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 15);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 16);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 23);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 42);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32Array");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToInt32Array.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32Array");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32Array");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32Array");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4 8 15 16 23 42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32Array");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_str_array_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrArrayToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "ze";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrArrayToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "this";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "is";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "a";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "test";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if array elements of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/she";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/needs";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/more";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/ze";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "ze";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_str_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToStrArray_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStrArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING));

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "she");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "needs");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "more");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "of");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "ze");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "punishment");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStrArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToStrArray.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStrArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStrArray");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "ze";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStrArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "ze";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStrArray");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_int32_array_array_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	int32_t         int32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_Int32ArrayArrayToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "4 8 15 16 23 42\n1 1 2 3 5 8");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32ArrayArrayToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if first-level array elements of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong first-level element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "4";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "8";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "15";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "16";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "23";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if second-level array elements of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong second-level element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubiter);

		str_value = "4";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "8";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "15";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "16";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "23";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4 8 15 16 23 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "4 8 15 16 23 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"Int32ArrayArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_int32_array_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	int32_t         int32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToInt32ArrayArray_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32ArrayArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4 8 15 16 23 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT32_AS_STRING));

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 4);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 15);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 16);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 23);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 42);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);
		dbus_message_iter_next (&subiter);


		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 1);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 1);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 2);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 3);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 5);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32ArrayArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToInt32ArrayArray.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32ArrayArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32ArrayArray");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32ArrayArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "4 8 15 16 23 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToInt32ArrayArray");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_struct_array_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	const char *    str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StructArrayToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "Joe 34\nPaul 27");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StructArrayToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an array member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong array member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &arrayiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Paul";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a structure member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong struct member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "27";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra structure entry is given.
	 */
	TEST_FEATURE ("with extra member");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		str_value = "Male";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		str_value = "Male";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		str_value = "Jane";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StructArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_struct_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	char *          str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToStructArray_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStructArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe 34\nPaul 27";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_STRUCT_END_CHAR_AS_STRING));

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Paul");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 27);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStructArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToStructArray.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStructArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStructArray");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStructArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe 34\nPaul 27";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToStructArray");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_dict_entry_array_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	const char *    str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_DictEntryArrayToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "Joe 34\nPaul 27");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.DictEntryArrayToStr.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an array member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong array member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &arrayiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Paul";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a dictionary member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong dict entry member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "27";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&iter, &arrayiter);

		str_value = "Jane";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"DictEntryArrayToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_dict_entry_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	char *          str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToDictEntryArray_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDictEntryArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe 34\nPaul 27";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_DICT_ENTRY_END_CHAR_AS_STRING));

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Paul");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 27);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDictEntryArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToDictEntryArray.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDictEntryArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDictEntryArray");

		dbus_message_iter_init_append (method_call, &iter);

		uint32_value = 34;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDictEntryArray");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "Joe 34\nPaul 27";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToDictEntryArray");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_unix_fd_to_str (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	int             unix_fd_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	const char *    str_value;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_UnixFdToStr_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UnixFdToStr");

		dbus_message_iter_init_append (method_call, &iter);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_NE (str_value[0], '\0');
		TEST_TRUE (strchr ("0123456789", str_value[0]));

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UnixFdToStr");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UnixFdToStr");

		dbus_message_iter_init_append (method_call, &iter);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"UnixFdToStr");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_str_to_unix_fd (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	char *          str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	int             unix_fd_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_StrToUnixFd_method");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the function works as we expect when we give the
	 * expected argument type.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUnixFd");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "1";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply),
			     DBUS_TYPE_UNIX_FD_AS_STRING);

		dbus_message_iter_init (reply, &iter);

		dbus_message_iter_get_basic (&iter, &unix_fd_value);
		TEST_GT (unix_fd_value, 2); // duplicated by dbus

		dbus_message_unref (reply);
		close (unix_fd_value);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUnixFd");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrToUnixFd.EmptyInput"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUnixFd");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUnixFd");

		dbus_message_iter_init_append (method_call, &iter);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUnixFd");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "1";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			"com.netsplit.Nih.Test",
			"StrToUnixFd");

		dbus_message_iter_init_append (method_call, &iter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}




void
test_new_byte (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	uint8_t         byte_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_byte");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_byte (server_conn,
					     "/com/netsplit/Nih/Test",
					     97);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewByte"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_BYTE_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &byte_value);
		TEST_EQ (byte_value, 97);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_boolean (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int             boolean_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_boolean");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_boolean (server_conn,
						"/com/netsplit/Nih/Test",
						TRUE);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewBoolean"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_BOOLEAN_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &boolean_value);
		TEST_EQ (boolean_value, TRUE);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_int16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int16_t         int16_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_int16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_int16 (server_conn,
					      "/com/netsplit/Nih/Test",
					      -42);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt16"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_INT16_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &int16_value);
		TEST_EQ (int16_value, -42);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_uint16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	uint16_t        uint16_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_uint16");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_uint16 (server_conn,
					       "/com/netsplit/Nih/Test",
					       42);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewUInt16"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_UINT16_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &uint16_value);
		TEST_EQ (uint16_value, 42);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_int32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int32_t         int32_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_int32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_int32 (server_conn,
					      "/com/netsplit/Nih/Test",
					      -1048576);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt32"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_INT32_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, -1048576);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_uint32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	uint32_t        uint32_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_uint32");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_uint32 (server_conn,
					       "/com/netsplit/Nih/Test",
					       1048576);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewUInt32"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_UINT32_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &uint32_value);
		TEST_EQ (uint32_value, 1048576);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_int64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int64_t         int64_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_int64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_int64 (server_conn,
					      "/com/netsplit/Nih/Test",
					      -4815162342L);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt64"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_INT64_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &int64_value);
		TEST_EQ (int64_value, -4815162342L);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_uint64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	uint64_t        uint64_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_uint64");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_uint64 (server_conn,
					       "/com/netsplit/Nih/Test",
					       4815162342L);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewUInt64"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_UINT64_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &uint64_value);
		TEST_EQ (uint64_value, 4815162342L);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_double (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	double          double_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_double");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_double (server_conn,
					       "/com/netsplit/Nih/Test",
					       3.141597);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewDouble"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_DOUBLE_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &double_value);
		TEST_EQ (double_value, 3.141597);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_string (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	const char *    str_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_string");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_string (server_conn,
					       "/com/netsplit/Nih/Test",
					       "she needs more of ze punishment");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewString"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_STRING_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_object_path (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	const char *    str_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_object_path");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_object_path (server_conn,
						    "/com/netsplit/Nih/Test",
						    "/com/netsplit/Nih/Test");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewObjectPath"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_OBJECT_PATH_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_signature (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	const char *    str_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_signature");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_signature (server_conn,
						  "/com/netsplit/Nih/Test",
						  "a(ib)");

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewSignature"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_SIGNATURE_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "a(ib)");

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_struct (void)
{
	pid_t                dbus_pid;
	DBusConnection *     client_conn;
	DBusConnection *     server_conn;
	DBusError            dbus_error;
	int                  ret;
	DBusMessage *        signal;
	DBusMessageIter      iter;
	DBusMessageIter      subiter;
	MyTestNewStructValue struct_value;
	const char *         str_value;
	uint32_t             uint32_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_struct");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		struct_value.item0 = "Joe";
		struct_value.item1 = 34;

		ret = my_test_emit_new_struct (server_conn,
					       "/com/netsplit/Nih/Test",
					       &struct_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewStruct"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_STRUCT_END_CHAR_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_int32_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int32_t *       int32_array = NULL;
	size_t          int32_array_len;
	DBusMessageIter subiter;
	int32_t         int32_value;

	TEST_FUNCTION ("my_test_emit_new_int32_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			int32_array = nih_alloc (NULL, sizeof (int32_t) * 6);
			int32_array[0] = 4;
			int32_array[1] = 8;
			int32_array[2] = 15;
			int32_array[3] = 16;
			int32_array[4] = 23;
			int32_array[5] = 42;
			int32_array_len = 6;
		}

		ret = my_test_emit_new_int32_array (server_conn,
						    "/com/netsplit/Nih/Test",
						    int32_array,
						    int32_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);

			nih_free (int32_array);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt32Array"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT32_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 4);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 15);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 16);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 23);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, 42);
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		nih_free (int32_array);
	}


	/* Check that we can also given an empty array, which is actually
	 * the NULL pointer.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_int32_array (server_conn,
						    "/com/netsplit/Nih/Test",
						    NULL, 0);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);

			nih_free (int32_array);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt32Array"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT32_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_str_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	char **         str_array = NULL;
	DBusMessageIter subiter;
	const char *    str_value;

	TEST_FUNCTION ("my_test_emit_new_str_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			str_array = nih_alloc (NULL, sizeof (char *) * 7);
			str_array[0] = "she";
			str_array[1] = "needs";
			str_array[2] = "more";
			str_array[3] = "of";
			str_array[4] = "ze";
			str_array[5] = "punishment";
			str_array[6] = NULL;
		}

		ret = my_test_emit_new_str_array (server_conn,
						  "/com/netsplit/Nih/Test",
						  str_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);

			nih_free (str_array);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewStrArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "she");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "needs");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "more");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "of");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "ze");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "punishment");
		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		nih_free (str_array);
	}


	/* Check that we can give an empty array consisting of just the
	 * NULL pointer.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			str_array = nih_alloc (NULL, sizeof (char *) * 1);
			str_array[0] = NULL;
		}

		ret = my_test_emit_new_str_array (server_conn,
						  "/com/netsplit/Nih/Test",
						  str_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);

			nih_free (str_array);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewStrArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		nih_free (str_array);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_int32_array_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int32_t **      int32_array_array = NULL;
	size_t *        int32_array_array_len = NULL;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	int32_t         int32_value;

	TEST_FUNCTION ("my_test_emit_new_int32_array_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			int32_array_array = nih_alloc (NULL,
						       sizeof (int32_t *) * 3);
			int32_array_array_len = nih_alloc (NULL,
							   sizeof (size_t) * 2);

			int32_array_array[0] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[0][0] = 4;
			int32_array_array[0][1] = 8;
			int32_array_array[0][2] = 15;
			int32_array_array[0][3] = 16;
			int32_array_array[0][4] = 23;
			int32_array_array[0][5] = 42;
			int32_array_array_len[0] = 6;

			int32_array_array[1] = nih_alloc (int32_array_array,
							  sizeof (int32_t) * 6);
			int32_array_array[1][0] = 1;
			int32_array_array[1][1] = 1;
			int32_array_array[1][2] = 2;
			int32_array_array[1][3] = 3;
			int32_array_array[1][4] = 5;
			int32_array_array[1][5] = 8;
			int32_array_array_len[1] = 6;

			int32_array_array[2] = NULL;
		}

		ret = my_test_emit_new_int32_array_array (server_conn,
							  "/com/netsplit/Nih/Test",
							  int32_array_array,
							  int32_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);

			nih_free (int32_array_array);
			nih_free (int32_array_array_len);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt32ArrayArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT32_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 4);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 15);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 16);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 23);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 42);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 1);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 1);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 2);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 3);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 5);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		nih_free (int32_array_array);
		nih_free (int32_array_array_len);
	}


	/* Check that we can also give an empty array, which actually has
	 * NULL for its size array.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			int32_array_array = nih_alloc (NULL,
						       sizeof (int32_t *) * 1);
			int32_array_array_len = NULL;

			int32_array_array[0] = NULL;
		}

		ret = my_test_emit_new_int32_array_array (server_conn,
							  "/com/netsplit/Nih/Test",
							  int32_array_array,
							  int32_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);

			nih_free (int32_array_array);
			nih_free (int32_array_array_len);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewInt32ArrayArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT32_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		nih_free (int32_array_array);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_struct_array (void)
{
	pid_t                dbus_pid;
	DBusConnection *     client_conn;
	DBusConnection *     server_conn;
	DBusError            dbus_error;
	int                  ret;
	DBusMessage *        signal;
	DBusMessageIter      iter;
	DBusMessageIter      arrayiter;
	DBusMessageIter      subiter;
	MyTestNewStructArrayValueElement **struct_array = NULL;
	const char *         str_value;
	uint32_t             uint32_value;

	TEST_FUNCTION ("my_test_emit_new_struct_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			struct_array = nih_alloc (NULL, sizeof (MyTestNewStructArrayValueElement *) * 3);

			struct_array[0] = nih_new (struct_array, MyTestNewStructArrayValueElement);
			struct_array[0]->item0 = "Joe";
			struct_array[0]->item1 = 34;

			struct_array[1] = nih_new (struct_array, MyTestNewStructArrayValueElement);
			struct_array[1]->item0 = "Paul";
			struct_array[1]->item1 = 27;

			struct_array[2] = NULL;
		}

		ret = my_test_emit_new_struct_array (server_conn,
						     "/com/netsplit/Nih/Test",
						     struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			nih_free (struct_array);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewStructArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_STRUCT_END_CHAR_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);


		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Paul");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 27);


		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		nih_free (struct_array);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}


	/* Check that we can pass an empty array, which has NULL as the
	 * only element.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			struct_array = nih_alloc (NULL, sizeof (MyTestNewStructArrayValueElement *) * 1);

			struct_array[0] = NULL;
		}

		ret = my_test_emit_new_struct_array (server_conn,
						     "/com/netsplit/Nih/Test",
						     struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			nih_free (struct_array);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewStructArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_STRUCT_END_CHAR_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		nih_free (struct_array);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_new_dict_entry_array (void)
{
	pid_t                dbus_pid;
	DBusConnection *     client_conn;
	DBusConnection *     server_conn;
	DBusError            dbus_error;
	int                  ret;
	DBusMessage *        signal;
	DBusMessageIter      iter;
	DBusMessageIter      arrayiter;
	DBusMessageIter      subiter;
	MyTestNewDictEntryArrayValueElement **dict_entry_array = NULL;
	const char *         str_value;
	uint32_t             uint32_value;

	TEST_FUNCTION ("my_test_emit_new_dict_entry_array");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FEATURE ("with array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			dict_entry_array = nih_alloc (NULL, sizeof (MyTestNewDictEntryArrayValueElement *) * 3);

			dict_entry_array[0] = nih_new (dict_entry_array, MyTestNewDictEntryArrayValueElement);
			dict_entry_array[0]->item0 = "Joe";
			dict_entry_array[0]->item1 = 34;

			dict_entry_array[1] = nih_new (dict_entry_array, MyTestNewDictEntryArrayValueElement);
			dict_entry_array[1]->item0 = "Paul";
			dict_entry_array[1]->item1 = 27;

			dict_entry_array[2] = NULL;
		}

		ret = my_test_emit_new_dict_entry_array (server_conn,
						     "/com/netsplit/Nih/Test",
						     dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			nih_free (dict_entry_array);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewDictEntryArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_DICT_ENTRY_END_CHAR_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);


		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Paul");

		dbus_message_iter_next (&subiter);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 27);


		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		nih_free (dict_entry_array);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}


	/* Check that we can pass an empty array, which has NULL as the
	 * only element.
	 */
	TEST_FEATURE ("with empty array");
	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		TEST_ALLOC_SAFE {
			dict_entry_array = nih_alloc (NULL, sizeof (MyTestNewDictEntryArrayValueElement *) * 1);

			dict_entry_array[0] = NULL;
		}

		ret = my_test_emit_new_dict_entry_array (server_conn,
						     "/com/netsplit/Nih/Test",
						     dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			nih_free (dict_entry_array);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewDictEntryArray"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING
			      DBUS_TYPE_UINT32_AS_STRING
			      DBUS_DICT_ENTRY_END_CHAR_AS_STRING));

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (signal);

		nih_free (dict_entry_array);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_new_unix_fd (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	DBusError       dbus_error;
	int             ret;
	DBusMessage *   signal;
	DBusMessageIter iter;
	int             unix_fd_value;

	/* Check that the generated function emits the expected signal,
	 * with the arguments we give.
	 */
	TEST_FUNCTION ("my_test_emit_new_unix_fd");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);

	TEST_ALLOC_FAIL {
		dbus_error_init (&dbus_error);
		dbus_bus_add_match (client_conn, "type='signal'", &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);

		ret = my_test_emit_new_unix_fd (server_conn,
						"/com/netsplit/Nih/Test",
						1);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_error_init (&dbus_error);
			dbus_bus_remove_match (client_conn, "type='signal'",
					       &dbus_error);
			assert (! dbus_error_is_set (&dbus_error));
			dbus_error_free (&dbus_error);
			continue;
		}

		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, signal);
		TEST_TRUE (dbus_message_is_signal (
				   signal, "com.netsplit.Nih.Test", "NewUnixFd"));
		TEST_EQ_STR (dbus_message_get_signature (signal),
			     DBUS_TYPE_UNIX_FD_AS_STRING);

		dbus_message_iter_init (signal, &iter);

		dbus_message_iter_get_basic (&iter, &unix_fd_value);
		TEST_GT (unix_fd_value, 2); // dbus dups this

		dbus_message_unref (signal);
		close (unix_fd_value);

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (client_conn, "type='signal'",
				       &dbus_error);
		assert (! dbus_error_is_set (&dbus_error));
		dbus_error_free (&dbus_error);
	}

	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}




void
test_get_byte (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	uint8_t         byte_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_byte_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		byte_property = 97;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_BYTE);

		dbus_message_iter_get_basic (&subiter, &byte_value);
		TEST_EQ (byte_value, 97);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		byte_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Byte.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		byte_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BYTE_AS_STRING,
						  &subiter);

		byte_value = 97;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_byte (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	uint8_t         byte_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_byte_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		byte_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BYTE_AS_STRING,
						  &subiter);

		byte_value = 97;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (byte_property, 97);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BYTE_AS_STRING,
						  &subiter);

		byte_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Byte.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BYTE_AS_STRING,
						  &subiter);

		byte_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "97";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BYTE_AS_STRING,
						  &subiter);

		byte_value = 97;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BYTE,
						&byte_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "97";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "byte";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_boolean (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	int             boolean_value;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_boolean_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		boolean_property = TRUE;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_BOOLEAN);

		dbus_message_iter_get_basic (&subiter, &boolean_value);
		TEST_EQ (boolean_value, TRUE);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		boolean_property = FALSE;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Boolean.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BOOLEAN_AS_STRING,
						  &subiter);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_boolean (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	int             boolean_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_boolean_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		boolean_property = FALSE;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BOOLEAN_AS_STRING,
						  &subiter);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (boolean_property, TRUE);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BOOLEAN_AS_STRING,
						  &subiter);

		boolean_value = FALSE;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Boolean.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "True";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_BOOLEAN_AS_STRING,
						  &subiter);

		boolean_value = TRUE;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_BOOLEAN,
						&boolean_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_int16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	int16_t         int16_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int16_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int16_property = -42;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, -42);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int16_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int16.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int16_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT16_AS_STRING,
						  &subiter);

		int16_value = -42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	int16_t         int16_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int16_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int16_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT16_AS_STRING,
						  &subiter);

		int16_value = -42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (int16_property, -42);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT16_AS_STRING,
						  &subiter);

		int16_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int16.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT16_AS_STRING,
						  &subiter);

		int16_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "-42";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "-42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT16_AS_STRING,
						  &subiter);

		int16_value = -42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
						&int16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "-42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_uint16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	uint16_t        uint16_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_uint16_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint16_property = 42;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT16);

		dbus_message_iter_get_basic (&subiter, &uint16_value);
		TEST_EQ (uint16_value, 42);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint16_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt16.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint16_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT16_AS_STRING,
						  &subiter);

		uint16_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_uint16 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	uint16_t        uint16_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_uint16_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint16_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT16_AS_STRING,
						  &subiter);

		uint16_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (uint16_property, 42);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT16_AS_STRING,
						  &subiter);

		uint16_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt16.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT16_AS_STRING,
						  &subiter);

		uint16_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "42";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT16_AS_STRING,
						  &subiter);

		uint16_value = 42;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT16,
						&uint16_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint16";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_int32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	int32_t         int32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int32_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_property = -1048576;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT32);

		dbus_message_iter_get_basic (&subiter, &int32_value);
		TEST_EQ (int32_value, -1048576);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	int32_t         int32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int32_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (int32_property, -1048576);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subiter);

		int32_value = -1048576;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "-1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_uint32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	uint32_t        uint32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_uint32_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint32_property = 1048576;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 1048576);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint32_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt32.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint32_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &subiter);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_uint32 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_uint32_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint32_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &subiter);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (uint32_property, 1048576);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &subiter);

		uint32_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt32.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &subiter);

		uint32_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "1048576";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT32_AS_STRING,
						  &subiter);

		uint32_value = 1048576;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "1048576";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint32";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_int64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	int64_t         int64_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int16_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int64_property = -4815162342L;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT64);

		dbus_message_iter_get_basic (&subiter, &int64_value);
		TEST_EQ (int64_value, -4815162342L);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int64_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int64.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int64_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT64_AS_STRING,
						  &subiter);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	int64_t         int64_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int64_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int64_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT64_AS_STRING,
						  &subiter);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (int64_property, -4815162342L);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT64_AS_STRING,
						  &subiter);

		int64_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int64.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT64_AS_STRING,
						  &subiter);

		int64_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_INT64_AS_STRING,
						  &subiter);

		int64_value = -4815162342L;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT64,
						&int64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "-4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_uint64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	uint64_t        uint64_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_uint64_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint64_property = 4815162342L;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT64);

		dbus_message_iter_get_basic (&subiter, &uint64_value);
		TEST_EQ (uint64_value, 4815162342L);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint64_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt64.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint64_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT64_AS_STRING,
						  &subiter);

		uint64_value = 4815162342L;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_uint64 (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	uint64_t        uint64_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_uint64_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		uint64_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT64_AS_STRING,
						  &subiter);

		uint64_value = 4815162342L;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (uint64_property, 4815162342L);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT64_AS_STRING,
						  &subiter);

		uint64_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UInt64.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT64_AS_STRING,
						  &subiter);

		uint64_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UINT64_AS_STRING,
						  &subiter);

		uint64_value = 4815162342L;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT64,
						&uint64_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "4815162342";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "uint64";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_double (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	double          double_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_double_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		double_property = 3.141597;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_DOUBLE);

		dbus_message_iter_get_basic (&subiter, &double_value);
		TEST_EQ (double_value, 3.141597);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		double_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Double.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		double_property = 4;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_double (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	double          double_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_double_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		double_property = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (double_property, 3.141597);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_value = 0;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Double.Zero"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_value = 4;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "97";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "boolean";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "3.141597";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_DOUBLE_AS_STRING,
						  &subiter);

		double_value = 3.141597;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
						&double_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "3.141597";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "double";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_string (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_string_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		str_property = "she needs more of ze punishment";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "she needs more of ze punishment");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		str_property = "";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.String.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		str_property = "invalid";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_string (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_string_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		str_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ_STR (str_property, "she needs more of ze punishment");
		nih_free (str_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.String.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/she/needs/more/of/ze/punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "/she/needs/more/of/ze/punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "string";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_object_path (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_object_path_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		object_path_property = "/com/netsplit/Nih/Test";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_OBJECT_PATH);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		object_path_property = "/";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.ObjectPath.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		object_path_property = "/invalid";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_object_path (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_object_path_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		object_path_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ_STR (object_path_property, "/com/netsplit/Nih/Test");
		nih_free (object_path_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.ObjectPath.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/invalid";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subiter);

		str_value = "/com/netsplit/Nih/Test";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "object_path";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_signature (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_signature_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		signature_property = "a(ib)";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_SIGNATURE);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "a(ib)");

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		signature_property = "";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Signature.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		signature_property = "inva(x)id";

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_SIGNATURE_AS_STRING,
						  &subiter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_signature (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_signature_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		signature_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_SIGNATURE_AS_STRING,
						  &subiter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ_STR (signature_property, "a(ib)");
		nih_free (signature_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_SIGNATURE_AS_STRING,
						  &subiter);

		str_value = "";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Signature.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_SIGNATURE_AS_STRING,
						  &subiter);

		str_value = "inva(x)id";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_SIGNATURE_AS_STRING,
						  &subiter);

		str_value = "a(ib)";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_SIGNATURE,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "signature";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_structure (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter variter;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_structure_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			struct_property = nih_new (NULL, MyStruct);
			struct_property->item0 = "Joe";
			struct_property->item1 = 34;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &variter);

		TEST_EQ (dbus_message_iter_get_arg_type (&variter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&variter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (struct_property);
		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			struct_property = nih_new (NULL, MyStruct);
			struct_property->item0 = "";
			struct_property->item1 = 34;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Structure.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (struct_property);
		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			struct_property = nih_new (NULL, MyStruct);
			struct_property->item0 = "invalid";
			struct_property->item1 = 34;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (struct_property);
		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_structure (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	uint32_t        uint32_value;
	DBusMessageIter variter;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_structure_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		struct_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&variter, &subiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_NE_P (struct_property, NULL);
		TEST_ALLOC_SIZE (struct_property, sizeof (MyStruct));
		TEST_EQ_STR (struct_property->item0, "Joe");
		TEST_EQ (struct_property->item1, 34);

		nih_free (struct_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&variter, &subiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Structure.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "invalid";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&variter, &subiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a structure member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&variter, &subiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Joe";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&variter, &subiter);

		dbus_message_iter_close_container (&iter, &variter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "structure";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_int32_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	int32_t         int32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int32_array_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			int32_array_property = nih_alloc (NULL, sizeof (int32_t) * 6);
			int32_array_property[0] = 4;
			int32_array_property[1] = 8;
			int32_array_property[2] = 15;
			int32_array_property[3] = 16;
			int32_array_property[4] = 23;
			int32_array_property[5] = 42;
			int32_array_property_len = 6;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 4);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 15);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 16);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 23);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubiter, &int32_value);
		TEST_EQ (int32_value, 42);
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (int32_array_property);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_array_property = NULL;
		int32_array_property_len = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32Array.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			int32_array_property = nih_alloc (NULL, sizeof (int32_t) * 4);
			int32_array_property[0] = 4;
			int32_array_property[1] = 8;
			int32_array_property[2] = 15;
			int32_array_property[3] = 16;

			int32_array_property_len = 4;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (int32_array_property);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int32_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	int32_t         int32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int32_array_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_array_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_EQ (int32_array_property_len, 6);
		TEST_ALLOC_SIZE (int32_array_property, sizeof (int32_t) * 6);
		TEST_EQ (int32_array_property[0], 4);
		TEST_EQ (int32_array_property[1], 8);
		TEST_EQ (int32_array_property[2], 15);
		TEST_EQ (int32_array_property[3], 16);
		TEST_EQ (int32_array_property[4], 23);
		TEST_EQ (int32_array_property[5], 42);
		nih_free (int32_array_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32Array.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an array element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong array element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubiter);

		str_value = "4";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "8";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "15";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "16";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "23";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong variant element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "4 8 15 16 32 42";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "4 8 15 16 23 42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "4 8 15 16 23 42";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_str_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_str_array_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			str_array_property = nih_alloc (NULL, sizeof (char *) * 7);
			str_array_property[0] = "she";
			str_array_property[1] = "needs";
			str_array_property[2] = "more";
			str_array_property[3] = "of";
			str_array_property[4] = "ze";
			str_array_property[5] = "punishment";
			str_array_property[6] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "she");
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "needs");
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "more");
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "of");
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "ze");
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "punishment");
		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (str_array_property);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			str_array_property = nih_alloc (NULL, sizeof (char *) * 1);
			str_array_property[0] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (str_array_property);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			str_array_property = nih_alloc (NULL, sizeof (char *) * 5);
			str_array_property[0] = "this";
			str_array_property[1] = "is";
			str_array_property[2] = "a";
			str_array_property[3] = "test";
			str_array_property[4] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (str_array_property);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "the";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_str_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_str_array_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		str_array_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "ze";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_ALLOC_SIZE (str_array_property, sizeof (char *) * 7);
		TEST_EQ_STR (str_array_property[0], "she");
		TEST_EQ_STR (str_array_property[1], "needs");
		TEST_EQ_STR (str_array_property[2], "more");
		TEST_EQ_STR (str_array_property[3], "of");
		TEST_EQ_STR (str_array_property[4], "ze");
		TEST_EQ_STR (str_array_property[5], "punishment");
		TEST_EQ_P (str_array_property[6], NULL);

		nih_free (str_array_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StrArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubiter);

		str_value = "this";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "is";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "a";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "test";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an array element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong array element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_OBJECT_PATH_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_OBJECT_PATH_AS_STRING,
						  &subsubiter);

		str_value = "/she";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/needs";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/more";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/of";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/ze";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		str_value = "/punishment";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_OBJECT_PATH,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong variant element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubiter);

		str_value = "she";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "needs";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "more";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "of";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "ze";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "punishment";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "str_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_int32_array_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	DBusMessageIter subsubsubiter;
	int32_t         int32_value;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int32_array_array_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			int32_array_array_property = nih_alloc (
				NULL, sizeof (int32_t *) * 3);
			int32_array_array_property_len = nih_alloc (
				NULL, sizeof (size_t) * 2);

			int32_array_array_property[0] = nih_alloc (
				int32_array_array_property,
				sizeof (int32_t) * 6);
			int32_array_array_property[0][0] = 4;
			int32_array_array_property[0][1] = 8;
			int32_array_array_property[0][2] = 15;
			int32_array_array_property[0][3] = 16;
			int32_array_array_property[0][4] = 23;
			int32_array_array_property[0][5] = 42;
			int32_array_array_property_len[0] = 6;

			int32_array_array_property[1] = nih_alloc (
				int32_array_array_property,
				sizeof (int32_t) * 6);
			int32_array_array_property[1][0] = 1;
			int32_array_array_property[1][1] = 1;
			int32_array_array_property[1][2] = 2;
			int32_array_array_property[1][3] = 3;
			int32_array_array_property[1][4] = 5;
			int32_array_array_property[1][5] = 8;
			int32_array_array_property_len[1] = 6;

			int32_array_array_property[2] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subsubiter, &subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 4);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 15);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 16);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 23);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 42);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subsubiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subsubiter, &subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 1);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 1);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 2);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 3);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 5);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INT32);
		dbus_message_iter_get_basic (&subsubsubiter, &int32_value);
		TEST_EQ (int32_value, 8);
		dbus_message_iter_next (&subsubsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subsubiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (int32_array_array_property);
		nih_free (int32_array_array_property_len);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			int32_array_array_property = nih_alloc (
				NULL, sizeof (int32_t *) * 1);
			int32_array_array_property_len = NULL;

			int32_array_array_property[0] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32ArrayArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (int32_array_array_property);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			int32_array_array_property = nih_alloc (
				NULL, sizeof (int32_t *) * 2);
			int32_array_array_property_len = nih_alloc (
				NULL, sizeof (size_t) * 1);

			int32_array_array_property[0] = nih_alloc (
				int32_array_array_property,
				sizeof (int32_t) * 6);
			int32_array_array_property[0][0] = 4;
			int32_array_array_property[0][1] = 8;
			int32_array_array_property[0][2] = 15;
			int32_array_array_property[0][3] = 16;
			int32_array_array_property[0][4] = 23;
			int32_array_array_property[0][5] = 42;
			int32_array_array_property_len[0] = 6;

			int32_array_array_property[1] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
		nih_free (int32_array_array_property);
		nih_free (int32_array_array_property_len);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_int32_array_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	DBusMessageIter subsubiter;
	DBusMessageIter subsubsubiter;
	int32_t         int32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_int32_array_array_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		int32_array_array_property = NULL;
		int32_array_array_property_len = 0;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_ALLOC_SIZE (int32_array_array_property_len, sizeof (size_t) * 2);
		TEST_ALLOC_SIZE (int32_array_array_property, sizeof (int32_t *) * 3);

		TEST_EQ (int32_array_array_property_len[0], 6);
		TEST_ALLOC_SIZE (int32_array_array_property[0], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array_property[0],
				   int32_array_array_property);
		TEST_EQ (int32_array_array_property[0][0], 4);
		TEST_EQ (int32_array_array_property[0][1], 8);
		TEST_EQ (int32_array_array_property[0][2], 15);
		TEST_EQ (int32_array_array_property[0][3], 16);
		TEST_EQ (int32_array_array_property[0][4], 23);
		TEST_EQ (int32_array_array_property[0][5], 42);

		TEST_EQ (int32_array_array_property_len[1], 6);
		TEST_ALLOC_SIZE (int32_array_array_property[1], sizeof (int32_t) * 6);
		TEST_ALLOC_PARENT (int32_array_array_property[0],
				   int32_array_array_property);
		TEST_EQ (int32_array_array_property[1][0], 1);
		TEST_EQ (int32_array_array_property[1][1], 1);
		TEST_EQ (int32_array_array_property[1][2], 2);
		TEST_EQ (int32_array_array_property[1][3], 3);
		TEST_EQ (int32_array_array_property[1][4], 5);
		TEST_EQ (int32_array_array_property[1][5], 8);

		TEST_EQ_P (int32_array_array_property[2], NULL);

		nih_free (int32_array_array_property);
		nih_free (int32_array_array_property_len);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.Int32ArrayArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a nested array element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong nested array element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subsubiter);


		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubsubiter);

		str_value = "4";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "8";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "15";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "16";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "23";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);


		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubsubiter);

		str_value = "1";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "1";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "2";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "3";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "5";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "6";
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);


		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a top array element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong top array element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subsubiter);

		str_value = "4";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "8";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "15";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "16";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "23";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "42";
		dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong variant element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "4 8 15 16 32 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "4 8 15 16 23 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subiter);

		dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_INT32_AS_STRING),
						  &subsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 4;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 15;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 16;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 23;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 42;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_open_container (&subsubiter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_INT32_AS_STRING,
						  &subsubsubiter);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 1;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 2;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 3;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 5;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		int32_value = 8;
		dbus_message_iter_append_basic (&subsubsubiter, DBUS_TYPE_INT32,
						&int32_value);

		dbus_message_iter_close_container (&subsubiter, &subsubsubiter);

		dbus_message_iter_close_container (&subiter, &subsubiter);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "4 8 15 16 23 42\n1 1 2 3 5 8";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "int32_array_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_struct_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter variter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_struct_array_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			struct_array_property = nih_alloc (NULL, sizeof (MyStruct *) * 3);

			struct_array_property[0] = nih_new (struct_array_property, MyStruct);
			struct_array_property[0]->item0 = "Joe";
			struct_array_property[0]->item1 = 34;

			struct_array_property[1] = nih_new (struct_array_property, MyStruct);
			struct_array_property[1]->item0 = "Paul";
			struct_array_property[1]->item1 = 27;

			struct_array_property[2] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &variter);

		TEST_EQ (dbus_message_iter_get_arg_type (&variter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&variter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);


		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Paul");


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 27);


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (struct_array_property);
		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			struct_array_property = nih_alloc (NULL, sizeof (MyStruct *) * 1);

			struct_array_property[0] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StructArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (struct_array_property);
		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			struct_array_property = nih_alloc (NULL, sizeof (MyStruct *) * 2);

			struct_array_property[0] = nih_new (struct_array_property, MyStruct);
			struct_array_property[0]->item0 = "Joe";
			struct_array_property[0]->item1 = 34;

			struct_array_property[1] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (struct_array_property);
		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_struct_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	uint32_t        uint32_value;
	DBusMessageIter variter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_struct_array_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		struct_array_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_NE_P (struct_array_property, NULL);
		TEST_ALLOC_SIZE (struct_array_property, sizeof (MyStruct *) * 3);

		TEST_ALLOC_PARENT (struct_array_property[0], struct_array_property);
		TEST_ALLOC_SIZE (struct_array_property[0], sizeof (MyStruct));
		TEST_EQ_STR (struct_array_property[0]->item0, "Joe");
		TEST_EQ (struct_array_property[0]->item1, 34);

		TEST_ALLOC_PARENT (struct_array_property[1], struct_array_property);
		TEST_ALLOC_SIZE (struct_array_property[1], sizeof (MyStruct));
		TEST_EQ_STR (struct_array_property[1]->item0, "Paul");
		TEST_EQ (struct_array_property[1]->item1, 27);

		TEST_EQ_P (struct_array_property[2], NULL);

		nih_free (struct_array_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.StructArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a structure member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "27";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an array element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong array element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &arrayiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Paul";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong variant element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Joe";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_STRUCT_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_STRUCT,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "struct_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_dict_entry_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	uint32_t        uint32_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter variter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_dict_entry_array_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			dict_entry_array_property = nih_alloc (NULL, sizeof (MyStruct *) * 3);

			dict_entry_array_property[0] = nih_new (dict_entry_array_property, MyStruct);
			dict_entry_array_property[0]->item0 = "Joe";
			dict_entry_array_property[0]->item1 = 34;

			dict_entry_array_property[1] = nih_new (dict_entry_array_property, MyStruct);
			dict_entry_array_property[1]->item0 = "Paul";
			dict_entry_array_property[1]->item1 = 27;

			dict_entry_array_property[2] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &variter);

		TEST_EQ (dbus_message_iter_get_arg_type (&variter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&variter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Joe");


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);


		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&arrayiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "Paul");


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 27);


		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);

		nih_free (dict_entry_array_property);
		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			dict_entry_array_property = nih_alloc (NULL, sizeof (MyStruct *) * 1);

			dict_entry_array_property[0] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.DictEntryArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (dict_entry_array_property);
		nih_free (object);
	}


	/* Check that a generic error may be returned from the property
	 * handler, and is returned as a D-Bus "failed" error reply to
	 * the message.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);

			dict_entry_array_property = nih_alloc (NULL, sizeof (MyStruct *) * 2);

			dict_entry_array_property[0] = nih_new (dict_entry_array_property, MyStruct);
			dict_entry_array_property[0]->item0 = "Joe";
			dict_entry_array_property[0]->item1 = 34;

			dict_entry_array_property[1] = NULL;
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (dict_entry_array_property);
		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_dict_entry_array (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	uint32_t        uint32_value;
	DBusMessageIter variter;
	DBusMessageIter arrayiter;
	DBusMessageIter subiter;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusError       dbus_error;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_dict_entry_array_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		dict_entry_array_property = NULL;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_NE_P (dict_entry_array_property, NULL);
		TEST_ALLOC_SIZE (dict_entry_array_property, sizeof (MyStruct *) * 3);

		TEST_ALLOC_PARENT (dict_entry_array_property[0], dict_entry_array_property);
		TEST_ALLOC_SIZE (dict_entry_array_property[0], sizeof (MyStruct));
		TEST_EQ_STR (dict_entry_array_property[0]->item0, "Joe");
		TEST_EQ (dict_entry_array_property[0]->item1, 34);

		TEST_ALLOC_PARENT (dict_entry_array_property[1], dict_entry_array_property);
		TEST_ALLOC_SIZE (dict_entry_array_property[1], sizeof (MyStruct));
		TEST_EQ_STR (dict_entry_array_property[1]->item0, "Paul");
		TEST_EQ (dict_entry_array_property[1]->item1, 27);

		TEST_EQ_P (dict_entry_array_property[2], NULL);

		nih_free (dict_entry_array_property);

		nih_free (object);
	}


	/* Check that a D-Bus error raised from the function is returned
	 * as an error return of the same name and message.
	 */
	TEST_FEATURE ("with invalid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.DictEntryArray.Empty"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that a non-D-Bus error raised from the function is
	 * returned as the generic D-Bus "failed" error to the user,
	 * with the message copied across.
	 */
	TEST_FEATURE ("with generic error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, "Invalid argument");
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a structure member of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong member type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "34";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "27";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an array element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong array element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  DBUS_TYPE_STRING_AS_STRING,
						  &arrayiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Paul";
		dbus_message_iter_append_basic (&arrayiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong variant element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &variter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&variter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &variter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "Joe";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  (DBUS_TYPE_ARRAY_AS_STRING
						   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &variter);

		dbus_message_iter_open_container (&variter, DBUS_TYPE_ARRAY,
						  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						   DBUS_TYPE_STRING_AS_STRING
						   DBUS_TYPE_UINT32_AS_STRING
						   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						  &arrayiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Joe";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 34;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_open_container (&arrayiter, DBUS_TYPE_DICT_ENTRY,
						  NULL, &subiter);

		str_value = "Paul";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		uint32_value = 27;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
						&uint32_value);

		dbus_message_iter_close_container (&arrayiter, &subiter);

		dbus_message_iter_close_container (&variter, &arrayiter);

		dbus_message_iter_close_container (&iter, &variter);

		str_value = "she needs more of ze punishment";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "dict_entry_array";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_get_unix_fd (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;
	DBusMessageIter subiter;
	int             unix_fd_value;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_unix_fd_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can obtain the current value of a property,
	 * marshalled through the Get call, and that it's returned inside
	 * a variant.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		unix_fd_property = 1;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_VARIANT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UNIX_FD);

		dbus_message_iter_get_basic (&subiter, &unix_fd_value);
		TEST_GT (unix_fd_value, 2); // dbus dups it

		dbus_message_unref (reply);
		close (unix_fd_value);

		nih_free (object);
	}


	/* Check that a D-Bus error may be returned from the property
	 * handler, and is returned as a D-Bus error reply to the message.
	 */
	TEST_FEATURE ("with D-Bus error");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		unix_fd_property = -1;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.Test.UnixFd.Invalid"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Get");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UNIX_FD_AS_STRING,
						  &subiter);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_set_unix_fd (void)
{
	pid_t           dbus_pid;
	DBusConnection *client_conn;
	DBusConnection *server_conn;
	NihDBusObject * object = NULL;
	DBusMessage *   method_call;
	DBusMessageIter iter;
	const char *    str_value;
	DBusMessageIter subiter;
	int             unix_fd_value;
	dbus_uint32_t   serial;
	DBusMessage *   reply;

	TEST_FUNCTION ("my_com_netsplit_Nih_Test_unix_fd_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (client_conn);
	TEST_DBUS_OPEN (server_conn);


	/* Check that we can set a new value of a property, marshalled
	 * through the Set call, and that an empty reply is returned.
	 */
	TEST_FEATURE ("with valid argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		unix_fd_property = -1;

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UNIX_FD_AS_STRING,
						  &subiter);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);
		TEST_EQ_STR (dbus_message_get_signature (reply), "");

		dbus_message_unref (reply);

		TEST_GT (unix_fd_property, 2); // dbus dups it

		close (unix_fd_property);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if a variant element of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong element type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_STRING_AS_STRING,
						  &subiter);

		str_value = "True";
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_close_container (&iter, &subiter);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an argument of the wrong type is given.
	 */
	TEST_FEATURE ("with wrong argument type");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if an extra argument is given.
	 */
	TEST_FEATURE ("with extra argument");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
						  DBUS_TYPE_UNIX_FD_AS_STRING,
						  &subiter);

		unix_fd_value = 1;
		dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UNIX_FD,
						&unix_fd_value);

		dbus_message_iter_close_container (&iter, &subiter);

		str_value = "True";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	/* Check that the function returns an invalid arguments error
	 * if no arguments are given.
	 */
	TEST_FEATURE ("with missing arguments");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			object = nih_dbus_object_new (NULL, server_conn,
						      "/com/netsplit/Nih/Test",
						      my_interfaces,
						      NULL);
		}

		method_call = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih/Test",
			DBUS_INTERFACE_PROPERTIES,
			"Set");

		dbus_message_iter_init_append (method_call, &iter);

		str_value = "com.netsplit.Nih.Test";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		str_value = "unix_fd";
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
						&str_value);

		dbus_connection_send (client_conn, method_call, &serial);
		dbus_connection_flush (client_conn);
		dbus_message_unref (method_call);

		TEST_DBUS_DISPATCH (server_conn);
		dbus_connection_flush (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);
		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);

		nih_free (object);
	}


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}




int
main (int   argc,
      char *argv[])
{
	nih_error_init ();

	test_ordinary_method ();
	test_nameless_method ();
	test_async_method ();

	test_byte_to_str ();
	test_str_to_byte ();

	test_boolean_to_str ();
	test_str_to_boolean ();

	test_int16_to_str ();
	test_str_to_int16 ();

	test_uint16_to_str ();
	test_str_to_uint16 ();

	test_int32_to_str ();
	test_str_to_int32 ();

	test_uint32_to_str ();
	test_str_to_uint32 ();

	test_int64_to_str ();
	test_str_to_int64 ();

	test_uint64_to_str ();
	test_str_to_uint64 ();

	test_double_to_str ();
	test_str_to_double ();

	test_object_path_to_str ();
	test_str_to_object_path ();

	test_signature_to_str ();
	test_str_to_signature ();

	test_struct_to_str ();
	test_str_to_struct ();

	test_int32_array_to_str ();
	test_str_to_int32_array ();

	test_str_array_to_str ();
	test_str_to_str_array ();

	test_int32_array_array_to_str ();
	test_str_to_int32_array_array ();

	test_struct_array_to_str ();
	test_str_to_struct_array ();

	test_dict_entry_array_to_str ();
	test_str_to_dict_entry_array ();

	test_unix_fd_to_str ();
	test_str_to_unix_fd ();

	test_new_byte ();
	test_new_boolean ();
	test_new_int16 ();
	test_new_uint16 ();
	test_new_int32 ();
	test_new_uint32 ();
	test_new_int64 ();
	test_new_uint64 ();
	test_new_double ();
	test_new_string ();
	test_new_object_path ();
	test_new_signature ();
	test_new_struct ();
	test_new_int32_array ();
	test_new_str_array ();
	test_new_int32_array_array ();
	test_new_struct_array ();
	test_new_dict_entry_array ();
	test_new_unix_fd ();

	test_get_byte ();
	test_set_byte ();

	test_get_boolean ();
	test_set_boolean ();

	test_get_int16 ();
	test_set_int16 ();

	test_get_uint16 ();
	test_set_uint16 ();

	test_get_int32 ();
	test_set_int32 ();

	test_get_uint32 ();
	test_set_uint32 ();

	test_get_int64 ();
	test_set_int64 ();

	test_get_uint64 ();
	test_set_uint64 ();

	test_get_double ();
	test_set_double ();

	test_get_string ();
	test_set_string ();

	test_get_object_path ();
	test_set_object_path ();

	test_get_signature ();
	test_set_signature ();

	test_get_structure ();
	test_set_structure ();

	test_get_int32_array ();
	test_set_int32_array ();

	test_get_str_array ();
	test_set_str_array ();

	test_get_int32_array_array ();
	test_set_int32_array_array ();

	test_get_struct_array ();
	test_set_struct_array ();

	test_get_dict_entry_array ();
	test_set_dict_entry_array ();

	test_get_unix_fd ();
	test_set_unix_fd ();

	return 0;
}
