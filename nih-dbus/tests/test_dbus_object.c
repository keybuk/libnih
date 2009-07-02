/* libnih
 *
 * test_dbus_object.c - test suite for nih-dbus/dbus_object.c
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

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_connection.h>
#include <nih-dbus/dbus_object.h>


static int             foo_called = FALSE;
static NihDBusObject * last_object = NULL;
static NihDBusMessage *last_message = NULL;
static DBusConnection *last_message_conn = NULL;

static DBusHandlerResult
foo_handler (NihDBusObject * object,
	     NihDBusMessage *message)
{
	foo_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	TEST_FREE_TAG (message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static int bar_decline = FALSE;
static int bar_called = FALSE;

static DBusHandlerResult
bar_handler (NihDBusObject * object,
	     NihDBusMessage *message)
{
	bar_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	if (bar_decline)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	TEST_FREE_TAG (message);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static const char *colour;
static int colour_get_called = FALSE;
static int colour_set_called = FALSE;

static int
colour_get (NihDBusObject *  object,
	    NihDBusMessage * message,
	    DBusMessageIter *iter)
{
	DBusMessageIter subiter;

	colour_get_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	TEST_FREE_TAG (message);

	if (! strcmp (colour, "secret")) {
		nih_dbus_error_raise (DBUS_ERROR_ACCESS_DENIED,
				      "Access denied");
		return -1;
	} else if (! strcmp (colour, "chicken")) {
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;
	}

	if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT,
						DBUS_TYPE_STRING_AS_STRING,
						&subiter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
					      &colour)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! dbus_message_iter_close_container (iter, &subiter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	return 0;
}

static int
colour_set (NihDBusObject *  object,
	    NihDBusMessage * message,
	    DBusMessageIter *iter)
{
	DBusMessageIter subiter;
	const char *    value;
	nih_local char *dup = NULL;

	colour_set_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	TEST_FREE_TAG (message);

	TEST_EQ (dbus_message_iter_get_arg_type (iter),
		 DBUS_TYPE_VARIANT);

	dbus_message_iter_recurse (iter, &subiter);

	TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
		 DBUS_TYPE_STRING);

	dbus_message_iter_get_basic (&subiter, &value);

	if (! strcmp (value, "pig")) {
		nih_dbus_error_raise ("com.netsplit.Nih.NotAColour",
				      "pig is not a colour");
		return -1;

	} else if (! strlen (value)) {
		nih_error_raise (EBADF, strerror (EBADF));
		return -1;

	} else {
		TEST_EQ_STR (value, "red");

		dup = nih_strdup (NULL, value);
		if (! dup) {
			nih_error_raise_no_memory ();
			return -1;
		}
	}

	return 0;
}

static int size_get_called = FALSE;

static int
size_get (NihDBusObject *  object,
	  NihDBusMessage * message,
	  DBusMessageIter *iter)
{
	DBusMessageIter subiter;
	dbus_uint32_t   uint32_value;

	size_get_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	TEST_FREE_TAG (message);

	if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT,
						DBUS_TYPE_UINT32_AS_STRING,
						&subiter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	uint32_value = 34;
	if (! dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
					      &uint32_value)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! dbus_message_iter_close_container (iter, &subiter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	return 0;
}

static int other_get_called = FALSE;

static int
other_get (NihDBusObject *  object,
	   NihDBusMessage * message,
	   DBusMessageIter *iter)
{
	DBusMessageIter subiter;
	dbus_uint32_t   uint32_value;

	other_get_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	TEST_FREE_TAG (message);

	if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT,
						DBUS_TYPE_UINT32_AS_STRING,
						&subiter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	uint32_value = 186;
	if (! dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
					      &uint32_value)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	if (! dbus_message_iter_close_container (iter, &subiter)) {
		nih_error_raise_no_memory ();
		return -1;
	}

	return 0;
}

static int poke_set_called = FALSE;

static int
poke_set (NihDBusObject *  object,
	  NihDBusMessage * message,
	  DBusMessageIter *iter)
{
	poke_set_called = TRUE;
	last_object = object;
	last_message = message;
	last_message_conn = message->connection;

	TEST_FREE_TAG (message);

	return 0;
}

static const NihDBusArg foo_args[] = {
	{ "str", "s", NIH_DBUS_ARG_IN },
	{ "len", "u", NIH_DBUS_ARG_IN },
	{ "count", "u", NIH_DBUS_ARG_OUT },
	{ NULL }
};

static const NihDBusArg bar_args[] = {
	{ NULL, "d", NIH_DBUS_ARG_IN },
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
	{ "Foo", foo_args, foo_handler },
	{ "Bar", bar_args, bar_handler },
	{ NULL }
};

static const NihDBusSignal interface_a_signals[] = {
	{ "Alert", signal_args },
	{ "Panic", signal_args },
	{ NULL }
};

static const NihDBusMethod interface_b_methods[] = {
	{ "Bar", bar_args, foo_handler },
	{ "Baz", baz_args, foo_handler },
	{ NULL }
};

static const NihDBusProperty interface_b_props[] = {
	{ "Colour", "s", NIH_DBUS_READWRITE, colour_get, colour_set },
	{ "Size",   "u", NIH_DBUS_READ,      size_get,   NULL },
	{ "Poke",   "d", NIH_DBUS_WRITE,     NULL,       poke_set },
	{ NULL }
};

static const NihDBusProperty interface_c_props[] = {
	{ "Colour", "u", NIH_DBUS_READWRITE, other_get, poke_set },
	{ "Height", "u", NIH_DBUS_READ,      other_get, NULL },
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

static const NihDBusInterface interface_c = {
	"Nih.TestC",
	NULL,
	NULL,
	interface_c_props
};

static const NihDBusInterface *no_interfaces[] = {
	NULL
};

static const NihDBusInterface *one_interface[] = {
	&interface_a,
	NULL
};

static const NihDBusInterface *prop_interface[] = {
	&interface_b,
	NULL
};

static const NihDBusInterface *all_interfaces[] = {
	&interface_a,
	&interface_b,
	&interface_c,
	NULL
};

void
test_object_new (void)
{
	pid_t           dbus_pid;
	DBusConnection *conn;
	NihDBusObject * object;

	/* Check that we can register a new object, having the filled in
	 * structure returned for us with the object registered against
	 * the connection at the right path.
	 */
	TEST_FUNCTION ("nih_dbus_object_new");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (conn);

	TEST_ALLOC_FAIL {
		void *data;

		object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
					      all_interfaces, &object);

		if (test_alloc_failed) {
			TEST_EQ_P (object, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (object, sizeof (NihDBusObject));

		TEST_ALLOC_PARENT (object->path, object);
		TEST_EQ_STR (object->path, "/com/netsplit/Nih");

		TEST_EQ_P (object->connection, conn);
		TEST_EQ_P (object->data, &object);
		TEST_EQ_P (object->interfaces, all_interfaces);
		TEST_EQ (object->registered, TRUE);

		TEST_TRUE (dbus_connection_get_object_path_data (
				   conn, "/com/netsplit/Nih", &data));
		TEST_EQ_P (data, object);

		nih_free (object);
	}

	TEST_DBUS_CLOSE (conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_object_destroy (void)
{
	pid_t           dbus_pid;
	DBusConnection *conn;
	NihDBusObject * object;
	void *          data;

	/* Check that a registered D-Bus object is unregistered from the
	 * bus when it is destroyed.
	 */
	TEST_FUNCTION ("nih_dbus_object_destroy");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (conn);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      all_interfaces, &object);
	assert (object != NULL);
	assert (dbus_connection_get_object_path_data (
			conn, "/com/netsplit/Nih", &data));
	assert (data == object);

	nih_free (object);

	TEST_TRUE (dbus_connection_get_object_path_data (
			   conn, "/com/netsplit/Nih", &data));
	TEST_EQ_P (data, NULL);

	TEST_DBUS_CLOSE (conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}

void
test_object_unregister (void)
{
	pid_t           dbus_pid;
	DBusConnection *conn;
	NihDBusObject * object;

	/* Check that when a D-Bus connection is destroyed, any registered
	 * D-Bus objects go as well.
	 */
	TEST_FUNCTION ("nih_dbus_object_unregister");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (conn);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      all_interfaces, &object);
	assert (object != NULL);

	TEST_FREE_TAG (object);

	TEST_DBUS_CLOSE (conn);

	TEST_FREE (object);


	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_object_message (void)
{
	pid_t            dbus_pid;
	DBusConnection * server_conn;
	DBusConnection * client_conn;
	NihDBusObject *  object;
	DBusMessage *    message;
	dbus_uint32_t    serial;
	DBusMessage *    reply;

	TEST_FUNCTION ("nih_dbus_object_message");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


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
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"Nih.TestA",
			"Foo");
		assert (message != NULL);

		assert (dbus_connection_send (client_conn, message, NULL));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

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
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		bar_decline = FALSE;
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			NULL,
			"Bar");
		assert (message != NULL);

		assert (dbus_connection_send (client_conn, message, NULL));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (foo_called);
		TEST_TRUE (bar_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);
	}

	nih_free (object);


	/* Check that if the first of two handlers for a method without
	 * a specified interface declines to handle, we move onto the
	 * second one.
	 */
	TEST_FEATURE ("with first handler declining and second available");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		bar_decline = TRUE;
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			NULL,
			"Bar");
		assert (message != NULL);

		assert (dbus_connection_send (client_conn, message, NULL));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (bar_called);
		TEST_TRUE (foo_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		bar_decline = FALSE;
	}

	nih_free (object);


	/* Check that an unknown method on a known interface results in
	 * an error being returned to the caller.
	 */
	TEST_FEATURE ("with unknown method on known interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"Nih.TestB",
			"Wibble");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that an unknown method on an unknown interface results in
	 * an error being returned to the caller.
	 */
	TEST_FEATURE ("with unknown method on unknown interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"Nih.TestC",
			"Wibble");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that an unknown method with no specified interface results in
	 * an error being returned to the caller.
	 */
	TEST_FEATURE ("with unknown method with no interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		foo_called = FALSE;
		bar_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			NULL,
			"Wibble");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
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

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			"Nih.TestA",
			"Foo");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_FALSE (foo_called);
		TEST_FALSE (bar_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_object_introspect (void)
{
	pid_t            dbus_pid;
	DBusConnection * server_conn;
	DBusConnection * client_conn;
	NihDBusObject *  object;
	NihDBusObject *  child1;
	NihDBusObject *  child2;
	DBusMessage *    message;
	dbus_uint32_t    serial;
	DBusMessage *    reply;
	const char *     xml;
	DBusMessageIter  iter;

	TEST_FUNCTION ("nih_dbus_object_introspect");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that the Introspect message is handled internally with
	 * an accurate portrayal of the interfaces and their properties
	 * returned.
	 */
	TEST_FEATURE ("with fully-fledged object");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE,
			"Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "s"));

		TEST_TRUE (dbus_message_get_args (reply, NULL,
						  DBUS_TYPE_STRING, &xml,
						  DBUS_TYPE_INVALID));

		TEST_EQ_STRN (xml, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
		xml += strlen (DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);

		TEST_EQ_STRN (xml, "<node name=\"/com/netsplit/Nih\">\n");
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
		TEST_EQ_STRN (xml, "      <arg type=\"d\" direction=\"in\"/>\n");
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
		TEST_EQ_STRN (xml, "      <arg type=\"d\" direction=\"in\"/>\n");
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

		TEST_EQ_STRN (xml, "  <interface name=\"Nih.TestC\">\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <property name=\"Colour\" type=\"u\" access=\"readwrite\"/>\n");
		xml = strchr (xml, '\n') + 1;
		TEST_EQ_STRN (xml, "    <property name=\"Height\" type=\"u\" access=\"read\"/>\n");
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

		TEST_EQ_STRN (xml, "</node>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STR (xml, "");

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that the Introspect message does not include the
	 * Properties interfaces in the output if none of the interfaces
	 * implement properties.
	 */
	TEST_FEATURE ("with no properties");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      one_interface, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE,
			"Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "s"));

		TEST_TRUE (dbus_message_get_args (reply, NULL,
						  DBUS_TYPE_STRING, &xml,
						  DBUS_TYPE_INVALID));

		TEST_EQ_STRN (xml, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
		xml += strlen (DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);

		TEST_EQ_STRN (xml, "<node name=\"/com/netsplit/Nih\">\n");
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
		TEST_EQ_STRN (xml, "      <arg type=\"d\" direction=\"in\"/>\n");
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

		TEST_EQ_STRN (xml, "</node>\n");
		xml = strchr (xml, '\n') + 1;

		TEST_EQ_STR (xml, "");

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that the Introspect message works when there are no
	 * interfaces.
	 */
	TEST_FEATURE ("with no interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE,
			"Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "s"));

		TEST_TRUE (dbus_message_get_args (reply, NULL,
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

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that the Introspect message contains node entries for
	 * children, but doesn't bother to flesh them out.
	 */
	TEST_FEATURE ("with children nodes");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);
	child1 = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih/Frodo",
				      one_interface, &server_conn);
	child2 = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih/Bilbo",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE,
			"Introspect");
		assert (message != NULL);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);
		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "s"));

		TEST_TRUE (dbus_message_get_args (reply, NULL,
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

		dbus_message_unref (reply);
	}

	nih_free (child2);
	nih_free (child1);
	nih_free (object);


	/* Check that we receive an Invalid Args error when we pass too
	 * many arguments.
	 */
	TEST_FEATURE ("with too many arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_INTROSPECTABLE,
			"Introspect");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		xml = "<node/>\n";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&xml));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_object_property_get (void)
{
	pid_t            dbus_pid;
	DBusConnection * server_conn;
	DBusConnection * client_conn;
	NihDBusObject *  object;
	DBusMessage *    message;
	DBusMessageIter  iter;
	DBusMessageIter  subiter;
	const char *     interface_name;
	const char *     property_name;
	dbus_uint32_t    serial;
	DBusMessage *    reply;
	const char *     str_value;
	uint32_t         uint32_value;
	DBusError        dbus_error;

	TEST_FUNCTION ("nih_dbus_object_property_get");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can get the value of a read/write property, with the
	 * actual reply handled internally but the variant appended to
	 * the message.
	 */
	TEST_FEATURE ("with read/write property");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		colour = "blue";
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "v"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);

		TEST_EQ_STR (str_value, "blue");

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we can get the value of a read-only property, with the
	 * actual reply handled internally but the variant appended to
	 * the message.
	 */
	TEST_FEATURE ("with read-only property");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		size_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Size";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (size_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "v"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);

		TEST_EQ (uint32_value, 34);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we can't get the value of a write-only property, and
	 * that an "access denied" error message is returned instead.
	 */
	TEST_FEATURE ("with write-only property");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Poke";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_ACCESS_DENIED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that the first of two properties with the same name
	 * but on different interfaces is used when the property interface
	 * is not given.
	 */
	TEST_FEATURE ("with property registered to multiple interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour = "blue";
		colour_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "v"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);

		TEST_EQ_STR (str_value, "blue");

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that attempting to get an unknown property on a known
	 * interface results in an error reply.
	 */
	TEST_FEATURE ("with unknown property on known interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Height";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that a property on an unknown interface always results in
	 * an error reply.
	 */
	TEST_FEATURE ("with unknown property on unknown interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.FooBar";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that an unknown property when no interface was specified
	 * results in an error reply.
	 */
	TEST_FEATURE ("with unknown property with no interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Width";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that an error reply is always received when no interfaces
	 * were defined.
	 */
	TEST_FEATURE ("with no interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Width";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that a property get handler may raise a D-Bus error, and
	 * that it is returned by the Get method.
	 */
	TEST_FEATURE ("with D-Bus error from handler");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		colour = "secret";
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_ACCESS_DENIED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that a property get handler may raise a generic error, which
	 * should be returned as a general failed message.
	 */
	TEST_FEATURE ("with generic error from handler");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		colour = "chicken";
		colour_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, strerror (EBADF));
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we forget to
	 * pass the property name.
	 */
	TEST_FEATURE ("with missing property name");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we forget to
	 * pass any arguments.
	 */
	TEST_FEATURE ("with missing arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we pass too
	 * many arguments.
	 */
	TEST_FEATURE ("with too many arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Get");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		str_value = "pink";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&str_value));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_object_property_get_all (void)
{
	pid_t            dbus_pid;
	DBusConnection * server_conn;
	DBusConnection * client_conn;
	NihDBusObject *  object;
	DBusMessage *    message;
	DBusMessageIter  iter;
	DBusMessageIter  arrayiter;
	DBusMessageIter  dictiter;
	DBusMessageIter  subiter;
	const char *     interface_name;
	dbus_uint32_t    serial;
	DBusMessage *    reply;
	const char *     property_name;
	const char *     str_value;
	dbus_uint32_t    uint32_value;
	DBusError        dbus_error;

	TEST_FUNCTION ("nih_dbus_object_property_get_all");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can get the values of all properties on a given
	 * interface, receiving a reply containing an array of dictionary
	 * entries mapping property name to its variant value.
	 */
	TEST_FEATURE ("with known interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		colour = "blue";
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_TRUE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "a{sv}"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);
		dbus_message_iter_recurse (&arrayiter, &dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&dictiter, &property_name);
		TEST_EQ_STR (property_name, "Colour");

		dbus_message_iter_next (&dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_VARIANT);
		dbus_message_iter_recurse (&dictiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "blue");

		dbus_message_iter_next (&dictiter);
		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);
		dbus_message_iter_recurse (&arrayiter, &dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&dictiter, &property_name);
		TEST_EQ_STR (property_name, "Size");

		dbus_message_iter_next (&dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_VARIANT);
		dbus_message_iter_recurse (&dictiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);

		dbus_message_iter_next (&dictiter);
		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);


		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that when we don't given an interface, the values of all
	 * properties on all interfaces are received - except where a
	 * property with the same name exists on multiple in which case
	 * the first matching name is returned.
	 */
	TEST_FEATURE ("with property registered to multiple interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		colour = "blue";
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_TRUE (size_get_called);
		TEST_TRUE (other_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "a{sv}"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);
		dbus_message_iter_recurse (&arrayiter, &dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&dictiter, &property_name);
		TEST_EQ_STR (property_name, "Colour");

		dbus_message_iter_next (&dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_VARIANT);
		dbus_message_iter_recurse (&dictiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "blue");

		dbus_message_iter_next (&dictiter);
		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);
		dbus_message_iter_recurse (&arrayiter, &dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&dictiter, &property_name);
		TEST_EQ_STR (property_name, "Size");

		dbus_message_iter_next (&dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_VARIANT);
		dbus_message_iter_recurse (&dictiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 34);

		dbus_message_iter_next (&dictiter);
		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_DICT_ENTRY);
		dbus_message_iter_recurse (&arrayiter, &dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_STRING);

		dbus_message_iter_get_basic (&dictiter, &property_name);
		TEST_EQ_STR (property_name, "Height");

		dbus_message_iter_next (&dictiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_VARIANT);
		dbus_message_iter_recurse (&dictiter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 186);

		dbus_message_iter_next (&dictiter);
		TEST_EQ (dbus_message_iter_get_arg_type (&dictiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&arrayiter);


		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);


		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that attempting to get the values of all properties on
	 * an interface which has none results in a reply containing an empty
	 * array, rather than an error.
	 */
	TEST_FEATURE ("with known interface but no properties");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestA";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "a{sv}"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that attempting to get the values of all properties on
	 * an unknown interface results in a reply containing an empty
	 * array, rather than an error.
	 */
	TEST_FEATURE ("with unknown interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.FooBar";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "a{sv}"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that attempting to get the values of all properties when
	 * there are no interfaces results in a reply containing an empty
	 * array, rather than an error.
	 */
	TEST_FEATURE ("with no interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestA";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, "a{sv}"));

		dbus_message_iter_init (reply, &iter);
		dbus_message_iter_recurse (&iter, &arrayiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&arrayiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that trying to get the values of all properties when one
	 * returns a D-Bus error returns that D-Bus error.
	 */
	TEST_FEATURE ("with D-Bus error from handler");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		colour = "secret";
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_ACCESS_DENIED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that trying to get the values of all properties when one
	 * returns a generic error means that property is simply not included
	 * in the returned set.
	 */
	TEST_FEATURE ("with generic error from handler");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		colour = "chicken";
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, strerror (EBADF));
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we forget to
	 * pass any arguments.
	 */
	TEST_FEATURE ("with missing arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we pass too
	 * many arguments.
	 */
	TEST_FEATURE ("with too many arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_get_called = FALSE;
		size_get_called = FALSE;
		other_get_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"GetAll");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_get_called);
		TEST_FALSE (size_get_called);
		TEST_FALSE (other_get_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	TEST_DBUS_CLOSE (client_conn);
	TEST_DBUS_CLOSE (server_conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


void
test_object_property_set (void)
{
	pid_t            dbus_pid;
	DBusConnection * server_conn;
	DBusConnection * client_conn;
	NihDBusObject *  object;
	DBusMessage *    message;
	DBusMessageIter  iter;
	DBusMessageIter  subiter;
	const char *     interface_name;
	const char *     property_name;
	dbus_uint32_t    serial;
	DBusMessage *    reply;
	const char *     str_value;
	double           double_value;
	dbus_uint32_t    uint32_value;
	DBusError        dbus_error;

	TEST_FUNCTION ("nih_dbus_object_property_set");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (server_conn);
	TEST_DBUS_OPEN (client_conn);


	/* Check that we can set the value of a read/write property, with the
	 * registered setter function being called to do so with the
	 * right value.
	 */
	TEST_FEATURE ("with read/write property");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		assert (dbus_connection_send (client_conn, message, &serial));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_set_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, ""));

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we can set the value of a write-only property, with the
	 * registered setter function being called to do so with the
	 * right value.
	 */
	TEST_FEATURE ("with write-only property");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Poke";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter));

		double_value = 3.14;
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		assert (dbus_connection_send (client_conn, message, &serial));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (poke_set_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, ""));

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we cannot set the value of a read-only property, and
	 * that the access denied error is returned instead.
	 */
	TEST_FEATURE ("with read-only property");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Size";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_UINT32_AS_STRING,
							  &subiter));

		uint32_value = 34;
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
							&uint32_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		assert (dbus_connection_send (client_conn, message, &serial));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_ACCESS_DENIED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that the first of two properties with the same name
	 * but on different interfaces is used when the property interface
	 * is not given.
	 */
	TEST_FEATURE ("with property registered to multiple interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_EQ (dbus_message_get_type (reply),
			 DBUS_MESSAGE_TYPE_METHOD_RETURN);
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		TEST_TRUE (dbus_message_has_signature (reply, ""));

		dbus_message_iter_init (reply, &iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that attempting to set an unknown property on a known
	 * interface results in an error reply.
	 */
	TEST_FEATURE ("with unknown property on known interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Height";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that a property on an unknown interface always results in
	 * an error reply.
	 */
	TEST_FEATURE ("with unknown property on unknown interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.FooBar";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that an unknown property when no interface was specified
	 * results in an error reply.
	 */
	TEST_FEATURE ("with unknown property with no interface");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Width";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that an error reply is always received when no interfaces
	 * were defined.
	 */
	TEST_FEATURE ("with no interfaces");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      no_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Width";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_UNKNOWN_METHOD));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that when the setter function returns a D-Bus error,
	 * that is returned as a reply to the caller.
	 */
	TEST_FEATURE ("with D-Bus error from handler");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "pig";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		assert (dbus_connection_send (client_conn, message, &serial));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_set_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, "com.netsplit.Nih.NotAColour"));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that when the setter function returns a generic error,
	 * the message is returned inside a D-Bus failed error.
	 */
	TEST_FEATURE ("with generic error from handler");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      prop_interface, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		assert (dbus_connection_send (client_conn, message, &serial));
		dbus_connection_flush (client_conn);

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_TRUE (colour_set_called);
		TEST_EQ_P (last_object, object);
		TEST_FREE (last_message);
		TEST_EQ_P (last_message_conn, server_conn);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_FAILED));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_error_init (&dbus_error);
		dbus_set_error_from_message (&dbus_error, reply);
		TEST_EQ_STR (dbus_error.message, strerror (EBADF));
		dbus_error_free (&dbus_error);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we forget to
	 * pass the property value.
	 */
	TEST_FEATURE ("with missing property value");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we forget to
	 * pass the property name or value.
	 */
	TEST_FEATURE ("with missing property name and value");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we forget to
	 * pass any arguments.
	 */
	TEST_FEATURE ("with missing arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


	/* Check that we receive an Invalid Args error when we pass too
	 * many arguments.
	 */
	TEST_FEATURE ("with too many arguments");
	object = nih_dbus_object_new (NULL, server_conn, "/com/netsplit/Nih",
				      all_interfaces, &server_conn);

	TEST_ALLOC_FAIL {
		colour_set_called = FALSE;
		poke_set_called = FALSE;
		last_object = NULL;
		last_message = NULL;
		last_message_conn = NULL;

		message = dbus_message_new_method_call (
			dbus_bus_get_unique_name (server_conn),
			"/com/netsplit/Nih",
			DBUS_INTERFACE_PROPERTIES,
			"Set");
		assert (message != NULL);

		dbus_message_iter_init_append (message, &iter);

		interface_name = "Nih.TestB";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&interface_name));

		property_name = "Colour";
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&property_name));

		assert (dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter));

		str_value = "red";
		assert (dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value));

		assert (dbus_message_iter_close_container (&iter, &subiter));

		uint32_value = 32;
		assert (dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
							&uint32_value));

		TEST_ALLOC_SAFE {
			assert (dbus_connection_send (client_conn, message, &serial));
			dbus_connection_flush (client_conn);
		}

		dbus_message_unref (message);

		TEST_DBUS_DISPATCH (server_conn);

		TEST_FALSE (colour_set_called);
		TEST_FALSE (poke_set_called);
		TEST_EQ_P (last_object, NULL);
		TEST_EQ_P (last_message_conn, NULL);

		TEST_DBUS_MESSAGE (client_conn, reply);

		TEST_TRUE (dbus_message_is_error (reply, DBUS_ERROR_INVALID_ARGS));
		TEST_EQ (dbus_message_get_reply_serial (reply), serial);

		dbus_message_unref (reply);
	}

	nih_free (object);


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

	test_object_new ();
	test_object_destroy ();
	test_object_unregister ();
	test_object_message ();
	test_object_introspect ();
	test_object_property_get ();
	test_object_property_get_all ();
	test_object_property_set ();

	return 0;
}
