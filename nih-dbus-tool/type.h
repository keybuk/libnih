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
	__attribute__ ((warn_unused_result, malloc));
char *      type_var_to_string  (const void *parent, TypeVar *var)
	__attribute__ ((warn_unused_result, malloc));
char *      type_var_layout     (const void *parent, NihList *vars)
	__attribute__ ((warn_unused_result, malloc));

TypeFunc *  type_func_new       (const void *parent, const char *type,
				 const char *name)
	__attribute__ ((warn_unused_result, malloc));
char *      type_func_to_string (const void *parent, TypeFunc *func)
	__attribute__ ((warn_unused_result, malloc));
char *      type_func_layout    (const void *parent, NihList *funcs)
	__attribute__ ((warn_unused_result, malloc));

char *      type_to_const       (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char *      type_to_pointer     (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char *      type_to_static      (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));
char *      type_to_extern      (char **type, const void *parent)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_TYPE_H */
