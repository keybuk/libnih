/* nih-dbus-tool
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
