/* libnih
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

#ifndef NIH_DBUS_OBJECT_H
#define NIH_DBUS_OBJECT_H

#include <nih/macros.h>

#include <nih-dbus/dbus_interface.h>

#include <dbus/dbus.h>


/**
 * NihDBusObject:
 * @path: path of object,
 * @connection: associated connection,
 * @data: pointer to object data,
 * @interfaces: NULL-terminated array of interfaces the object supports,
 * @registered: TRUE while the object is registered.
 *
 * This structure represents an object visible on the given @connection
 * at @path and being handled by libnih-dbus.  It connects the @data
 * pointer to the individual method and property handler functions
 * defined by the @interfaces.
 *
 * Automatic introspection is provided based on @interfaces.
 *
 * No reference is held to @connection, therefore you may not assume that
 * it is valid.  In general, the object will be automatically freed should
 * @connection be cleaned up.
 **/
struct nih_dbus_object {
	char *                   path;
	DBusConnection *         connection;
	void *                   data;
	const NihDBusInterface **interfaces;
	int                      registered;
};


NIH_BEGIN_EXTERN

NihDBusObject *nih_dbus_object_new (const void *parent,
				    DBusConnection *connection,
				    const char *path,
				    const NihDBusInterface **interfaces,
				    void *data)
	__attribute__ ((malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_OBJECT_H */
