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

#ifndef NIH_DBUS_TOOL_PARSE_H
#define NIH_DBUS_TOOL_PARSE_H

#include <nih/macros.h>
#include <nih/list.h>

#include "node.h"
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "argument.h"


/**
 * ParseStackType:
 *
 * Type of parsed object on the stack.
 **/
typedef enum parse_stack_type {
	PARSE_IGNORED,
	PARSE_NODE,
	PARSE_INTERFACE,
	PARSE_METHOD,
	PARSE_SIGNAL,
	PARSE_PROPERTY,
	PARSE_ARGUMENT,
	PARSE_ANNOTATION,
} ParseStackType;

/**
 * ParseStack:
 * @entry: list header,
 * @type: type of object parsed,
 * @data: object pointer.
 *
 * This structure represents an object parsed from the XML file and is used
 * as a stack.  @type indicates which type of object is being parsed, and
 * the @data pointer is part of a union of the different types of pointer.
 **/
typedef struct parse_stack {
	NihList            entry;
	ParseStackType     type;
	union {
		void *     data;
		Node *     node;
		Interface *interface;
		Method *   method;
		Signal *   signal;
		Property * property;
		Argument * argument;
	};
} ParseStack;


/**
 * ParseContext:
 * @parent: parent for node,
 * @stack: parse stack,
 * @filename: filename being parsed,
 * @node: top-level node.
 *
 * This structure is used as the user data for the XML parser, it tracks the
 * stack of objects being parsed and returns the top-level node object which
 * has all of the interfaces, etc.
 **/
typedef struct parse_context {
	const void *parent;
	NihList     stack;
	const char *filename;
	Node *      node;
} ParseContext;


NIH_BEGIN_EXTERN

ParseStack *parse_stack_push (const void *parent, NihList *stack,
			      ParseStackType type, void *data)
	__attribute__ ((warn_unused_result, malloc));
ParseStack *parse_stack_top  (NihList *stack);

void        parse_start_tag  (XML_Parser xmlp, const char *tag,
			      char * const *attr);
void        parse_end_tag    (XML_Parser xmlp, const char *tag);

Node *      parse_xml        (const void *parent, int fd, const char *filename)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_PARSE_H */
