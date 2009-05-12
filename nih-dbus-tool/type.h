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

#ifndef NIH_DBUS_TOOL_TYPE_H
#define NIH_DBUS_TOOL_TYPE_H

#include <nih/macros.h>
#include <nih/list.h>

#include <dbus/dbus.h>


/**
 * TypeVar:
 * @entry: list entry,
 * @type: C type,
 * @name: variable name.
 *
 * This structure represents a C variable declaration, containing both
 * the type and name of the variable.  They are returned by the marshal()
 * and demarshal() functions via the locals list.
 **/
typedef struct type_var {
	NihList entry;
	char *  type;
	char *  name;
} TypeVar;

/**
 * TypeFunc:
 * @entry: list entry,
 * @type: C function return type,
 * @name: function name,
 * @args: function arguments,
 * @attribs: attributes.
 *
 * This structure represents a C function definition or prototype,
 * containing its return type, name, arguments and any function attributes
 * for compiler hints.  They are returned by the various function
 * generators via the prototypes or externs lists.
 *
 * @args is a list of TypeVar entries, @attribs is a list of NihListEntry
 * entries with strings as the data pointer.
 **/
typedef struct type_func {
	NihList entry;
	char *  type;
	char *  name;
	NihList args;
	NihList attribs;
} TypeFunc;


NIH_BEGIN_EXTERN

const char *type_const          (int dbus_type);
const char *type_basic_type     (int dbus_type);

char *      type_of             (const void * parent, DBusSignatureIter *iter);

TypeVar *   type_var_new        (const void *parent, const char *type,
				 const char *name)
	__attribute__ ((malloc, warn_unused_result));
char *      type_var_to_string  (const void *parent, TypeVar *var)
	__attribute__ ((malloc, warn_unused_result));
char *      type_var_layout     (const void *parent, NihList *vars)
	__attribute__ ((malloc, warn_unused_result));

TypeFunc *  type_func_new       (const void *parent, const char *type,
				 const char *name)
	__attribute__ ((malloc, warn_unused_result));
char *      type_func_to_string (const void *parent, TypeFunc *func)
	__attribute__ ((malloc, warn_unused_result));
char *      type_func_layout    (const void *parent, NihList *funcs)
	__attribute__ ((malloc, warn_unused_result));

char *      type_to_const       (char **type, const void *parent)
	__attribute__ ((malloc, warn_unused_result));
char *      type_to_pointer     (char **type, const void *parent)
	__attribute__ ((malloc, warn_unused_result));
char *      type_to_static      (char **type, const void *parent)
	__attribute__ ((malloc, warn_unused_result));
char *      type_to_extern      (char **type, const void *parent)
	__attribute__ ((malloc, warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TYPE_H */
