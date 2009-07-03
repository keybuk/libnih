/* libnih
 *
 * dbus_object.c - D-Bus local object implementation
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/hash.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_error.h>
#include <nih-dbus/errors.h>

#include "dbus_object.h"


/* Prototypes for static functions */
static int               nih_dbus_object_destroy      (NihDBusObject *object);
static void              nih_dbus_object_unregister   (DBusConnection *connection,
						       NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_message      (DBusConnection *connection,
						       DBusMessage *message,
						       NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_introspect   (DBusConnection *connection,
						       DBusMessage *message,
						       NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_property_get (DBusConnection *connection,
						       DBusMessage *message,
						       NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_property_get_all (DBusConnection *connection,
							   DBusMessage *message,
							   NihDBusObject *object);
static DBusHandlerResult nih_dbus_object_property_set (DBusConnection *connection,
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
 * @connection: D-Bus connection to associate with,
 * @path: path of object,
 * @interfaces: interfaces list to attach,
 * @data: data pointer.
 *
 * Creates a new D-Bus object with the attached list of @interfaces which
 * specify the methods, signals and properties that object will export
 * and the C functions that will handle them.
 *
 * @interfaces should be a NULL-terminated array of pointers to
 * NihDBusInterface structures.  Normally this is constructed using pointers
 * to structures defined by nih-dbus-tool which provides all the necessary
 * glue arrays and functions.
 *
 * The object structure is allocated using nih_alloc() and connected to
 * the given @connection, it can be unregistered by freeing it and it will be
 * automatically unregistered should @connection be disconnected.
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
		     DBusConnection *         connection,
		     const char *             path,
		     const NihDBusInterface **interfaces,
		     void *                   data)
{
	NihDBusObject *object;

	nih_assert (connection != NULL);
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

	/* We don't reference the connection, it's only used to unregister
	 * the object when freed directly; in addition, we get called if
	 * the connection is freed and discard this object - and don't want
	 * to block that happening.
	 */
	object->connection = connection;

	object->data = data;
	object->interfaces = interfaces;
	object->registered = FALSE;

	if (! dbus_connection_register_object_path (object->connection,
						    object->path,
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
		dbus_connection_unregister_object_path (object->connection,
							object->path);
	}

	return 0;
}

/**
 * nih_dbus_object_unregister:
 * @connection: D-Bus connection,
 * @object: D-Bus object to destroy.
 *
 * Called by D-Bus to unregister the @object attached to the D-Bus connection
 * @connection, requires us to free the attached structure.
 **/
static void
nih_dbus_object_unregister (DBusConnection *connection,
			    NihDBusObject * object)
{
	nih_assert (connection != NULL);
	nih_assert (object != NULL);
	nih_assert (object->connection == connection);

	if (object->registered) {
		object->registered = FALSE;
		nih_free (object);
	}
}


/**
 * nih_dbus_object_message:
 * @connection: D-Bus connection,
 * @message: D-Bus message received,
 * @object: Object that received the message.
 *
 * Called by D-Bus when a @message is received for a registered @object.  We
 * handle messages related to introspection and properties ourselves,
 * otherwise the method invoked is located in the @object's interfaces array
 * and the handler function called to handle it.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_object_message (DBusConnection *connection,
			 DBusMessage *   message,
			 NihDBusObject * object)
{
	const NihDBusInterface **interface;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->connection == connection);

	/* Handle introspection internally */
	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
		return nih_dbus_object_introspect (connection, message, object);

	/* Handle properties semi-internally */
	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_PROPERTIES, "Get"))
		return nih_dbus_object_property_get (connection, message, object);

	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_PROPERTIES, "Set"))
		return nih_dbus_object_property_set (connection, message, object);

	if (dbus_message_is_method_call (
		    message, DBUS_INTERFACE_PROPERTIES, "GetAll"))
		return nih_dbus_object_property_get_all (connection, message, object);


	/* No built-in handling, locate a handler function in the defined
	 * interfaces that can handle it.
	 */
	for (interface = object->interfaces; interface && *interface;
	     interface++) {
		const NihDBusMethod *method;

		for (method = (*interface)->methods; method && method->name;
		     method++) {
			nih_assert (method->handler != NULL);

			if (dbus_message_is_method_call (message,
							 (*interface)->name,
							 method->name)) {
				nih_local NihDBusMessage *msg = NULL;
				DBusHandlerResult         result;

				msg = nih_dbus_message_new (NULL,
							    connection, message);
				if (! msg)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				nih_error_push_context ();
				result = method->handler (object, msg);
				nih_error_pop_context ();

				if (result != DBUS_HANDLER_RESULT_NOT_YET_HANDLED)
					return result;
			}
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/**
 * nih_dbus_object_introspect:
 * @connection: D-Bus connection,
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
nih_dbus_object_introspect (DBusConnection *connection,
			    DBusMessage *   message,
			    NihDBusObject * object)
{
	const NihDBusInterface **interface;
	nih_local char *         xml = NULL;
	char **                  children = NULL;
	char **                  child;
	DBusMessage *            reply = NULL;
	int                      have_props = FALSE;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->connection == connection);

	/* Make sure the message signature was what we expected */
	if (! dbus_message_has_signature (message, "")) {
		reply = dbus_message_new_error (message, DBUS_ERROR_INVALID_ARGS,
						_("Invalid arguments to Introspect method"));
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	xml = nih_strdup (NULL, DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE);
	if (! xml)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Root node */
	if (! nih_strcat_sprintf (&xml, NULL, "<node name=\"%s\">\n",
				  object->path))
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
					    "      <arg"))
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				if (arg->name)
					if (! nih_strcat_sprintf (
						    &xml, NULL,
						    " name=\"%s\"",
						    arg->name))
						return DBUS_HANDLER_RESULT_NEED_MEMORY;

				if (! nih_strcat_sprintf (
					    &xml, NULL,
					    " type=\"%s\""
					    " direction=\"%s\"/>\n",
					    arg->type,
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
					    "      <arg"))
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				if (arg->name)
					if (! nih_strcat_sprintf (
						    &xml, NULL,
						    " name=\"%s\"",
						    arg->name))
						return DBUS_HANDLER_RESULT_NEED_MEMORY;

				if (! nih_strcat_sprintf (
					    &xml, NULL,
					    " type=\"%s\"/>\n",
					    arg->type))
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

	/* Obviously we support introspection */
	if (! nih_strcat_sprintf (&xml, NULL,
				  "  <interface name=\"%s\">\n"
				  "    <method name=\"Introspect\">\n"
				  "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
				  "    </method>\n"
				  "  </interface>\n",
				  DBUS_INTERFACE_INTROSPECTABLE))
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Add node items for children */
	if (! dbus_connection_list_registered (connection, object->path, &children))
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

	if (! dbus_connection_send (connection, reply, NULL)) {
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

/**
 * nih_dbus_object_property_get:
 * @connection: D-Bus connection,
 * @message: D-Bus message received,
 * @object: Object that received the message.
 *
 * Called because the D-Bus properties Get method has been invoked on
 * @object  We locate the property in the @object's interfaces array and
 * call the getter function to append a variant onto the reply we
 * generate.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_object_property_get (DBusConnection *connection,
			      DBusMessage *   message,
			      NihDBusObject * object)
{
	DBusMessage *            reply;
	DBusMessageIter          iter;
	const char *             interface_name;
	const char *             property_name;
	const NihDBusInterface **interface;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->connection == connection);

	/* Retrieve the requested interface and property names from the
	 * method call, first making sure the message signature was what
	 * we expected.
	 */
	if (! dbus_message_has_signature (message,
					  (DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING))) {
		reply = dbus_message_new_error (message, DBUS_ERROR_INVALID_ARGS,
						_("Invalid arguments to Get method"));
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_init (message, &iter);
	dbus_message_iter_get_basic (&iter, &interface_name);
	dbus_message_iter_next (&iter);
	dbus_message_iter_get_basic (&iter, &property_name);
	dbus_message_iter_next (&iter);


	/* Locate a getter function in the defined interfaces. */
	for (interface = object->interfaces; interface && *interface;
	     interface++) {
		const NihDBusProperty *property;

		for (property = (*interface)->properties;
		     property && property->name;
		     property++) {
			nih_local NihDBusMessage *msg = NULL;
			int                       ret;

			if (strcmp (property->name, property_name)
			    || (strlen (interface_name)
				&& strcmp ((*interface)->name, interface_name)))
				continue;

			if (property->getter) {
				msg = nih_dbus_message_new (NULL,
							    connection, message);
				if (! msg)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				reply = dbus_message_new_method_return (message);
				if (! reply)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				dbus_message_iter_init_append (reply, &iter);

				nih_error_push_context ();
				ret = property->getter (object, msg, &iter);
				if (ret < 0) {
					NihError *err;

					dbus_message_unref (reply);

					err = nih_error_get ();
					if (err->number == ENOMEM) {
						nih_free (err);
						nih_error_pop_context ();

						return DBUS_HANDLER_RESULT_NEED_MEMORY;
					} else if (err->number == NIH_DBUS_ERROR) {
						NihDBusError *dbus_err = (NihDBusError *)err;

						reply = NIH_MUST (dbus_message_new_error (
									  message,
									  dbus_err->name,
									  dbus_err->message));
						nih_free (err);
						nih_error_pop_context ();
					} else {
						reply = NIH_MUST (dbus_message_new_error (
									  message,
									  DBUS_ERROR_FAILED,
									  err->message));
						nih_free (err);
						nih_error_pop_context ();
					}
				} else {
					nih_error_pop_context ();
				}
			} else {
				reply = dbus_message_new_error_printf (
					message, DBUS_ERROR_ACCESS_DENIED,
					_("The %s property is write-only"),
					property->name);
				if (! reply)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
			}

			if (! dbus_connection_send (connection, reply, NULL)) {
				dbus_message_unref (reply);
				return DBUS_HANDLER_RESULT_NEED_MEMORY;
			}

			dbus_message_unref (reply);

			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/**
 * nih_dbus_object_property_get_all:
 * @connection: D-Bus connection,
 * @message: D-Bus message received,
 * @object: Object that received the message.
 *
 * Called because the D-Bus properties Get method has been invoked on
 * @object  We locate the property in the @object's interfaces array and
 * call the getter function to append a variant onto the reply we
 * generate.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_object_property_get_all (DBusConnection *connection,
				  DBusMessage *   message,
				  NihDBusObject * object)
{
	DBusMessage *             reply;
	DBusMessageIter           iter;
	DBusMessageIter           arrayiter;
	const char *              interface_name;
	nih_local NihHash *       name_hash = NULL;
	nih_local NihDBusMessage *msg = NULL;
	const NihDBusInterface ** interface;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->connection == connection);

	/* Retrieve the requested interface name from the method call,
	 * first making sure the message signature was what we expected.
	 */
	if (! dbus_message_has_signature (message, DBUS_TYPE_STRING_AS_STRING)) {
		reply = dbus_message_new_error (message, DBUS_ERROR_INVALID_ARGS,
						_("Invalid arguments to GetAll method"));
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_init (message, &iter);
	dbus_message_iter_get_basic (&iter, &interface_name);
	dbus_message_iter_next (&iter);

	/* D-Bus forbids us from returning multiple properties with the
	 * same name in the dictionary, so we actually have to build
	 * a dictionary of the properties we've visited.
	 */
	name_hash = nih_hash_string_new (NULL, 0);
	if (! name_hash)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Use the same NihDBusMessage object for each of the getters we
	 * call for efficiency
	 */
	msg = nih_dbus_message_new (NULL, connection, message);
	if (! msg)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	/* Begin constructing the reply immediately as well */
	reply = dbus_message_new_method_return (message);
	if (! reply)
		return DBUS_HANDLER_RESULT_NEED_MEMORY;

	dbus_message_iter_init_append (reply, &iter);

	if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
						(DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
						 DBUS_TYPE_STRING_AS_STRING
						 DBUS_TYPE_VARIANT_AS_STRING
						 DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
						&arrayiter)) {
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	/* Call each of the getter functions for the matching interface,
	 * or all of them if it's an empty string.
	 */
	for (interface = object->interfaces; interface && *interface;
	     interface++) {
		const NihDBusProperty *property;

		if (strlen (interface_name)
		    && strcmp ((*interface)->name, interface_name))
			continue;

		for (property = (*interface)->properties;
		     property && property->name;
		     property++) {
			if (property->getter
			    && (! nih_hash_lookup (name_hash, property->name))) {
				NihListEntry *  entry;
				DBusMessageIter dictiter;
				int             ret;

				entry = nih_list_entry_new (name_hash);
				if (! entry) {
					dbus_message_iter_abandon_container (&iter, &arrayiter);
					dbus_message_unref (reply);
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
				}

				entry->str = (char *)property->name;
				nih_hash_add (name_hash, &entry->entry);

				if (! dbus_message_iter_open_container (
					    &arrayiter, DBUS_TYPE_DICT_ENTRY,
					    NULL, &dictiter)) {
					dbus_message_iter_abandon_container (&iter, &arrayiter);
					dbus_message_unref (reply);
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
				}

				if (! dbus_message_iter_append_basic (
					    &dictiter, DBUS_TYPE_STRING,
					    &(property->name))) {
					dbus_message_iter_abandon_container (&arrayiter, &dictiter);
					dbus_message_iter_abandon_container (&iter, &arrayiter);
					dbus_message_unref (reply);
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
				}

				nih_error_push_context ();
				ret = property->getter (object, msg, &dictiter);
				if (ret < 0) {
					NihError *err;

					dbus_message_iter_abandon_container (&arrayiter, &dictiter);
					dbus_message_iter_abandon_container (&iter, &arrayiter);
					dbus_message_unref (reply);

					err = nih_error_get ();
					if (err->number == ENOMEM) {
						nih_free (err);
						nih_error_pop_context ();

						return DBUS_HANDLER_RESULT_NEED_MEMORY;
					} else if (err->number == NIH_DBUS_ERROR) {
						NihDBusError *dbus_err = (NihDBusError *)err;

						reply = NIH_MUST (dbus_message_new_error (
									  message,
									  dbus_err->name,
									  dbus_err->message));
						nih_free (err);
						nih_error_pop_context ();
					} else {
						reply = NIH_MUST (dbus_message_new_error (
									  message,
									  DBUS_ERROR_FAILED,
									  err->message));
						nih_free (err);
						nih_error_pop_context ();
					}

					goto reply;
				} else {
					nih_error_pop_context ();

					if (! dbus_message_iter_close_container (
						    &arrayiter, &dictiter)) {
						dbus_message_iter_abandon_container (&iter, &arrayiter);
						dbus_message_unref (reply);
						return DBUS_HANDLER_RESULT_NEED_MEMORY;
					}
				}


			}
		}
	}

	/* Close the array and send the reply */
	if (! dbus_message_iter_close_container (&iter, &arrayiter)) {
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

reply:
	if (! dbus_connection_send (connection, reply, NULL)) {
		dbus_message_unref (reply);
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_message_unref (reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

/**
 * nih_dbus_object_property_set:
 * @connection: D-Bus connection,
 * @message: D-Bus message received,
 * @object: Object that received the message.
 *
 * Called because the D-Bus properties Set method has been invoked on
 * @object  We locate the property in the @object's interfaces array and
 * call the setter function to retrieve the variant and generate a reply.
 *
 * Returns: result of handling the message.
 **/
static DBusHandlerResult
nih_dbus_object_property_set (DBusConnection *connection,
			      DBusMessage *   message,
			      NihDBusObject * object)
{
	DBusMessage *            reply;
	DBusMessageIter          iter;
	const char *             interface_name;
	const char *             property_name;
	const NihDBusInterface **interface;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);
	nih_assert (object != NULL);
	nih_assert (object->connection == connection);

	/* Retrieve the requested interface and property names from the
	 * method call, first making sure the message signature was what
	 * we expected.
	 */
	if (! dbus_message_has_signature (message,
					  (DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_VARIANT_AS_STRING))) {
		reply = dbus_message_new_error (message, DBUS_ERROR_INVALID_ARGS,
						_("Invalid arguments to Set method"));
		if (! reply)
			return DBUS_HANDLER_RESULT_NEED_MEMORY;

		if (! dbus_connection_send (connection, reply, NULL)) {
			dbus_message_unref (reply);
			return DBUS_HANDLER_RESULT_NEED_MEMORY;
		}

		dbus_message_unref (reply);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_init (message, &iter);
	dbus_message_iter_get_basic (&iter, &interface_name);
	dbus_message_iter_next (&iter);
	dbus_message_iter_get_basic (&iter, &property_name);
	dbus_message_iter_next (&iter);


	/* Locate a setter function in the defined interfaces. */
	for (interface = object->interfaces; interface && *interface;
	     interface++) {
		const NihDBusProperty *property;

		for (property = (*interface)->properties;
		     property && property->name;
		     property++) {
			nih_local NihDBusMessage *msg = NULL;
			DBusMessage *             reply;
			int                       ret;

			if (strcmp (property->name, property_name)
			    || (strlen (interface_name)
				&& strcmp ((*interface)->name, interface_name)))
				continue;

			if (property->setter) {
				msg = nih_dbus_message_new (NULL,
							    connection, message);
				if (! msg)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;

				nih_error_push_context ();
				ret = property->setter (object, msg, &iter);
				if (ret < 0) {
					NihError *err;

					err = nih_error_get ();
					if (err->number == ENOMEM) {
						nih_free (err);
						nih_error_pop_context ();

						return DBUS_HANDLER_RESULT_NEED_MEMORY;
					} else if (err->number == NIH_DBUS_ERROR) {
						NihDBusError *dbus_err = (NihDBusError *)err;

						reply = NIH_MUST (dbus_message_new_error (
									  message,
									  dbus_err->name,
									  dbus_err->message));
						nih_free (err);
						nih_error_pop_context ();
					} else {
						reply = NIH_MUST (dbus_message_new_error (
									  message,
									  DBUS_ERROR_FAILED,
									  err->message));
						nih_free (err);
						nih_error_pop_context ();
					}
				} else {
					nih_error_pop_context ();

					reply = NIH_MUST (dbus_message_new_method_return (message));
				}
			} else {
				reply = dbus_message_new_error_printf (
					message, DBUS_ERROR_ACCESS_DENIED,
					_("The %s property is read-only"),
					property->name);
				if (! reply)
					return DBUS_HANDLER_RESULT_NEED_MEMORY;
			}

			NIH_MUST (dbus_connection_send (connection, reply, NULL));
			dbus_message_unref (reply);

			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
