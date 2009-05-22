/* libnih
 *
 * dbus_proxy.c - D-Bus remote object proxy implementation
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */


#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/logging.h>

#include "dbus_proxy.h"


/**
 * nih_dbus_proxy_new:
 * @parent: parent object for new proxy,
 * @conn: D-Bus connection to associate with,
 * @name: well-known name of object owner,
 * @path: path of object.
 *
 * Creates a new D-Bus proxy for a remote object @path on the well-known
 * bus name @name.
 *
 * The proxy structure is allocated using nih_alloc() and will contain
 * a reference to the given @conn, you should take care to free a proxy
 * when the @conn is disconnected as this will not happen automatically.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned proxy.  When all parents
 * of the returned proxy are freed, the returned proxy will also be
 * freed.
 *
 * Returns: new NihDBusProxy structure on success, or NULL if
 * insufficient memory.
 **/
NihDBusProxy *
nih_dbus_proxy_new (const void *    parent,
		    DBusConnection *conn,
		    const char *    name,
		    const char *    path)
{
	NihDBusProxy *proxy;

	nih_assert (conn != NULL);
	nih_assert (path != NULL);

	proxy = nih_new (parent, NihDBusProxy);
	if (! proxy)
		return NULL;

	proxy->name = NULL;
	if (name) {
		proxy->name = nih_strdup (proxy, name);
		if (! proxy->name) {
			nih_free (proxy);
			return NULL;
		}
	}

	proxy->path = nih_strdup (proxy, path);
	if (! proxy->path) {
		nih_free (proxy);
		return NULL;
	}

	proxy->conn = conn;

	return proxy;
}
