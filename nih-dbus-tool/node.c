/* nih-dbus-tool
 *
 * node.c - top-level object parsing and handling
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <expat.h>

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "node.h"
#include "interface.h"
#include "parse.h"
#include "errors.h"


/**
 * node_path_valid:
 * @path: Object path to verify.
 *
 * Verifies whether @path matches the specification for D-Bus object paths.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
node_path_valid (const char *path)
{
	nih_assert (path != NULL);

	/* Path must begin with a '/' character */
	if (path[0] != '/')
		return FALSE;

	/* We can get away with just using strlen() here even through path
	 * is in UTF-8 because all the valid characters are ASCII.
	 */
	for (size_t i = 1; i < strlen (path); i++) {
		/* Path components may be separated by single '/' characters,
		 * multiple ones are not allowed.
		 */
		if (path[i] == '/') {
			if (path[i-1] == '/')
				return FALSE;

			continue;
		}

		/* Valid characters are [A-Za-z0-9_] */
		if (   ((path[i] < 'A') || (path[i] > 'Z'))
		    && ((path[i] < 'a') || (path[i] > 'z'))
		    && ((path[i] < '0') || (path[i] > '9'))
		    && (path[i] != '_'))
			return FALSE;
	}

	/* Final character may not be '/' unless it's the root object. */
	if ((strlen (path) > 1) && (path[strlen (path) - 1] == '/'))
		return FALSE;

	return TRUE;
}


/**
 * node_new:
 * @parent: parent object for new node,
 * @path: D-Bus path of node.
 *
 * Allocates a new D-Bus object Node data structure, with the path optionally
 * set to @path.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned node.  When all parents
 * of the returned node are freed, the returned node will also be
 * freed.
 *
 * Returns: the new node or NULL if the allocation failed.
 **/
Node *
node_new (const void *parent,
	  const char *path)
{
	Node *node;

	node = nih_new (parent, Node);
	if (! node)
		return NULL;

	if (path) {
		node->path = nih_strdup (node, path);
		if (! node->path) {
			nih_free (node);
			return NULL;
		}
	} else {
		node->path = NULL;
	}

	nih_list_init (&node->interfaces);

	return node;
}


/**
 * node_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for a "node" start
 * tag, the top-level of the introspection data and defining a D-Bus
 * object.
 *
 * If the node does not appear at the top-level a warning is emitted
 * (unless directly inside another node tag) and the tag will be ignored.
 *
 * Nodes may have a "name" attribute containing the D-Bus object path
 * of the node.
 *
 * Any unknown attributes result in a warning and will be ignored.
 *
 * A Node object will be allocated and pushed onto the stack, this is not
 * saved into the context until the end tag is found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
node_start_tag (XML_Parser    xmlp,
		const char   *tag,
		char * const *attr)
{
	ParseContext *context;
	ParseStack   *parent;
	Node         *node;
	char * const *key;
	char * const *value;
	const char   *name = NULL;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Nodes should only appear at the top-level, unless they're within
	 * another node in which case we just ignore them.
	 */
	parent = parse_stack_top (&context->stack);
	if (parent) {
		if (parent->type != PARSE_NODE) {
			nih_warn ("%s:%zi:%zi: %s", context->filename,
				  XML_GetCurrentLineNumber (xmlp),
				  XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unexpected <node> tag"));
		}

		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL))
			nih_return_system_error (-1);

		return 0;
	}

	/* Retrieve the name from the attributes */
	for (key = attr; key && *key; key += 2) {
		value = key + 1;
		nih_assert (value && *value);

		if (! strcmp (*key, "name")) {
			name = *value;
		} else {
			nih_warn ("%s:%zi:%zi: %s: %s", context->filename,
				  XML_GetCurrentLineNumber (xmlp),
				  XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown <node> attribute"),
				  *key);
		}
	}

	/* If we have a name check that it's valid */
	if (name && (! node_path_valid (name)))
		nih_return_error (-1, NODE_INVALID_PATH,
				  _(NODE_INVALID_PATH_STR));

	/* Allocate a Node object and push onto the stack */
	node = node_new (NULL, name);
	if (! node)
		nih_return_system_error (-1);

	if (! parse_stack_push (NULL, &context->stack, PARSE_NODE, node)) {
		nih_error_raise_system ();
		nih_free (node);
		return -1;
	}

	return 0;
}

/**
 * node_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for a "node" end
 * tag, and matches a call to node_start_tag() made at the same parsing
 * level.
 *
 * The node is set in the context so it can be returned once the parser
 * completes.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
node_end_tag (XML_Parser  xmlp,
	      const char *tag)
{
	ParseContext *context;
	ParseStack   *entry;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	nih_assert (entry->type == PARSE_NODE);

	nih_debug ("Set parsed node to %s", entry->node->path ?: "(unknown)");
	nih_assert (context->node == NULL);
	context->node = entry->node;
	if (context->parent)
		nih_ref (entry->node, context->parent);

	nih_unref_only (entry->node, entry);
	nih_free (entry);

	return 0;
}


/**
 * node_lookup_interface:
 * @node: node to search,
 * @symbol: interface symbol to find.
 *
 * Finds an interface in @node's interfaces list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: interface found or NULL if no interface matches.
 **/
Interface *
node_lookup_interface (Node       *node,
		       const char *symbol)
{
	nih_assert (node != NULL);
	nih_assert (symbol != NULL);

	NIH_LIST_FOREACH (&node->interfaces, iter) {
		Interface *interface = (Interface *)iter;

		if (interface->symbol
		    && (! strcmp (interface->symbol, symbol)))
			return interface;
	}

	return NULL;
}
