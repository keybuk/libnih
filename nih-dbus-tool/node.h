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

char *     node_object_functions (const void *parent, const char *prefix,
				  Node *node,
				  NihList *prototypes, NihList *handlers,
				  NihList *structs, NihList *externs)
	__attribute__ ((warn_unused_result, malloc));
char *     node_proxy_functions  (const void *parent, const char *prefix,
				  Node *node,
				  NihList *prototypes, NihList *structs,
				  NihList *typedefs, NihList *externs)
	__attribute__ ((warn_unused_result, malloc));

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_NODE_H */
