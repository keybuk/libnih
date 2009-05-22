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

#ifndef NIH_DBUS_TOOL_TESTS_DEMARSHAL_CODE_H
#define NIH_DBUS_TOOL_TESTS_DEMARSHAL_CODE_H

#include <nih/macros.h>

#include <dbus/dbus.h>

#include <stdint.h>

struct dbus_struct_suasan {
	char *    item0;
	uint32_t  item1;
	char **   item2;
	int16_t * item3;
	size_t    item3_len;
};

struct dbus_struct_su {
	char *    item0;
	uint32_t  item1;
};


NIH_BEGIN_EXTERN

int my_byte_demarshal               (const void *parent, DBusMessage *message,
				     uint8_t *value);
int my_boolean_demarshal            (const void *parent, DBusMessage *message,
				     int *value);
int my_int16_demarshal              (const void *parent, DBusMessage *message,
				     int16_t *value);
int my_uint16_demarshal             (const void *parent, DBusMessage *message,
				     uint16_t *value);
int my_int32_demarshal              (const void *parent, DBusMessage *message,
				     int32_t *value);
int my_uint32_demarshal             (const void *parent, DBusMessage *message,
				     uint32_t *value);
int my_int64_demarshal              (const void *parent, DBusMessage *message,
				     int64_t *value);
int my_uint64_demarshal             (const void *parent, DBusMessage *message,
				     uint64_t *value);
int my_double_demarshal             (const void *parent, DBusMessage *message,
				     double *value);
int my_string_demarshal             (const void *parent, DBusMessage *message,
				     char **value);
int my_object_path_demarshal        (const void *parent, DBusMessage *message,
				     char **value);
int my_signature_demarshal          (const void *parent, DBusMessage *message,
				     char **value);
int my_int16_array_demarshal        (const void *parent, DBusMessage *message,
				     int16_t **value,
				     size_t *value_len);
int my_int16_array_array_demarshal  (const void *parent, DBusMessage *message,
				     int16_t ***value,
				     size_t **value_len);
int my_string_array_demarshal       (const void *parent, DBusMessage *message,
				     char ***value);
int my_string_array_array_demarshal (const void *parent, DBusMessage *message,
				     char ****value);
int my_struct_demarshal             (const void *parent, DBusMessage *message,
				     struct dbus_struct_suasan **value);
int my_struct_array_demarshal       (const void *parent, DBusMessage *message,
				     struct dbus_struct_su ***value);

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TESTS_DEMARSHAL_CODE_H */
