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
