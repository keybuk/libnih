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

#ifndef NIH_DBUS_TOOL_SIGNAL_H
#define NIH_DBUS_TOOL_SIGNAL_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

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
	__attribute__ ((malloc, warn_unused_result));

int       signal_start_tag       (XML_Parser xmlp, const char *tag,
				  char * const *attr)
	__attribute__ ((warn_unused_result));
int       signal_end_tag         (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int       signal_annotation      (Signal *signal,
				  const char *name, const char *value)
	__attribute__ ((warn_unused_result));

Argument *signal_lookup_argument (Signal *signal, const char *symbol);

char *    signal_emit_function   (const void *parent,
				  const char *interface_name, Signal *signal,
				  const char *name,
				  NihList *prototypes, NihList *externs)
	__attribute__ ((malloc, warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_SIGNAL_H */
