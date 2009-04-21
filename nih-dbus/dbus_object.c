/* libnih
 *
 * dbus_object.c - D-Bus local object implementation
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "dbus_object.h"


/* Prototypes for static functions */
static int               nih_dbus_object_destroy    (NihDBusObject *object);
static void              nih_dbus_object_unregister (DBusConnection *conn,
						     NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_message    (DBusConnection *conn,
						     DBusMessage *message,
						     NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_introspect (DBusConnection *conn,
						     DBusMessage *message,
						     NihDBusObject *object);


/**
 * nih_dbus_object_vtable:
 *
 * Table of functions for handling D-Bus objects.
 **/
static const DBusObjectPathVTable nih_dbus_object_vtable = {
	(DBusObjectPathUnregisterFunction)nih_dbus_object_unregister,
	(DBusObjectPathMessageFunction)nih_dbus_object_message,
	NULL,
};


/**
 * nih_dbus_object_new:
 * @parent: parent object for new object,
 * @conn: D-Bus connection to associate with,
 * @path: path of object,
 * @interfaces: interfaces list to attach,
 * @data: data pointer.
 *
 * Creates a new D-Bus object with the attached list of @interfaces which
 * specify the methods, signals and properties that object will export
 * and the C functions that will marshal them.
 *
 * @interfaces should be a NULL-terminated array of pointers to
 * NihDBusInterface structures.  Normally this is constructed using pointers
 * to structures defined by nih-dbus-tool which provides all the necessary
 * glue arrays and functions.
 *
 * The object structure is allocated using nih_alloc() and connected to
 * the given @conn, it can be unregistered by freeing it and it will be
 * automatically unregistered should @conn be disconnected.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned object.  When all parents
 * of the returned object are freed, the returned object will also be
 * freed.
 *
 * Returns: new NihDBusObject structure on success, or NULL if
 * insufficient memory.
 **/
NihDBusObject *
nih_dbus_object_new (const void *             parent,
		     DBusConnection *         conn,
		     const char *             path,
		     const NihDBusInterface **interfaces,
		     void *                   data)
{
	NihDBusObject *object;

	nih_assert (conn != NULL);
	nih_assert (path != NULL);
	nih_assert (interfaces != NULL);

	object = nih_new (parent, NihDBusObject);
	if (! object)
		return NULL;

	object->path = nih_strdup (object, path);
	if (! object->path) {
		nih_free (object);
		return NULL;
	}

	object->conn = conn;
	object->data = data;
	object->interfaces = interfaces;
	object->registered = FALSE;

	if (! dbus_connection_register_object_path (object->conn, object->path,
						    &nih_dbus_object_vtable,
						    object)) {
		nih_free (object);
		return NULL;
	}

	object->registered = TRUE;
	nih_alloc_set_destructor (object, nih_dbus_object_destroy);

	return object;
}

/**
 * nih_dbus_object_destroy:
 * @object: D-Bus object being destroyed.
 *
 * Destructor function for an NihDBusObject structure, ensures that it
 * is unregistered from the attached D-Bus connection and path.
 *
 * Returns: always zero.
 **/
static int
nih_dbus_object_destroy (NihDBusObject *object)
{
	nih_assert (object != NULL);

	if (object->registered) {
		object->registered = FALSE;
		dbus_connection_unregister_object_path (object->conn,
							object->path);
	}

	return 0;
}

/**
 * nih_dbus_object_unregister:
 * @conn: D-Bus connection,
 * @object: D-Bus object to destroy.
 *
 * Called by D-Bus to unregister the @object attached to the D-Bus connection
 * @conn, requires us to free the attached structure.
 **/
static void
nih_dbus_object_unregister (DBusConnection *conn,
			    NihDBusObject * object)
{
	nih_assert (conn != NULL);
	nih_assert (object != NULL);
	nih_assert (object->conn == conn);

	if (object->registered) {
		object->registered = FALSE;
		nih_free (object);
	}
}


/**
 * nih_dbus_object_message:
 * @conn: D-Bus connection,
 * @message: D-Bus message received,
 * @object: Object that received the message.
 *
 * Called by D-Bus when a @message is received for a registered @object.  We
 * handle messages related to introspection and properties ourselves,
 * otherwise the method invoked is located in the @object's interfaces array
 * and the marshaller function called to handle it.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_object_message (DBusConnection *conn,
			 DBusMessage *   message,
			 NihDBusObject * object)
{
	const NihDBusInterface **interface;

	nih_assert (conn != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->conn == conn);

	/* Handle introspection internally */
	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
		return nih_dbus_object_introspect (conn, message, object);

	/* FIXME handle properties */
	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_PROPERTIES, "Get"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_PROPERTIES, "Set"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_PROPERTIES, "GetAll"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;


	/* No built-in handling, locate a marshaller function in the defined
	 * interfaces that can handle it.
	 */
	for (interface = object->interfaces; interface && *interface;
	     interface++) {
		const NihDBusMethod *method;

		for (method = (*interface)->methods; method && method->name;
		     method++) {
			nih_assert (method->marshaller != NULL);

			if (dbus_message_is_method_call (message,
							 (*interface)->name,
							 method->name)) {
				nih_local NihDBusMessage *msg = NULL;
				DBusHandlerResult         result;

				msg = nih_dbus_message_new (NULL,
							    conn, message);
				if (! msg)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				nih_error_push_context ();
				result = method->marshaller (object, msg);
				nih_error_pop_context ();

				return result;
			}
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/**
 * nih_dbus_object_introspect:
 * @conn: D-Bus connection,
 * @message: D-Bus message received,
 * @object: Object that received the message.
 *
 * Called because the D-Bus introspection method has been invoked on @object,
 * we return an XML description of the object's interfaces, methods, signals
 * and properties based on its interfaces array.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_object_introspect (DBusConnection *conn,
			    DBusMessage *   message,
			    NihDBusObject * object)
{
	const NihDBusInterface **interface;
	nih_local char *         xml = NULL;
	char **                  children = NULL;
	char **                  child;
	DBusMessage *            reply = NULL;
	int                      have_props = FALSE;

	nih_assert (conn != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->conn == conn);

	xml = nih_strdup (NULL, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
	if (! xml)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Root node */
	if (! nih_strcat_sprintf (&xml, NULL, "<node name=\"%s\">\n",
				  object->path))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Obviously we support introspection */
	if (! nih_strcat_sprintf (&xml, NULL,
				  "  <interface name=\"%s\">\n"
				  "    <method name=\"Introspect\">\n"
				  "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
				  "    </method>\n"
				  "  </interface>\n",
				  DBUS_INTERFACE_INTROSPECTABLE))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Add each interface definition */
	for (interface = object->interfaces; interface && *interface;
	     interface++) {
		const NihDBusMethod *  method;
		const NihDBusSignal *  signal;
		const NihDBusProperty *property;

		if (! nih_strcat_sprintf (&xml, NULL,
					  "  <interface name=\"%s\">\n",
					  (*interface)->name))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		for (method = (*interface)->methods; method && method->name;
		     method++) {
			const NihDBusArg *arg;

			if (! nih_strcat_sprintf (&xml, NULL,
						  "    <method name=\"%s\">\n",
						  method->name))
				return DBUS_HANDLER_RESULT_NEED_MEMORY;

			for (arg = method->args; arg && arg->type; arg++) {
				if (! nih_strcat_sprintf (
					    &xml, NULL,
					    "      <arg name=\"%s\" type=\"%s\""
					    " direction=\"%s\"/>\n",
					    arg->name, arg->type,
					    (arg->dir == NIH_DBUS_ARG_IN ? "in"
					     : "out")))
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
			}

			if (! nih_strcat (&xml, NULL, "    </method>\n"))
				return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		for (signal = (*interface)->signals; signal && signal->name;
		     signal++) {
			const NihDBusArg *arg;

			if (! nih_strcat_sprintf (&xml, NULL,
						  "    <signal name=\"%s\">\n",
						  signal->name))
				return DBUS_HANDLER_RESULT_NEED_MEMORY;

			for (arg = signal->args; arg && arg->type; arg++) {
				if (! nih_strcat_sprintf (
					    &xml, NULL,
					    "      <arg name=\"%s\" type=\"%s\"/>\n",
					    arg->name, arg->type))
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
			}

			if (! nih_strcat (&xml, NULL, "    </signal>\n"))
				return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		for (property = (*interface)->properties;
		     property && property->name; property++) {
			have_props = TRUE;
			if (! nih_strcat_sprintf (
				    &xml, NULL,
				    "    <property name=\"%s\" type=\"%s\" "
				    "access=\"%s\"/>\n",
				    property->name, property->type,
				    (property->access == NIH_DBUS_READ ? "read"
				     : (property->access == NIH_DBUS_WRITE
					? "write" : "readwrite"))))
				return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		if (! nih_strcat (&xml, NULL, "  </interface>\n"))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	/* We may also support properties, but don't want to announce that
	 * unless we really do have some.
	 */
	if (have_props)
		if (! nih_strcat_sprintf (
			    &xml, NULL,
			    "  <interface name=\"%s\">\n"
			    "    <method name=\"Get\">\n"
			    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
			    "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
			    "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
			    "    </method>\n"
			    "    <method name=\"Set\">\n"
			    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
			    "      <arg name=\"property_name\" type=\"s\" direction=\"in\"/>\n"
			    "      <arg name=\"value\" type=\"v\" direction=\"in\"/>\n"
			    "    </method>\n"
			    "    <method name=\"GetAll\">\n"
			    "      <arg name=\"interface_name\" type=\"s\" direction=\"in\"/>\n"
			    "      <arg name=\"props\" type=\"a{sv}\" direction=\"out\"/>\n"
			    "    </method>\n"
			    "  </interface>\n",
			    DBUS_INTERFACE_PROPERTIES))
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Add node items for children */
	if (! dbus_connection_list_registered (conn, object->path, &children))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	for (child = children; *child; child++) {
		if (! nih_strcat_sprintf (&xml, NULL, "  <node name=\"%s\"/>\n",
					  *child)) {
			dbus_free_string_array (children);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}
	}

	if (! nih_strcat (&xml, NULL, "</node>\n")) {
		dbus_free_string_array (children);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_free_string_array (children);


	/* Generate and send the reply */
	reply = dbus_message_new_method_return (message);
	if (! reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	if (! dbus_message_append_args (reply,
					DBUS_TYPE_STRING, &xml,
					DBUS_TYPE_INVALID)) {
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	if (! dbus_connection_send (conn, reply, NULL)) {
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}
