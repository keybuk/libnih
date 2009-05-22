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

#ifndef NIH_DBUS_TOOL_METHOD_H
#define NIH_DBUS_TOOL_METHOD_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

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

Argument *method_lookup_argument (Method *method, const char *symbol);

char *    method_object_function       (const void *parent, Method *method,
					const char *name,
					const char *handler_name,
					NihList *prototypes, NihList *handlers)
	__attribute__ ((warn_unused_result, malloc));

char *    method_reply_function        (const void *parent, Method *method,
					const char *name,
					NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

char *    method_proxy_function        (const void *parent,
					const char *interface_name,
					Method *method, const char *name,
					const char *notify_name,
					const char *handler_type,
					NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

char *    method_proxy_notify_function (const void *parent,
					Method *method, const char *name,
					const char *handler_type,
					NihList *prototypes, NihList *typedefs)
	__attribute__ ((warn_unused_result, malloc));

char *    method_proxy_sync_function   (const void *parent,
					const char *interface_name,
					Method *method, const char *name,
					NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_METHOD_H */
