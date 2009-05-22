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

#ifndef NIH_DBUS_TOOL_PROPERTY_H
#define NIH_DBUS_TOOL_PROPERTY_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include <nih-dbus/dbus_object.h>


/**
 * Property:
 * @entry: list header,
 * @name: D-Bus name of property,
 * @symbol: name used when constructing C name,
 * @type: type signature of property,
 * @access: access of property,
 * @deprecated: whether this property is deprecated.
 *
 * D-Bus interfaces specify zero or more properties, which are identified by
 * @name over the bus and have the type signature @type.  Properties may be
 * read-only, write-only or read/write depending on @access.
 *
 * When generating the C symbol names @symbol will be used.  If @symbol
 * is NULL, @name will be converted into the usual C lowercase and underscore
 * style and used instead.
 **/
typedef struct property {
	NihList       entry;
	char *        name;
	char *        symbol;
	char *        type;
	NihDBusAccess access;
	int           deprecated;
} Property;


NIH_BEGIN_EXTERN

int       property_name_valid                (const char *name);

Property *property_new                       (const void *parent,
					      const char *name,
					      const char *type,
					      NihDBusAccess access)
	__attribute__ ((warn_unused_result, malloc));

int       property_start_tag                 (XML_Parser xmlp, const char *tag,
					      char * const *attr)
	__attribute__ ((warn_unused_result));
int       property_end_tag                   (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int       property_annotation                (Property *property,
					      const char *name,
					      const char *value)
	__attribute__ ((warn_unused_result));

char *    property_object_get_function       (const void *parent,
					      Property *property,
					      const char *name,
					      const char *handler_name,
					      NihList *prototypes,
					      NihList *handlers)
	__attribute__ ((warn_unused_result, malloc));
char *    property_object_set_function       (const void *parent,
					      Property *property,
					      const char *name,
					      const char *handler_name,
					      NihList *prototypes,
					      NihList *handlers)
	__attribute__ ((warn_unused_result, malloc));

char *    property_proxy_get_function        (const void *parent,
					      const char *interface_name,
					      Property *property,
					      const char *name,
					      const char *notify_name,
					      const char *handler_type,
					      NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *    property_proxy_get_notify_function (const void *parent,
					      Property *property,
					      const char *name,
					      const char *handler_type,
					      NihList *prototypes,
					      NihList *typedefs)
	__attribute__ ((warn_unused_result, malloc));

char *    property_proxy_set_function        (const void *parent,
					      const char *interface_name,
					      Property *property,
					      const char *name,
					      const char *notify_name,
					      const char *handler_type,
					      NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *    property_proxy_set_notify_function (const void *parent,
					      Property *property,
					      const char *name,
					      const char *handler_type,
					      NihList *prototypes,
					      NihList *typedefs)
	__attribute__ ((warn_unused_result, malloc));

char *    property_proxy_get_sync_function   (const void *parent,
					      const char *interface_name,
					      Property *property,
					      const char *name,
					      NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *    property_proxy_set_sync_function   (const void *parent,
					      const char *interface_name,
					      Property *property,
					      const char *name,
					      NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_PROPERTY_H */
