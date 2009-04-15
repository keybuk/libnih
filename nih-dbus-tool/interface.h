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

#ifndef NIH_DBUS_TOOL_INTERFACE_H
#define NIH_DBUS_TOOL_INTERFACE_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include "method.h"
#include "signal.h"
#include "property.h"


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

int        interface_name_valid      (const char *name);

Interface *interface_new             (const void *parent, const char *name)
	__attribute__ ((malloc, warn_unused_result));

int        interface_start_tag       (XML_Parser xmlp, const char *tag,
				       char * const *attr)
	__attribute__ ((warn_unused_result));
int        interface_end_tag         (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int        interface_annotation      (Interface *interface,
				       const char *name, const char *value)
	__attribute__ ((warn_unused_result));

Method *   interface_lookup_method   (Interface *interface,
				      const char *symbol);
Signal *   interface_lookup_signal   (Interface *interface,
				      const char *symbol);
Property * interface_lookup_property (Interface *interface,
				      const char *symbol);

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_INTERFACE_H */
