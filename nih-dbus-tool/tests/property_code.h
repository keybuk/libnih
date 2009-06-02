/* nih-dbus-tool
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

#ifndef NIH_DBUS_TOOL_TESTS_PROPERTY_CODE_H
#define NIH_DBUS_TOOL_TESTS_PROPERTY_CODE_H

#include <nih/macros.h>

#include <dbus/dbus.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_object.h>
#include <nih-dbus/dbus_pending_data.h>
#include <nih-dbus/dbus_proxy.h>


typedef void (*MyGetPropertyReply) (void *data, NihDBusMessage *message,
				    const char *value);
typedef void (*MySetPropertyReply) (void *data, NihDBusMessage *message);

typedef void (*MyGetTestPropertyReply) (void *data, NihDBusMessage *message,
					const char *value);
typedef void (*MySetTestPropertyReply) (void *data, NihDBusMessage *message);


NIH_BEGIN_EXTERN

int              my_com_netsplit_Nih_Test_property_get (NihDBusObject *object,
							NihDBusMessage *message,
							DBusMessageIter *iter)
	__attribute__ ((warn_unused_result));
int              my_com_netsplit_Nih_Test_property_set (NihDBusObject *object,
							NihDBusMessage *message,
							DBusMessageIter *iter)
	__attribute__ ((warn_unused_result));

DBusPendingCall *my_get_test_property (NihDBusProxy *proxy,
				       MyGetTestPropertyReply handler,
				       NihDBusErrorHandler error_handler,
				       void *data, int timeout)
	__attribute__ ((warn_unused_result));

void             my_com_netsplit_Nih_Test_property_get_notify (DBusPendingCall *pending_call,
							       NihDBusPendingData *pending_data);

DBusPendingCall *my_set_test_property (NihDBusProxy *proxy,
				       const char *value,
				       MySetTestPropertyReply handler,
				       NihDBusErrorHandler error_handler,
				       void *data, int timeout)
	__attribute__ ((warn_unused_result));

void             my_com_netsplit_Nih_Test_property_set_notify (DBusPendingCall *pending_call,
							       NihDBusPendingData *pending_data);

int              my_get_property_sync (const void *parent,
				       NihDBusProxy *proxy, char **value)
	__attribute__ ((warn_unused_result));
int              my_set_property_sync (NihDBusProxy *proxy,
				       const char *value)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TESTS_PROPERTY_CODE_H */
