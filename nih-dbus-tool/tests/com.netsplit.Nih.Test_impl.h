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

#ifndef DBUS__COM_NETSPLIT_NIH_TEST_IMPL_H
#define DBUS__COM_NETSPLIT_NIH_TEST_IMPL_H

#include <dbus/dbus.h>

#include <nih/macros.h>

#include <nih-dbus/dbus_message.h>


NIH_BEGIN_EXTERN

extern int             async_method_main_loop;
extern char *          async_method_input;
extern NihDBusMessage *async_method_message;

extern uint8_t   byte_property;
extern int       boolean_property;
extern int16_t   int16_property;
extern uint16_t  uint16_property;
extern int32_t   int32_property;
extern uint32_t  uint32_property;
extern int64_t   int64_property;
extern uint64_t  uint64_property;
extern double    double_property;
extern char *    str_property;
extern char *    object_path_property;
extern char *    signature_property;
extern int32_t * int32_array_property;
extern size_t    int32_array_property_len;
extern char **   str_array_property;
extern int32_t **int32_array_array_property;
extern size_t *  int32_array_array_property_len;

NIH_END_EXTERN

#endif /* DBUS__COM_NETSPLIT_NIH_TEST_IMPL_H */
