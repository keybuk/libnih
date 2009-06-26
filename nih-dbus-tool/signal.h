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

#ifndef NIH_DBUS_TOOL_SIGNAL_H
#define NIH_DBUS_TOOL_SIGNAL_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include "interface.h"
#include "argument.h"


/**
 * Signal:
 * @entry: list header,
 * @name: D-Bus name of signal,
 * @symbol: name used when constructing C name,
 * @deprecated: whether this signal is deprecated,
 * @arguments: arguments provided by the signal.
 *
 * D-Bus interfaces specify zero or more signals, which are identified by
 * @name over the bus and may have zero or more @arguments.
 *
 * When generating the C symbol names @symbol will be used.  If @symbol
 * is NULL, @name will be converted into the usual C lowercase and underscore
 * style and used instead.
 **/
typedef struct signal {
	NihList entry;
	char *  name;
	char *  symbol;
	int     deprecated;
	NihList arguments;
} Signal;


NIH_BEGIN_EXTERN

int       signal_name_valid      (const char *name);

Signal *  signal_new             (const void *parent, const char *name)
	__attribute__ ((warn_unused_result, malloc));

int       signal_start_tag       (XML_Parser xmlp, const char *tag,
				  char * const *attr)
	__attribute__ ((warn_unused_result));
int       signal_end_tag         (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int       signal_annotation      (Signal *signal,
				  const char *name, const char *value)
	__attribute__ ((warn_unused_result));

Signal *  signal_lookup          (Interface *interface, const char *symbol);
Argument *signal_lookup_argument (Signal *signal, const char *symbol);

char *    signal_object_function (const void *parent, const char *prefix,
				  Interface *interface, Signal *signal,
				  NihList *prototypes, NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    signal_proxy_function  (const void *parent, const char *prefix,
				  Interface *interface, Signal *signal,
				  NihList *prototypes, NihList *typedefs,
				  NihList *structs)
	__attribute__ ((warn_unused_result, malloc));

char *    signal_args_array      (const void *parent, const char *prefix,
				  Interface *interface, Signal *signal,
				  NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_SIGNAL_H */
