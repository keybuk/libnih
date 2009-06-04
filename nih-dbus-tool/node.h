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

#ifndef NIH_DBUS_TOOL_NODE_H
#define NIH_DBUS_TOOL_NODE_H

#include <expat.h>

#include <nih/macros.h>
#include <nih/list.h>

#include "interface.h"


/**
 * Node:
 * @path: D-Bus path of node,
 * @interfaces: interfaces the node implements.
 *
 * A node is the top-level tag in D-Bus introspection data and represents
 * a specific D-Bus object with the path given in @path.
 **/
typedef struct node {
	char *  path;
	NihList interfaces;
} Node;


NIH_BEGIN_EXTERN

int        node_path_valid       (const char *name);

Node *     node_new              (const void *parent, const char *path)
	__attribute__ ((warn_unused_result, malloc));

int        node_start_tag        (XML_Parser xmlp, const char *tag,
				  char * const *attr)
	__attribute__ ((warn_unused_result));
int        node_end_tag          (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

Interface *node_lookup_interface (Node *node, const char *symbol);

char *     node_interfaces_array (const void *parent, const char *prefix,
				  Node *node, int object, NihList *prototypes)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_NODE_H */
