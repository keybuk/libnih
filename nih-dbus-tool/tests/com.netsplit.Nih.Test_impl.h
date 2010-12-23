/* libnih
 *
 * Copyright © 2010 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2010 Canonical Ltd.
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

#ifndef DBUS__COM_NETSPLIT_NIH_TEST_IMPL_H
#define DBUS__COM_NETSPLIT_NIH_TEST_IMPL_H

#include <dbus/dbus.h>

#include <nih/macros.h>

#include <nih-dbus/dbus_message.h>


typedef struct my_struct {
	char *   item0;
	uint32_t item1;
} MyStruct;


NIH_BEGIN_EXTERN

extern int             async_method_main_loop;
extern char *          async_method_input;
extern NihDBusMessage *async_method_message;

extern uint8_t    byte_property;
extern int        boolean_property;
extern int16_t    int16_property;
extern uint16_t   uint16_property;
extern int32_t    int32_property;
extern uint32_t   uint32_property;
extern int64_t    int64_property;
extern uint64_t   uint64_property;
extern double     double_property;
extern char *     str_property;
extern char *     object_path_property;
extern char *     signature_property;
extern MyStruct * struct_property;
extern int32_t *  int32_array_property;
extern size_t     int32_array_property_len;
extern char **    str_array_property;
extern int32_t ** int32_array_array_property;
extern size_t *   int32_array_array_property_len;
extern MyStruct **struct_array_property;
extern MyStruct **dict_entry_array_property;
extern int        unix_fd_property;

NIH_END_EXTERN

#endif /* DBUS__COM_NETSPLIT_NIH_TEST_IMPL_H */
