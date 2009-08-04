/* nih-dbus-tool
 *
 * node.c - top-level object parsing and handling
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

#include "symbol.h"
#include "indent.h"
#include "type.h"
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
		const char *  tag,
		char * const *attr)
{
	ParseContext *  context;
	ParseStack *    parent;
	nih_local Node *node = NULL;
	char * const *  key;
	char * const *  value;
	const char *    name = NULL;

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
			nih_warn ("%s:%zu:%zu: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
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
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
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
	ParseStack *  entry;

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
	nih_ref (entry->node, context->parent);

	nih_unref (entry->node, entry);
	nih_free (entry);

	return 0;
}


/**
 * node_lookup_interface:
 * @node: node to search,
 * @symbol: interface symbol to find.
 *
 * Finds an interface in @node's interfaces list which has the generated
 * or supplied C symbol @symbol.  If @symbol is NULL, the default interface
 * will be returned.
 *
 * Returns: interface found or NULL if no interface matches.
 **/
Interface *
node_lookup_interface (Node *      node,
		       const char *symbol)
{
	nih_assert (node != NULL);

	NIH_LIST_FOREACH (&node->interfaces, iter) {
		Interface *interface = (Interface *)iter;

		if ((interface->symbol && symbol
		     && (! strcmp (interface->symbol, symbol)))
		    || ((! interface->symbol) && (! symbol)))
			return interface;
	}

	return NULL;
}


/**
 * node_interfaces_array:
 * @parent: parent object for new string,
 * @prefix: prefix for array name,
 * @node: node to generate array for,
 * @object: whether array is for an object or proxy,
 * @prototypes: list to append prototypes to.
 *
 * Generates C code to declare an array of NihDBusInterface pointers for
 * the node @node, the code includes each of the NihDBusInterface structure
 * definitions individually as well as the array definitions for methods,
 * signals, properties and their arguments in them.
 *
 * If @object is TRUE, the array will be for an object definition so method
 * handler function and property getter and setter function pointers will
 * be filled in.  If @object is FALSE, the array will be for a proxy
 * definition so the signal filter function pointers will be filled in.
 *
 * The prototype of the returned variable declaration, and the prototypes
 * of the interface structures, are returned as TypeVar objects appended
 * to the @prototypes list.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
node_interfaces_array (const void *parent,
		       const char *prefix,
		       Node *      node,
		       int         object,
		       NihList *   prototypes)
{
	nih_local char *name = NULL;
	nih_local char *block = NULL;
	char *          code = NULL;
	TypeVar *       var;

	nih_assert (prefix != NULL);
	nih_assert (node != NULL);
	nih_assert (prototypes != NULL);

	name = symbol_extern (NULL, prefix, NULL, NULL, "interfaces", NULL);
	if (! name)
		return NULL;

	/* Append the address of each of the interface structures to the
	 * block we build, and the code to the interfaces list.
	 */
	NIH_LIST_FOREACH (&node->interfaces, iter) {
		Interface *     interface = (Interface *)iter;
		NihList         struct_prototypes;
		nih_local char *struct_code = NULL;

		nih_list_init (&struct_prototypes);

		struct_code = interface_struct (NULL, prefix, interface,
						object, &struct_prototypes);
		if (! struct_code) {
			if (code)
				nih_free (code);
			return NULL;
		}

		nih_assert (! NIH_LIST_EMPTY (&struct_prototypes));

		var = (TypeVar *)struct_prototypes.next;

		if (! nih_strcat_sprintf (&code, parent,
					  "%s\n", struct_code)) {
			if (code)
				nih_free (code);
			return NULL;
		}

		if (! nih_strcat_sprintf (&block, NULL,
					  "&%s,\n", var->name)) {
			if (code)
				nih_free (code);
			return NULL;
		}

		/* Copy the prototypes to the list we return, since we
		 * want to export those as well.
		 */
		NIH_LIST_FOREACH_SAFE (&struct_prototypes, iter) {
			var = (TypeVar *)iter;

			if (! type_to_extern (&var->type, var)) {
				if (code)
					nih_free (code);
				return NULL;
			}

			nih_ref (var, code);
			nih_list_add (prototypes, &var->entry);
		}
	}

	/* Append the final element to the block of elements, indent and
	 * surround with the structure definition.
	 */
	if (! nih_strcat (&block, NULL, "NULL\n")) {
		if (code)
			nih_free (code);
		return NULL;
	}

	if (! indent (&block, NULL, 1)) {
		if (code)
			nih_free (code);
		return NULL;
	}

	if (! nih_strcat_sprintf (&code, parent,
				  "const NihDBusInterface *%s[] = {\n"
				  "%s"
				  "};\n",
				  name,
				  block)) {
		if (code)
			nih_free (code);
		return NULL;
	}

	/* Append the prototype to the list */
	var = type_var_new (code, "const NihDBusInterface *", name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	var->array = TRUE;

	if (! type_to_extern (&var->type, var)) {
		if (code)
			nih_free (code);
		return NULL;
	}

	nih_list_add (prototypes, &var->entry);

	return code;
}


/**
 * node_object_functions:
 * @parent: parent object for new string,
 * @prefix: prefix for function names,
 * @node: node to generate functions for,
 * @prototypes: list to append prototypes to,
 * @handlers: list to append handler prototypes to,
 * @structs: list to append structure definitions to,
 * @externs: list to append prototypes of extern functions to.
 *
 * Generates C code for all of the functions that @node would require to
 * wrap existing C functions and implement the D-Bus interfaces described
 * for the object.
 *
 * Functions in the returned code to implement method handlers and property
 * getter and setters will be declared static and their prototypes returned
 * as TypeFunc objects appended to the @prototypes list.  You normally ensure
 * that these receive a forward declaration.
 *
 * Those functions will call implementation functions that other code is
 * expected to provide, the names and prototypes of these expected functions
 * are returned as TypeFunc objects appended to the @handlers list.  You
 * must implement these elsewhere, and ensure that the prototype has a
 * forward declaration.
 *
 * Functions in the returned code to implement signal emissions are part of
 * a public API that your own code may call.  The names and prototypes are
 * returned as TypeFunc objects appended to the @externs list, you would
 * normally place these in a header file.
 *
 * If any of the function arguments require a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and the method,
 * signal or property the function is for.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
node_object_functions (const void *parent,
		       const char *prefix,
		       Node *      node,
		       NihList *   prototypes,
		       NihList *   handlers,
		       NihList *   structs,
		       NihList *   externs)
{
	char *code = NULL;
	int   first = TRUE;

	nih_assert (prefix != NULL);
	nih_assert (node != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (handlers != NULL);
	nih_assert (structs != NULL);
	nih_assert (externs != NULL);

	code = nih_strdup (parent, "");
	if (! code)
		return NULL;

	NIH_LIST_FOREACH (&node->interfaces, interface_iter) {
		Interface *interface = (Interface *)interface_iter;

		NIH_LIST_FOREACH (&interface->methods, method_iter) {
			Method *        method = (Method *)method_iter;
			NihList         method_prototypes;
			NihList         method_handlers;
			NihList         method_structs;
			NihList         method_externs;
			nih_local char *object_func = NULL;
			nih_local char *reply_func = NULL;

			nih_list_init (&method_prototypes);
			nih_list_init (&method_handlers);
			nih_list_init (&method_structs);
			nih_list_init (&method_externs);

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			object_func = method_object_function (
				NULL, prefix, interface, method,
				&method_prototypes, &method_handlers,
				&method_structs);
			if (! object_func)
				goto error;

			if (! nih_strcat_sprintf (&code, parent,
						  "static %s",
						  object_func))
				goto error;

			if (method->async) {
				reply_func = method_reply_function (
					NULL, prefix, interface, method,
					&method_externs, &method_structs);
				if (! reply_func)
					goto error;

				if (! nih_strcat_sprintf (&code, parent,
							  "\n"
							  "%s",
							  reply_func))
					goto error;
			}

			NIH_LIST_FOREACH_SAFE (&method_prototypes, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_static (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (prototypes, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&method_handlers, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_extern (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (handlers, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&method_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}

			NIH_LIST_FOREACH_SAFE (&method_externs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (externs, &func->entry);
			}
		}

		NIH_LIST_FOREACH (&interface->signals, signal_iter) {
			Signal *        signal = (Signal *)signal_iter;
			NihList         signal_structs;
			NihList         signal_externs;
			nih_local char *object_func = NULL;

			nih_list_init (&signal_structs);
			nih_list_init (&signal_externs);

			object_func = signal_object_function (
				NULL, prefix, interface, signal,
				&signal_externs, &signal_structs);
			if (! object_func)
				goto error;

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			if (! nih_strcat (&code, parent, object_func))
				goto error;

			NIH_LIST_FOREACH_SAFE (&signal_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}

			NIH_LIST_FOREACH_SAFE (&signal_externs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (externs, &func->entry);
			}
		}

		NIH_LIST_FOREACH (&interface->properties, property_iter) {
			Property *      property = (Property *)property_iter;
			NihList         property_prototypes;
			NihList         property_handlers;
			NihList         property_structs;
			nih_local char *get_func = NULL;
			nih_local char *set_func = NULL;

			nih_list_init (&property_prototypes);
			nih_list_init (&property_handlers);
			nih_list_init (&property_structs);

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			if (property->access != NIH_DBUS_WRITE) {
				get_func = property_object_get_function (
					NULL, prefix, interface, property,
					&property_prototypes, &property_handlers,
					&property_structs);
				if (! get_func)
					goto error;

				if (! nih_strcat_sprintf (&code, parent,
							  "static %s",
							  get_func))
					goto error;
			}

			if (property->access == NIH_DBUS_READWRITE) {
				if (! nih_strcat (&code, parent, "\n"))
					goto error;

				/* Don't duplicate structures; these will
				 * get freed automatically.
				 */
				nih_list_init (&property_structs);
			}

			if (property->access != NIH_DBUS_READ) {
				set_func = property_object_set_function (
					NULL, prefix, interface, property,
					&property_prototypes, &property_handlers,
					&property_structs);
				if (! set_func)
					goto error;

				if (! nih_strcat_sprintf (&code, parent,
							  "static %s",
							  set_func))
					goto error;
			}

			NIH_LIST_FOREACH_SAFE (&property_prototypes, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_static (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (prototypes, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&property_handlers, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_extern (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (handlers, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}
		}
	}

	return code;
error:
	nih_free (code);

	return NULL;
}

/**
 * node_proxy_functions:
 * @parent: parent object for new string,
 * @prefix: prefix for function names,
 * @node: node to generate functions for,
 * @prototypes: list to append prototypes to,
 * @structs: list to append structure definitions to,
 * @typedefs: list to append callback typedefs to,
 * @externs: list to append prototypes of extern functions to.
 *
 * Generates C code for all of the functions that @node would require to
 * provide remote object access.
 *
 * Functions in the returned code to implement signal filter functions
 * will be declared static and their prototypes returned as TypeFunc objects
 * appended to the @prototypes list.  You normally ensure that these receive
 * a forward declaration.
 *
 * Functions in the returned code to implement method and property get/set
 * proxy functions are part of a public API that your own code may call.
 * The names and prototypes are returned as TypeFunc objects appended to
 * the @externs list, you would normally place these in a header file.
 *
 * Both sets of these functions will call handler and callback functions
 * that other code is expected to provide, either passed directly to the
 * function (for method and proxy functions) or passed to
 * nih_dbus_proxy_connect() (for signal functions).  The typedef for those
 * functions are returned as TypeFunc objects appended to the @typedefs list.
 * You would normally place these in a header file.
 *
 * If any of the function arguments require a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and the method,
 * signal or property the function is for.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
node_proxy_functions (const void *parent,
		      const char *prefix,
		      Node *      node,
		      NihList *   prototypes,
		      NihList *   structs,
		      NihList *   typedefs,
		      NihList *   externs)
{
	char *code = NULL;
	int   first = TRUE;

	nih_assert (prefix != NULL);
	nih_assert (node != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (structs != NULL);
	nih_assert (typedefs != NULL);
	nih_assert (externs != NULL);

	code = nih_strdup (parent, "");
	if (! code)
		return NULL;

	NIH_LIST_FOREACH (&node->interfaces, interface_iter) {
		Interface *interface = (Interface *)interface_iter;
		NihList    discard;

		nih_list_init (&discard);

		NIH_LIST_FOREACH (&interface->methods, method_iter) {
			Method *        method = (Method *)method_iter;
			NihList         method_prototypes;
			NihList         method_structs;
			NihList         method_typedefs;
			NihList         method_externs;
			nih_local char *proxy_func = NULL;
			nih_local char *notify_func = NULL;
			nih_local char *sync_func = NULL;

			nih_list_init (&method_prototypes);
			nih_list_init (&method_structs);
			nih_list_init (&method_typedefs);
			nih_list_init (&method_externs);

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			proxy_func = method_proxy_function (
				NULL, prefix, interface, method,
				&method_externs, &discard);
			if (! proxy_func)
				goto error;

			notify_func = method_proxy_notify_function (
				NULL, prefix, interface, method,
				&method_prototypes, &method_typedefs,
				&discard);
			if (! notify_func)
				goto error;

			sync_func = method_proxy_sync_function (
				NULL, prefix, interface, method,
				&method_externs, &method_structs);
			if (! sync_func)
				goto error;

			if (! nih_strcat_sprintf (&code, parent,
						  "%s"
						  "\n"
						  "static %s"
						  "\n"
						  "%s",
						  proxy_func,
						  notify_func,
						  sync_func))
				goto error;

			NIH_LIST_FOREACH_SAFE (&method_prototypes, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_static (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (prototypes, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&method_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}

			NIH_LIST_FOREACH_SAFE (&method_typedefs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (typedefs, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&method_externs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (externs, &func->entry);
			}
		}

		NIH_LIST_FOREACH (&interface->signals, signal_iter) {
			Signal *        signal = (Signal *)signal_iter;
			NihList         signal_prototypes;
			NihList         signal_structs;
			NihList         signal_typedefs;
			nih_local char *proxy_func = NULL;

			nih_list_init (&signal_prototypes);
			nih_list_init (&signal_structs);
			nih_list_init (&signal_typedefs);

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			proxy_func = signal_proxy_function (
				NULL, prefix, interface, signal,
				&signal_prototypes, &signal_typedefs,
				&signal_structs);
			if (! proxy_func)
				goto error;

			if (! nih_strcat_sprintf (&code, parent,
						  "static %s",
						  proxy_func))
				goto error;

			NIH_LIST_FOREACH_SAFE (&signal_prototypes, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_static (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (prototypes, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&signal_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}

			NIH_LIST_FOREACH_SAFE (&signal_typedefs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (typedefs, &func->entry);
			}
		}

		NIH_LIST_FOREACH (&interface->properties, property_iter) {
			Property *      property = (Property *)property_iter;
			NihList         property_prototypes;
			NihList         property_structs;
			NihList         property_typedefs;
			NihList         property_externs;
			nih_local char *get_func = NULL;
			nih_local char *get_notify_func = NULL;
			nih_local char *get_sync_func = NULL;
			nih_local char *set_func = NULL;
			nih_local char *set_notify_func = NULL;
			nih_local char *set_sync_func = NULL;

			nih_list_init (&property_prototypes);
			nih_list_init (&property_structs);
			nih_list_init (&property_typedefs);
			nih_list_init (&property_externs);

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			if (property->access != NIH_DBUS_WRITE) {
				get_func = property_proxy_get_function (
					NULL, prefix, interface, property,
					&property_externs, &discard);
				if (! get_func)
					goto error;

				get_notify_func = property_proxy_get_notify_function (
					NULL, prefix, interface, property,
					&property_prototypes, &property_typedefs,
					&discard);
				if (! get_notify_func)
					goto error;

				get_sync_func = property_proxy_get_sync_function (
					NULL, prefix, interface, property,
					&property_externs, &discard);
				if (! get_sync_func)
					goto error;

				if (! nih_strcat_sprintf (&code, parent,
							  "%s"
							  "\n"
							  "static %s"
							  "\n"
							  "%s",
							  get_func,
							  get_notify_func,
							  get_sync_func))
					goto error;
			}

			if (property->access == NIH_DBUS_READWRITE)
				if (! nih_strcat (&code, parent, "\n"))
					goto error;

			if (property->access != NIH_DBUS_READ) {
				set_func = property_proxy_set_function (
					NULL, prefix, interface, property,
					&property_externs, &discard);
				if (! set_func)
					goto error;

				set_notify_func = property_proxy_set_notify_function (
					NULL, prefix, interface, property,
					&property_prototypes, &property_typedefs,
					&discard);
				if (! set_notify_func)
					goto error;

				set_sync_func = property_proxy_set_sync_function (
					NULL, prefix, interface, property,
					&property_externs,
					(property->access == NIH_DBUS_WRITE
					 ? &property_structs
					 : &discard));
				if (! set_sync_func)
					goto error;

				if (! nih_strcat_sprintf (&code, parent,
							  "%s"
							  "\n"
							  "static %s"
							  "\n"
							  "%s",
							  set_func,
							  set_notify_func,
							  set_sync_func))
					goto error;
			}

			NIH_LIST_FOREACH_SAFE (&property_prototypes, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_static (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (prototypes, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}

			NIH_LIST_FOREACH_SAFE (&property_typedefs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (typedefs, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&property_externs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (externs, &func->entry);
			}
		}

		/* Functions to obtain all of the properties */
		if (! NIH_LIST_EMPTY (&interface->properties)) {
			NihList         all_prototypes;
			NihList         all_structs;
			NihList         all_typedefs;
			NihList         all_externs;
			nih_local char *get_all_func = NULL;
			nih_local char *get_all_notify_func = NULL;
			nih_local char *get_all_sync_func = NULL;

			nih_list_init (&all_prototypes);
			nih_list_init (&all_structs);
			nih_list_init (&all_typedefs);
			nih_list_init (&all_externs);

			if (! first)
				if (! nih_strcat (&code, parent, "\n\n"))
					goto error;
			first = FALSE;

			get_all_func = interface_proxy_get_all_function (
				NULL, prefix, interface,
				&all_externs, &discard);
			if (! get_all_func)
				goto error;

			get_all_notify_func = interface_proxy_get_all_notify_function (
				NULL, prefix, interface,
				&all_prototypes, &all_typedefs,
				&discard);
			if (! get_all_notify_func)
				goto error;

			get_all_sync_func = interface_proxy_get_all_sync_function (
				NULL, prefix, interface,
				&all_externs, &all_structs);
			if (! get_all_sync_func)
				goto error;

			if (! nih_strcat_sprintf (&code, parent,
						  "%s"
						  "\n"
						  "static %s"
						  "\n"
						  "%s",
						  get_all_func,
						  get_all_notify_func,
						  get_all_sync_func))
				goto error;

			NIH_LIST_FOREACH_SAFE (&all_prototypes, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				if (! type_to_static (&func->type, func))
					goto error;

				nih_ref (func, code);
				nih_list_add (prototypes, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&all_structs, iter) {
				TypeStruct *structure = (TypeStruct *)iter;

				nih_ref (structure, code);
				nih_list_add (structs, &structure->entry);
			}

			NIH_LIST_FOREACH_SAFE (&all_typedefs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (typedefs, &func->entry);
			}

			NIH_LIST_FOREACH_SAFE (&all_externs, iter) {
				TypeFunc *func = (TypeFunc *)iter;

				nih_ref (func, code);
				nih_list_add (externs, &func->entry);
			}
		}
	}

	return code;
error:
	nih_free (code);

	return NULL;
}
