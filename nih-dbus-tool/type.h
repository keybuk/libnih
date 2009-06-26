/* nih-dbus-tool
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
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

#ifndef NIH_DBUS_TOOL_TYPE_H
#define NIH_DBUS_TOOL_TYPE_H

#include <nih/macros.h>
#include <nih/list.h>

#include <dbus/dbus.h>


/**
 * TypeVar:
 * @entry: list entry,
 * @type: C type,
 * @name: variable name,
 * @array: TRUE of the variable is an array.
 *
 * This structure represents a C variable declaration, containing both
 * the type and name of the variable.  They are returned by the marshal()
 * and demarshal() functions via the locals list.
 **/
typedef struct type_var {
	NihList entry;
	char *  type;
	char *  name;
	int     array;
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

/**
 * TypeStruct:
 * @entry: list entry,
 * @name: structure name,
 * @members: structure members.
 *
 * This structure represents a C structure definition, defined as a
 * typedef called @name with an actual structure name converted from
 * that.
 *
 * @members is a list of TypeVar entries.
 **/
typedef struct type_struct {
	NihList entry;
	char *  name;
	NihList members;
} TypeStruct;


NIH_BEGIN_EXTERN

const char *type_const            (int dbus_type);
const char *type_basic_type       (int dbus_type);

char *      type_of               (const void * parent,
				   DBusSignatureIter *iter)
	__attribute__ ((warn_unused_result, malloc));

TypeVar *   type_var_new          (const void *parent, const char *type,
				   const char *name)
	__attribute__ ((warn_unused_result, malloc));
char *      type_var_to_string    (const void *parent, TypeVar *var)
	__attribute__ ((warn_unused_result, malloc));
char *      type_var_layout       (const void *parent, NihList *vars)
	__attribute__ ((warn_unused_result, malloc));

TypeFunc *  type_func_new         (const void *parent, const char *type,
				   const char *name)
	__attribute__ ((warn_unused_result, malloc));
char *      type_func_to_string   (const void *parent, TypeFunc *func)
	__attribute__ ((warn_unused_result, malloc));
char *      type_func_to_typedef  (const void *parent, TypeFunc *func)
	__attribute__ ((warn_unused_result, malloc));
char *      type_func_layout      (const void *parent, NihList *funcs)
	__attribute__ ((warn_unused_result, malloc));

TypeStruct *type_struct_new       (const void *parent, const char *name)
	__attribute__ ((warn_unused_result, malloc));
char *      type_struct_to_string (const void *parent, TypeStruct *structure)
	__attribute__ ((warn_unused_result, malloc));

char *      type_to_const         (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char *      type_to_pointer       (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char *      type_to_static        (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char *      type_to_extern        (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));

char *      type_strcat_assert    (char **block, const void *parent,
				   TypeVar *var, TypeVar *prev, TypeVar *next)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TYPE_H */
