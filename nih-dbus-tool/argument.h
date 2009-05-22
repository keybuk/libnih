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
