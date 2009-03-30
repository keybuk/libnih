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
	char    *path;
	NihList  interfaces;
} Node;


NIH_BEGIN_EXTERN

int        node_path_valid           (const char *name);

Node *     node_new                  (const void *parent, const char *path)
	__attribute__ ((malloc, warn_unused_result));

int        node_start_tag            (XML_Parser xmlp, const char *tag,
				 char * const *attr)
	__attribute__ ((warn_unused_result));
int        node_end_tag              (XML_Parser xmlp, const char *tag)
	__attribute__ ((warn_unused_result));

Interface *node_lookup_interface     (Node *node, const char *symbol);

NIH_END_EXTERN

#endif /* NIH_DBUS_TOOL_NODE_H */
