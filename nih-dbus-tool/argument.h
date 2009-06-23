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

#ifndef NIH_DBUS_TOOL_ARGUMENT_H
#define NIH_DBUS_TOOL_ARGUMENT_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include <nih-dbus/dbus_object.h>


/**
 * Argument:
 * @entry: list header,
 * @name: D-Bus name of argument,
 * @symbol: name used when constructing C name,
 * @type: type signature of argument,
 * @direction: direction of argument.
 *
 * D-Bus methods and signals specify zero or more arguments, which are
 * identified by an optional @name over the bus and have the type signature
 * @type.  Arguments may be either input to the method or output from it
 * depending on @direction.
 *
 * When generating the C symbol names @symbol will be used.  If @symbol
 * is NULL, @name will be converted into the usual C lowercase and underscore
 * style and used instead.  If @name is NULL, then a simple "arg1" form is
 * used.
 **/
typedef struct argument {
	NihList       entry;
	char *        name;
	char *        symbol;
	char *        type;
	NihDBusArgDir direction;
} Argument;


NIH_BEGIN_EXTERN

int       argument_name_valid (const char *name);

Argument *argument_new        (const void *parent, const char *name,
			       const char *type, NihDBusArgDir direction)
	__attribute__ ((warn_unused_result, malloc));

int       argument_start_tag  (XML_Parser xmlp, const char *tag,
			       char * const *attr)
	__attribute__ ((warn_unused_result));
int       argument_end_tag    (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

int       argument_annotation (Argument *argument,
			       const char *name, const char *value)
	__attribute__ ((warn_unused_result));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_ARGUMENT_H */
