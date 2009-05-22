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


/**
 * NihDBusProxy:
 * @name: D-Bus name of object owner,
 * @path: path of object,
 * @conn: associated connection.
 *
 * Instances of this structure may be created for remote objects that you
 * wish to use.  Fundamentally they combine the three elements of data
 * necessary into one easy object that is bound to the lifetime of the
 * associated connection.
 **/
typedef struct nih_dbus_proxy {
	char *          name;
	char *          path;
	DBusConnection *conn;
} NihDBusProxy;


NIH_BEGIN_EXTERN

NihDBusProxy *nih_dbus_proxy_new (const void *parent, DBusConnection *conn,
				  const char *name, const char *path)
	__attribute__ ((malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_PROXY_H */
