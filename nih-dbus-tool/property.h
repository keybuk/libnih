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

#ifndef NIH_DBUS_TOOL_PROPERTY_H
#define NIH_DBUS_TOOL_PROPERTY_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include <nih-dbus/dbus_object.h>

#include "interface.h"


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

Property *property_lookup                    (Interface *interface,
					      const char *symbol);

char *    property_object_get_function       (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *handlers,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));
char *    property_object_set_function       (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *handlers,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    property_proxy_get_function        (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));
char *    property_proxy_get_notify_function (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *typedefs,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    property_proxy_set_function        (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));
char *    property_proxy_set_notify_function (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *typedefs,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    property_proxy_get_sync_function   (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));
char *    property_proxy_set_sync_function   (const void *parent,
					      const char *prefix,
					      Interface *interface,
					      Property *property,
					      NihList *prototypes,
					      NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_PROPERTY_H */
