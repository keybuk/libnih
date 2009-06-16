/* libnih
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NIH_DBUS_PROXY_H
#define NIH_DBUS_PROXY_H

#include <nih/macros.h>

#include <dbus/dbus.h>

#include <nih-dbus/dbus_interface.h>


/**
 * NIH_DBUS_TIMEOUT_DEFAULT:
 *
 * This macro expands to the timeout value used by D-Bus to mean the
 * default timeout (25s).
 **/
#define NIH_DBUS_TIMEOUT_DEFAULT -1

/**
 * NIH_DBUS_TIMEOUT_NEVER:
 *
 * This macro expands to the timeout value used by D-Bus to mean that
 * the call should never timeout.
 **/
#define NIH_DBUS_TIMEOUT_NEVER INT_MAX


/**
 * NihDBusLostHandler:
 * @data: data pointer passed to nih_dbus_proxy_new(),
 * @proxy: proxy object.
 *
 * The D-Bus Lost Handler function is called when a proxied remote object
 * is no longer available, due to the owner of the name being changed or
 * leaving the bus.
 *
 * A common activity is to free the proxy.
 **/
typedef void (*NihDBusLostHandler) (void *data, NihDBusProxy *proxy);

/**
 * NihDBusSignalHandler:
 * @data: data pointer passed to nih_dbus_proxy_new(),
 * @message: NihDBusMessage context for signal arguments.
 *
 * A D-Bus Signal Handler is called when the expected signal is emitted
 * and caught by the signal filter function, generally they will be called
 * with additional arguments representing the demarshalled data from the
 * signal itself.
 **/
typedef void (*NihDBusSignalHandler) (void *data, NihDBusMessage *message,
				      ...);


/**
 * NihDBusProxy:
 * @connection: associated connection,
 * @name: D-Bus name of object owner,
 * @owner: actual unique D-Bus owner,
 * @path: path of object,
 * @lost_handler: handler to call when the proxied object is lost,
 * @data: data to pass to handler functions.
 *
 * Proxy objects represent a remote D-Bus object accessible over the bus.
 * The primary purpose of this object is to combine the three elements
 * of data that uniquely identify that remote object: the connection,
 * the bus name (either well known or unique) and the path.
 *
 * @name may be NULL for peer-to-peer D-Bus connections.
 *
 * Proxies are not generally bound to the life-time of the connection or
 * the remote object, thus there may be periods when functions will fail
 * or signal filter functions left dormant due to unavailability of the
 * remote object or even cease permanently when the bus connection is
 * disconnected.
 *
 * Passing a @lost_handler function means that @name will be tracked on
 * the bus.  Should the owner of @name change @lost_handler will be called
 * to allow clean-up of the proxy.
 **/
struct nih_dbus_proxy {
	DBusConnection *   connection;
	char *             name;
	char *             owner;
	char *             path;

	NihDBusLostHandler lost_handler;
	void *             data;
};

/**
 * NihDBusProxySignal:
 * @connection: associated connection,
 * @name: D-Bus name of object owner,
 * @path: path of object,
 * @interface: signal interface definition,
 * @signal: signal definition,
 * @handler: signal handler function,
 * @data: data to pass to @handler function.
 *
 * This structure represents a connected signal handler @handler which
 * should be run when a matching signal @signal on interface @interface
 * is emitted by an object @path, from @name on @connection.
 *
 * @name may be NULL for peer-to-peer D-Bus connections.
 *
 * Proxied signals are generally bounced to the life cycle of @proxy,
 * however this is not compulsory.
 **/
struct nih_dbus_proxy_signal {
	DBusConnection *        connection;
	char *                  name;
	char *                  path;
	const NihDBusInterface *interface;
	const NihDBusSignal *   signal;
	NihDBusSignalHandler    handler;
	void *                  data;
};


NIH_BEGIN_EXTERN

NihDBusProxy *      nih_dbus_proxy_new     (const void *parent,
					    DBusConnection *connection,
					    const char *name, const char *path,
					    NihDBusLostHandler lost_handler,
					    void *data)
	__attribute__ ((warn_unused_result, malloc));

NihDBusProxySignal *nih_dbus_proxy_connect (const void *parent,
					    NihDBusProxy *proxy,
					    const NihDBusInterface *interface,
					    const char *name,
					    NihDBusSignalHandler handler,
					    void *data)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_PROXY_H */
