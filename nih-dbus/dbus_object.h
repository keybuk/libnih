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
