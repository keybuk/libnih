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

#ifndef NIH_DBUS_TOOL_METHOD_H
#define NIH_DBUS_TOOL_METHOD_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include "interface.h"
#include "argument.h"


/**
 * Method:
 * @entry: list header,
 * @name: D-Bus name of method,
 * @symbol: name used when constructing C name,
 * @deprecated: whether this method is deprecated,
 * @async: whether the object implementation should be asynchronous,
 * @no_reply: TRUE if no reply should be expected or generated,
 * @arguments: arguments accepted by the method.
 *
 * D-Bus interfaces specify zero or more methods, which are selected by
 * @name over the bus and may have zero or more @arguments.
 *
 * When generating the C symbol names @symbol will be used.  If @symbol
 * is NULL, @name will be converted into the usual C lowercase and underscore
 * style and used instead.
 **/
typedef struct method {
	NihList entry;
	char *  name;
	char *  symbol;
	int     deprecated;
	int     async;
	int     no_reply;
	NihList arguments;
} Method;


NIH_BEGIN_EXTERN

int       method_name_valid            (const char *name);

Method *  method_new                   (const void *parent, const char *name)
	__attribute__ ((warn_unused_result, malloc));

int       method_start_tag             (XML_Parser xmlp, const char *tag,
					char * const *attr)
	__attribute__ ((warn_unused_result));
int       method_end_tag               (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int       method_annotation            (Method *method,
					const char *name, const char *value)
	__attribute__ ((warn_unused_result));

Method *  method_lookup                (Interface *interface,
					const char *symbol);
Argument *method_lookup_argument       (Method *method, const char *symbol);

char *    method_object_function       (const void *parent, const char *prefix,
					Interface *interface, Method *method,
					NihList *prototypes, NihList *handlers,
					NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    method_reply_function        (const void *parent, const char *prefix,
					Interface *interface, Method *method,
					NihList *prototypes, NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    method_proxy_function        (const void *parent, const char *prefix,
					Interface *interface, Method *method,
					NihList *prototypes, NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    method_proxy_notify_function (const void *parent, const char *prefix,
					Interface *interface, Method *method,
					NihList *prototypes, NihList *typedefs,
					NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    method_proxy_sync_function   (const void *parent, const char *prefix,
					Interface *interface, Method *method,
					NihList *prototypes, NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    method_args_array            (const void *parent, const char *prefix,
					Interface *interface, Method *method,
					NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_METHOD_H */
