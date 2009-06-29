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

int        interface_name_valid                    (const char *name);

Interface *interface_new                           (const void *parent,
						    const char *name)
	__attribute__ ((warn_unused_result, malloc));

int        interface_start_tag                     (XML_Parser xmlp,
						    const char *tag,
						    char * const *attr)
	__attribute__ ((warn_unused_result));
int        interface_end_tag                       (XML_Parser xmlp,
						    const char *tag)
	__attribute__ ((warn_unused_result));

int        interface_annotation                    (Interface *interface,
						    const char *name,
						    const char *value)
	__attribute__ ((warn_unused_result));

char *     interface_methods_array                 (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    int with_handlers,
						    NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *     interface_signals_array                 (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    int with_filters,
						    NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));
char *     interface_properties_array              (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    int with_handlers,
						    NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

char *     interface_struct                        (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    int object,
						    NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));


char *     interface_proxy_get_all_function        (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    NihList *prototypes,
						    NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *     interface_proxy_get_all_notify_function (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    NihList *prototypes,
						    NihList *typedefs,
						    NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *     interface_proxy_get_all_sync_function   (const void *parent,
						    const char *prefix,
						    Interface *interface,
						    NihList *prototypes,
						    NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_INTERFACE_H */
