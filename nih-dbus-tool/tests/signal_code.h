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

#ifndef NIH_DBUS_TOOL_TESTS_SIGNAL_CODE_H
#define NIH_DBUS_TOOL_TESTS_SIGNAL_CODE_H

#include <nih/macros.h>

#include <dbus/dbus.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_object.h>


typedef void (*MySignalHandler) (void *data, NihDBusMessage *message,
				 const char *msg);


NIH_BEGIN_EXTERN

int               my_emit_signal   (DBusConnection *connection,
				    const char *origin_path, const char *msg)
	__attribute__ ((warn_unused_result));

DBusHandlerResult my_com_netsplit_Nih_Test_Signal_signal (DBusConnection *connection,
							  DBusMessage *signal,
							  NihDBusProxySignal *proxied);

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TESTS_SIGNAL_CODE_H */
