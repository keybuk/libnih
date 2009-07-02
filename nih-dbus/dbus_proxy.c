/* libnih
 *
 * dbus_proxy.c - D-Bus remote object proxy implementation
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

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_object.h>

#include "dbus_proxy.h"


/* Prototypes for static functions */
static int   nih_dbus_proxy_destroy           (NihDBusProxy *proxy);
static int   nih_dbus_proxy_name_track        (NihDBusProxy *proxy)
	__attribute__ ((warn_unused_result));
static char *nih_dbus_proxy_name_rule         (const void *parent,
					       NihDBusProxy *proxy)
	__attribute__ ((warn_unused_result, malloc));
static int   nih_dbus_proxy_signal_destroy    (NihDBusProxySignal *proxied);
static char *nih_dbus_proxy_signal_rule       (const void *parent,
					       NihDBusProxySignal *proxied)
	__attribute__ ((warn_unused_result, malloc));

/* Prototypes for handler functions */
static DBusHandlerResult nih_dbus_proxy_name_owner_changed (DBusConnection *connection,
							    DBusMessage *message,
							    NihDBusProxy *proxy);


/**
 * nih_dbus_proxy_new:
 * @parent: parent object for new proxy,
 * @connection: D-Bus connection to associate with,
 * @name: well-known name of object owner,
 * @path: path of object,
 * @lost_handler: optional handler for remote object loss.
 * @data: data pointer for handlers.
 *
 * Creates a new D-Bus proxy for a remote object on @connection with the
 * well-known or unique bus name @name at @path.  The returned structure
 * is allocated with nih_alloc() and holds a reference to @connection.
 *
 * @name may be NULL for peer-to-peer D-Bus connections.
 *
 * Proxies are not generally bound to the life-time of the connection or
 * the remote object, thus there may be periods when functions will fail
 * or signal filter functions left dormant due to unavailability of the
 * remote object or even cease permanently when the bus connection is
 * disconnected.
 *
 * @name will be tracked on the bus, with the current owner's unique name
 * being available in the returned structure's owner member.  Should the
 * name be lost from the bus, the optional @lost_handler function will be
 * called to allow clean-up of the proxy.

 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned proxy.  When all parents
 * of the returned proxy are freed, the returned proxy will also be
 * freed.
 *
 * Returns: new NihDBusProxy structure on success, or NULL on raised
 * error.
 **/
NihDBusProxy *
nih_dbus_proxy_new (const void *       parent,
		    DBusConnection *   connection,
		    const char *       name,
		    const char *       path,
		    NihDBusLostHandler lost_handler,
		    void *             data)
{
	NihDBusProxy *proxy;

	nih_assert (connection != NULL);
	nih_assert (path != NULL);
	nih_assert ((lost_handler == NULL) || (name != NULL));

	proxy = nih_new (parent, NihDBusProxy);
	if (! proxy)
		nih_return_no_memory_error (NULL);

	proxy->connection = connection;

	proxy->name = NULL;
	if (name) {
		proxy->name = nih_strdup (proxy, name);
		if (! proxy->name) {
			nih_free (proxy);
			nih_return_no_memory_error (NULL);
		}
	}

	proxy->owner = NULL;

	proxy->path = nih_strdup (proxy, path);
	if (! proxy->path) {
		nih_free (proxy);
		nih_return_no_memory_error (NULL);
	}

	proxy->auto_start = TRUE;

	proxy->lost_handler = lost_handler;
	proxy->data = data;

	if (proxy->name) {
		if (nih_dbus_proxy_name_track (proxy) < 0) {
			nih_free (proxy);
			return NULL;
		}
	}

	dbus_connection_ref (proxy->connection);
	nih_alloc_set_destructor (proxy, nih_dbus_proxy_destroy);

	return proxy;
}

/**
 * nih_dbus_proxy_destroy:
 * @proxy: proxy object being destroyed.
 *
 * Destructor function for an NihDBusProxy structure; drops the bus rule
 * matching the NameOwnerChanged signal, the associated filter function,
 * and the reference to the D-Bus connection it holds.
 *
 * Returns: always zero.
 **/
static int
nih_dbus_proxy_destroy (NihDBusProxy *proxy)
{
	nih_local char *rule = NULL;
	DBusError       dbus_error;

	nih_assert (proxy != NULL);

	if (proxy->name) {
		rule = NIH_MUST (nih_dbus_proxy_name_rule (NULL, proxy));

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (proxy->connection, rule, &dbus_error);
		dbus_error_free (&dbus_error);

		dbus_connection_remove_filter (proxy->connection,
					       (DBusHandleMessageFunction)nih_dbus_proxy_name_owner_changed,
					       proxy);
	}

	dbus_connection_unref (proxy->connection);

	return 0;
}


/**
 * nih_dbus_proxy_name_track:
 * @proxy: proxy object.
 *
 * Set up name tracking for the given @proxy object.  We get the current
 * owner of the name in a synchronous call and set the connection up to
 * watch for a change in that owner updating the proxy's owner member in
 * both cases.
 *
 * If the proxy has no owner, the connection is instead set up to wait
 * for it to come onto the bus, and then reset later.
 *
 * Returns: 0 on success, negative value on raised error.
 **/
static int
nih_dbus_proxy_name_track (NihDBusProxy *proxy)
{
	nih_local char *rule = NULL;
	DBusError       dbus_error;
	DBusMessage *   method_call;
	DBusMessage *   reply;
	const char *    owner;

	nih_assert (proxy != NULL);
	nih_assert (proxy->name != NULL);

	/* Add the filter function that handles the NameOwnerChanged
	 * signal.  We need to do this first so that we can handle anything
	 * that arrives after we add the signal match.
	 */
	if (! dbus_connection_add_filter (proxy->connection,
					  (DBusHandleMessageFunction)nih_dbus_proxy_name_owner_changed,
					  proxy, NULL))
		nih_return_no_memory_error (-1);

	/* Ask the bus to send us matching signals.  We've put the filter
	 * function in place so we'll get callbacks straight away; but we
	 * still need to do this before asking for the current name so
	 * we don't miss something.
	 */
	rule = nih_dbus_proxy_name_rule (NULL, proxy);
	if (! rule) {
		nih_error_raise_no_memory ();
		goto error_after_filter;
	}

	dbus_error_init (&dbus_error);

	dbus_bus_add_match (proxy->connection, rule, &dbus_error);
	if (dbus_error_is_set (&dbus_error)) {
		if (dbus_error_has_name (&dbus_error, DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise_no_memory ();
		} else {
			nih_dbus_error_raise (dbus_error.name,
					      dbus_error.message);
		}

		dbus_error_free (&dbus_error);
		goto error_after_filter;
	}

	/* Now that the bus will send us signals about changes in the name's
	 * owner, and we'll handle them, we can get the current owner of the
	 * name.  We may have some signals in the queue that predate this,
	 * but the end result will be the same.
	 */
	method_call = dbus_message_new_method_call (DBUS_SERVICE_DBUS,
						    DBUS_PATH_DBUS,
						    DBUS_INTERFACE_DBUS,
						    "GetNameOwner");
	if (! method_call) {
		nih_error_raise_no_memory ();

		dbus_error_free (&dbus_error);
		goto error_after_match;
	}

	if (! dbus_message_append_args (method_call,
					DBUS_TYPE_STRING, &proxy->name,
					DBUS_TYPE_INVALID)) {
		nih_error_raise_no_memory ();

		dbus_message_unref (method_call);
		dbus_error_free (&dbus_error);
		goto error_after_match;
	}

	/* Parse the reply; an owner is returned, we fill in the owner
	 * member of the proxy - otherwise we set it to NULL.
	 */
	reply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call,
							   -1, &dbus_error);
	if (! reply) {
		if (dbus_error_has_name (&dbus_error,
					 DBUS_ERROR_NAME_HAS_NO_OWNER)) {
			nih_debug ("%s is not currently owned", proxy->name);

			dbus_message_unref (method_call);
			dbus_error_free (&dbus_error);

			/* Not an error */
			return 0;

		} else if (dbus_error_has_name (&dbus_error,
						DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise_no_memory ();
		} else {
			nih_dbus_error_raise (dbus_error.name,
					      dbus_error.message);
		}

		dbus_message_unref (method_call);
		dbus_error_free (&dbus_error);
		goto error_after_match;
	}

	dbus_message_unref (method_call);

	if (! dbus_message_get_args (reply, &dbus_error,
				     DBUS_TYPE_STRING, &owner,
				     DBUS_TYPE_INVALID)) {
		if (dbus_error_has_name (&dbus_error, DBUS_ERROR_NO_MEMORY)) {
			nih_error_raise_no_memory ();
		} else {
			nih_dbus_error_raise (dbus_error.name,
					      dbus_error.message);
		}

		dbus_message_unref (reply);
		dbus_error_free (&dbus_error);
		goto error_after_match;
	}

	dbus_error_free (&dbus_error);

	proxy->owner = nih_strdup (proxy, owner);
	if (! proxy->owner) {
		nih_error_raise_no_memory ();

		dbus_message_unref (reply);
		goto error_after_match;
	}

	dbus_message_unref (reply);

	nih_debug ("%s is currently owned by %s", proxy->name, proxy->owner);

	return 0;

error_after_match:
	dbus_error_init (&dbus_error);
	dbus_bus_remove_match (proxy->connection, rule, &dbus_error);
	dbus_error_free (&dbus_error);
error_after_filter:
	dbus_connection_remove_filter (proxy->connection,
				       (DBusHandleMessageFunction)nih_dbus_proxy_name_owner_changed,
				       proxy);

	return -1;
}

/**
 * nih_dbus_proxy_name_rule:
 * @parent: parent object for new string,
 * @proxy: proxy object.
 *
 * Generates a D-Bus match rule for the NameOwnerChanged signal for the
 * given @proxy.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL on insufficient memory.
 **/
static char *
nih_dbus_proxy_name_rule (const void *  parent,
			  NihDBusProxy *proxy)
{
	char *rule;

	nih_assert (proxy != NULL);
	nih_assert (proxy->name != NULL);

	rule = nih_sprintf (parent, ("type='%s',sender='%s',path='%s',"
				     "interface='%s',member='%s',"
				     "arg0='%s'"),
			    "signal",
			    DBUS_SERVICE_DBUS,
			    DBUS_PATH_DBUS,
			    DBUS_INTERFACE_DBUS,
			    "NameOwnerChanged",
			    proxy->name);

	return rule;
}

/**
 * nih_dbus_proxy_name_owner_changed:
 * @connection: D-Bus connection signal received on,
 * @message: signal message,
 * @proxy: associated proxy object.
 *
 * This function is called by D-Bus on receipt of the NameOwnerChanged
 * signal for the registered name that @proxy represents.  The proxy's
 * lost_handler function is called to decide what to do about it.
 *
 * Returns: usually DBUS_HANDLER_RESULT_NOT_YET_HANDLED so other signal
 * handlers also get a look-in, DBUS_HANDLED_RESULT_NEED_MEMORY if
 * insufficient memory.
 **/
static DBusHandlerResult
nih_dbus_proxy_name_owner_changed (DBusConnection *connection,
				   DBusMessage *   message,
				   NihDBusProxy *  proxy)
{
	DBusError   dbus_error;
	const char *name;
	const char *old_owner;
	const char *new_owner;

	nih_assert (connection != NULL);
	nih_assert (message != NULL);
	nih_assert (proxy->connection == connection);
	nih_assert (proxy->name != NULL);

	if (! dbus_message_is_signal (message, DBUS_INTERFACE_DBUS,
				      "NameOwnerChanged"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (! dbus_message_has_path (message, DBUS_PATH_DBUS))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (! dbus_message_has_sender (message, DBUS_SERVICE_DBUS))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	dbus_error_init (&dbus_error);
	if (! dbus_message_get_args (message, &dbus_error,
				     DBUS_TYPE_STRING, &name,
				     DBUS_TYPE_STRING, &old_owner,
				     DBUS_TYPE_STRING, &new_owner,
				     DBUS_TYPE_INVALID)) {
		dbus_error_free (&dbus_error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_error_free (&dbus_error);

	if (strcmp (name, proxy->name))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	/* Ok, it's really the right NameOwnerChanged signal.  If the name
	 * has a new owner, update the owner property (tracking a well known
	 * name between instances) otherwise call the lost handler.
	 */
	if (strlen (new_owner)) {
		nih_debug ("%s changed owner from %s to %s",
			   proxy->name, old_owner, new_owner);

		if (proxy->owner)
			nih_unref (proxy->owner, proxy);
		proxy->owner = NIH_MUST (nih_strdup (proxy, new_owner));
	} else {
		nih_debug ("%s owner left the bus", proxy->name);

		if (proxy->owner)
			nih_unref (proxy->owner, proxy);
		proxy->owner = NULL;

		if (proxy->lost_handler) {
			nih_error_push_context ();
			proxy->lost_handler (proxy->data, proxy);
			nih_error_pop_context ();
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


/**
 * nih_dbus_proxy_connect:
 * @proxy: proxy for remote object,
 * @interface: signal interface definition,
 * @name: name of signal,
 * @handler: signal handler function,
 * @data: data to pass to @handler.
 *
 * Connect the signal @name on @interface to @proxy so that the @handler
 * function is passed to the filter function defined by @signal when it
 * is received on the proxied D-Bus connection.
 *
 * The signal can be disconnected by freeing the returned structure, the
 * signal is also bound to the lifetime of @proxy so that the signal is
 * disconnected when the proxy is freed.
 *
 * Returns: newly allocated NihDBusProxySignal structure or NULL on raised
 * error.
 **/
NihDBusProxySignal *
nih_dbus_proxy_connect (NihDBusProxy *          proxy,
			const NihDBusInterface *interface,
			const char *            name,
			NihDBusSignalHandler    handler,
			void *                  data)
{
	NihDBusProxySignal *proxied;
	nih_local char *    rule = NULL;
	DBusError           dbus_error;

	nih_assert (proxy != NULL);
	nih_assert (interface != NULL);
	nih_assert (name != NULL);
	nih_assert (handler != NULL);

	proxied = nih_new (proxy, NihDBusProxySignal);
	if (! proxied)
		nih_return_no_memory_error (NULL);

	proxied->proxy = proxy;
	proxied->interface = interface;
	proxied->signal = NULL;
	proxied->handler = handler;
	proxied->data = data;

	for (const NihDBusSignal *signal = interface->signals;
	     signal && signal->name; signal++) {
		if (! strcmp (signal->name, name)) {
			proxied->signal = signal;
			break;
		}
	}
	nih_assert (proxied->signal != NULL);

	if (! dbus_connection_add_filter (proxied->proxy->connection,
					  (DBusHandleMessageFunction)proxied->signal->filter,
					  proxied, NULL)) {
		nih_free (proxied);
		nih_return_no_memory_error (NULL);
	}

	if (proxied->proxy->name) {
		rule = nih_dbus_proxy_signal_rule (NULL, proxied);
		if (! rule) {
			nih_error_raise_no_memory ();
			goto error;
		}

		dbus_error_init (&dbus_error);

		dbus_bus_add_match (proxied->proxy->connection, rule, &dbus_error);
		if (dbus_error_is_set (&dbus_error)) {
			if (dbus_error_has_name (&dbus_error, DBUS_ERROR_NO_MEMORY)) {
				nih_error_raise_no_memory ();
			} else {
				nih_dbus_error_raise (dbus_error.name,
						      dbus_error.message);
			}

			dbus_error_free (&dbus_error);
			goto error;
		}
	}

	nih_alloc_set_destructor (proxied, nih_dbus_proxy_signal_destroy);

	return proxied;

error:
	dbus_connection_remove_filter (proxied->proxy->connection,
				       (DBusHandleMessageFunction)proxied->signal->filter,
				       proxied);
	nih_free (proxied);

	return NULL;
}

/**
 * nih_dbus_proxy_signal_destroy:
 * @proxied: proxied signal being destroyed.
 *
 * Destructor function for an NihDBusProxySignal structure; drops the bus
 * rule matching the signal and the associated filter function.
 *
 * Returns: always zero.
 **/
static int
nih_dbus_proxy_signal_destroy (NihDBusProxySignal *proxied)
{
	nih_local char *rule = NULL;
	DBusError       dbus_error;

	nih_assert (proxied != NULL);

	if (proxied->proxy->name) {
		rule = NIH_MUST (nih_dbus_proxy_signal_rule (NULL, proxied));

		dbus_error_init (&dbus_error);
		dbus_bus_remove_match (proxied->proxy->connection,
				       rule, &dbus_error);
		dbus_error_free (&dbus_error);
	}

	dbus_connection_remove_filter (proxied->proxy->connection,
				       (DBusHandleMessageFunction)proxied->signal->filter,
				       proxied);

	return 0;
}

/**
 * nih_dbus_proxy_signal_rule:
 * @parent: parent object for new string,
 * @proxied: proxied signal.
 *
 * Generates a D-Bus match rule for the @proxied signal.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL on insufficient memory.
 **/
static char *
nih_dbus_proxy_signal_rule (const void *        parent,
			    NihDBusProxySignal *proxied)
{
	char *rule;

	nih_assert (proxied != NULL);
	nih_assert (proxied->proxy->name != NULL);

	rule = nih_sprintf (parent, ("type='%s',sender='%s',path='%s',"
				     "interface='%s',member='%s'"),
			    "signal",
			    proxied->proxy->name,
			    proxied->proxy->path,
			    proxied->interface->name,
			    proxied->signal->name);

	return rule;
}
