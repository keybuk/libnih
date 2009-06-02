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

#ifndef NIH_DBUS_TOOL_TESTS_METHOD_CODE_H
#define NIH_DBUS_TOOL_TESTS_METHOD_CODE_H

#include <nih/macros.h>

#include <dbus/dbus.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_object.h>
#include <nih-dbus/dbus_pending_data.h>


typedef void (*MyMethodReply) (void *data, NihDBusMessage *message,
			       char * const *output, int32_t length);
typedef void (*MyTestMethodReply) (void *data, NihDBusMessage *message,
				   char * const *output, int32_t length);


NIH_BEGIN_EXTERN

DBusHandlerResult my_com_netsplit_Nih_Test_Method_method      (NihDBusObject *object,
							       NihDBusMessage *message);

DBusHandlerResult my_com_netsplit_Nih_Test_AsyncMethod_method (NihDBusObject *object,
							       NihDBusMessage *message);

int               my_async_method_reply (NihDBusMessage *message,
					 char * const *output)
	__attribute__ ((warn_unused_result));

DBusPendingCall * my_test_method        (NihDBusProxy *proxy,
					 const char *str, int32_t flags,
					 MyTestMethodReply handler,
					 NihDBusErrorHandler error_handler,
					 void *data, int timeout)
	__attribute__ ((warn_unused_result));

void              my_method_notify      (DBusPendingCall *pending_call,
					 NihDBusPendingData *pending_data);

int               my_method_sync        (const void *parent,
					 NihDBusProxy *proxy,
					 const char *str, int32_t flags,
					 char ***output, int32_t *length)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TESTS_METHOD_CODE_H */
