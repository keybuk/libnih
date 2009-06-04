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

#ifndef NIH_DBUS_TOOL_INTERFACE_H
#define NIH_DBUS_TOOL_INTERFACE_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>


/**
 * Interface:
 * @entry: list header,
 * @name: D-Bus name of interface,
 * @symbol: name used when constructing C name,
 * @deprecated: whether this interface is deprecated,
 * @methods: methods defined by the interface,
 * @signals: signals defined by the interface,
 * @properties: properties defined by the interface.
 *
 * D-Bus objects implement zero or more interfaces defining the @methods,
 * @signals and @properties available.  Interfaces are selected by @name
 * over the bus.
 *
 * When generating the C symbol names @symbol will be used.  If @symbol
 * is NULL, and the interface is not the first for the object, as many
 * final components of @name required to ensure uniqueness will be used.
 **/
typedef struct interface {
	NihList entry;
	char *  name;
	char *  symbol;
	int     deprecated;
	NihList methods;
	NihList signals;
	NihList properties;
} Interface;


NIH_BEGIN_EXTERN

int        interface_name_valid       (const char *name);

Interface *interface_new              (const void *parent, const char *name)
	__attribute__ ((warn_unused_result, malloc));

int        interface_start_tag        (XML_Parser xmlp, const char *tag,
				       char * const *attr)
	__attribute__ ((warn_unused_result));
int        interface_end_tag          (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int        interface_annotation       (Interface *interface,
				       const char *name, const char *value)
	__attribute__ ((warn_unused_result));

char *     interface_methods_array    (const void *parent, const char *prefix,
				       Interface *interface, int with_handlers,
				       NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *     interface_signals_array    (const void *parent, const char *prefix,
				       Interface *interface, int with_filters,
				       NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *     interface_properties_array (const void *parent, const char *prefix,
				       Interface *interface, int with_handlers,
				       NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

char *     interface_struct           (const void *parent, const char *prefix,
				       Interface *interface, int object,
				       NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_INTERFACE_H */
