/* libnih
 *
 * test_dbus_object.c - test suite for nih-dbus/dbus_object.c
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

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/main.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_connection.h>
#include <nih-dbus/dbus_object.h>


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


static int connected = FALSE;
static DBusConnection *last_connection = NULL;

static int
my_connect_handler (DBusServer     *server,
		    DBusConnection *connection)
{
	connected = TRUE;

	last_connection = connection;

	nih_main_loop_exit (0);

	return TRUE;
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
	server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test_dbus",
				  &my_connect_handler, NULL);

	connected = FALSE;
	last_connection = NULL;

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


int
main (int   argc,
      char *argv[])
{
	test_object_new ();
	test_object_destroy ();
	test_object_unregister ();
	test_object_message ();

	return 0;
}
