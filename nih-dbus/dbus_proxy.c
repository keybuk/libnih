/* libnih
 *
 * dbus_proxy.c - D-Bus remote object proxy implementation
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
