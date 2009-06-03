/* nih-dbus-tool
 *
 * interface.c - interface parsing and handling
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "symbol.h"
#include "indent.h"
#include "node.h"
#include "interface.h"
#include "method.h"
#include "parse.h"
#include "errors.h"


/**
 * interface_name_valid:
 * @name: Interface name to verify.
 *
 * Verifies whether @name matches the specification for D-Bus interface
 * names.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
interface_name_valid (const char *name)
{
	size_t parts = 1;

	nih_assert (name != NULL);

	/* Name must not begin with a '.' */
	if (name[0] == '.')
		return FALSE;

	/* We can get away with just using strlen() here even through name
	 * is in UTF-8 because all the valid characters are ASCII.
	 */
	for (size_t i = 0; i < strlen (name); i++) {
		/* Name components may be separated by single '.' characters,
		 * multiple ones are not allowed.  Keep a count of how many
		 * parts we have, since there's a defined minimum.
		 */
		if (name[i] == '.') {
			if (name[i-1] == '.')
				return FALSE;

			parts++;
			continue;
		}

		/* Names may contain digits, but not at the beginning of the
		 * name or any part of it.
		 */
		if ((name[i] >= '0') && (name[i] <= '9')) {
			if (i == 0)
				return FALSE;
			if (name[i-1] == '.')
				return FALSE;

			continue;
		}

		/* Valid characters anywhere are [A-Za-z_] */
		if (   ((name[i] < 'A') || (name[i] > 'Z'))
		    && ((name[i] < 'a') || (name[i] > 'z'))
		    && (name[i] != '_'))
			return FALSE;
	}

	/* Name must consist of at least two parts */
	if (parts < 2)
		return FALSE;

	/* Final character may not be '.' */
	if (name[strlen (name) - 1] == '.')
		return FALSE;

	/* Name must be no more than 255 characters */
	if (strlen (name) > 255)
		return FALSE;

	return TRUE;
}


/**
 * interface_new:
 * @parent: parent object for new interface,
 * @name: D-Bus name of interface.
 *
 * Allocates a new D-Bus object Interface data structure, with the D-Bus
 * name set to @name.  The returned structure is not placed into any list.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned interface.  When all parents
 * of the returned interface are freed, the returned interface will also be
 * freed.
 *
 * Returns: the new interface or NULL if the allocation failed.
 **/
Interface *
interface_new (const void *parent,
	       const char *name)
{
	Interface *interface;

	nih_assert (name != NULL);

	interface = nih_new (parent, Interface);
	if (! interface)
		return NULL;

	nih_list_init (&interface->entry);

	nih_alloc_set_destructor (interface, nih_list_destroy);

	interface->name = nih_strdup (interface, name);
	if (! interface->name) {
		nih_free (interface);
		return NULL;
	}

	interface->symbol = NULL;
	interface->deprecated = FALSE;

	nih_list_init (&interface->methods);
	nih_list_init (&interface->signals);
	nih_list_init (&interface->properties);

	return interface;
}


/**
 * interface_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for an "interface"
 * start tag, a child of the "node" tag that defines a D-Bus interface
 * implemented by that object.
 *
 * If the interface does not appear within a node tag a warning is emitted
 * and the tag will be ignored.
 *
 * Interfaces must have a "name" attribute containing the D-Bus name
 * of the interface.
 *
 * Any unknown attributes result in a warning and will be ignored.
 *
 * An Interface object will be allocated and pushed onto the stack, this is
 * not added to the node until the end tag is found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
interface_start_tag (XML_Parser    xmlp,
		     const char *  tag,
		     char * const *attr)
{
	ParseContext *context;
	ParseStack *  parent;
	Interface *   interface;
	char * const *key;
	char * const *value;
	const char *  name = NULL;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Interfaces should only appear inside nodes. */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || (parent->type != PARSE_NODE)) {
		nih_warn ("%s:%zu:%zu: %s", context->filename,
			  (size_t)XML_GetCurrentLineNumber (xmlp),
			  (size_t)XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unexpected <interface> tag"));

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
				  _("Ignored unknown <interface> attribute"),
				  *key);
		}
	}

	/* Check we have a name and that it's valid */
	if (! name)
		nih_return_error (-1, INTERFACE_MISSING_NAME,
				  _(INTERFACE_MISSING_NAME_STR));
	if (! interface_name_valid (name))
		nih_return_error (-1, INTERFACE_INVALID_NAME,
				  _(INTERFACE_INVALID_NAME_STR));

	/* Allocate an Interface object and push onto the stack */
	interface = interface_new (NULL, name);
	if (! interface)
		nih_return_system_error (-1);

	if (! parse_stack_push (NULL, &context->stack,
				PARSE_INTERFACE, interface)) {
		nih_error_raise_system ();
		nih_free (interface);
		return -1;
	}

	return 0;
}

/**
 * interface_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for an "interface" end
 * tag, and matches a call to interface_start_tag() made at the same
 * parsing level.
 *
 * The interface is added to the list of interfaces defined by its parent
 * node.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
interface_end_tag (XML_Parser  xmlp,
		   const char *tag)
{
	ParseContext *context;
	ParseStack *  entry;
	ParseStack *  parent;
	Interface *   interface;
	Interface *   conflict;
	Node *        node;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	nih_assert (entry->type == PARSE_INTERFACE);
	interface = entry->interface;

	/* Generate a symbol from the trailing part of the name */
	if (! interface->symbol) {
		char *trail;

		trail = strrchr (interface->name, '.');
		nih_assert (trail != NULL);
		trail++;

		interface->symbol = symbol_from_name (interface, trail);
		if (! interface->symbol)
			nih_return_no_memory_error (-1);
	}

	nih_list_remove (&entry->entry);
	parent = parse_stack_top (&context->stack);
	nih_assert (parent != NULL);
	nih_assert (parent->type == PARSE_NODE);
	node = parent->node;

	/* Make sure there's not a conflict before adding the interface */
	conflict = node_lookup_interface (node, interface->symbol);
	if (conflict) {
		nih_error_raise_printf (INTERFACE_DUPLICATE_SYMBOL,
					_(INTERFACE_DUPLICATE_SYMBOL_STR),
					interface->symbol, conflict->name);
		return -1;
	}

	nih_debug ("Add %s interface to %s node",
		   interface->name, node->path ?: "(unknown)");
	nih_ref (interface, node);
	nih_list_add (&node->interfaces, &interface->entry);

	nih_free (entry);

	return 0;
}


/**
 * interface_annotation:
 * @interface: interface object annotation applies to,
 * @name: annotation name,
 * @value: annotation value.
 *
 * Handles applying the annotation @name with value @value to the interface
 * @interface.  Interfaces may be annotated as deprecated or may have an
 * alternate symbol name specified.
 *
 * Unknown annotations or illegal values to the known annotations result
 * in an error being raised.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
interface_annotation (Interface * interface,
		      const char *name,
		      const char *value)
{
	nih_assert (interface != NULL);
	nih_assert (name != NULL);
	nih_assert (value != NULL);

	if (! strcmp (name, "org.freedesktop.DBus.Deprecated")) {
		if (! strcmp (value, "true")) {
			nih_debug ("Marked %s interface as deprecated",
				   interface->name);
			interface->deprecated = TRUE;
		} else if (! strcmp (value, "false")) {
			nih_debug ("Marked %s interface as not deprecated",
				   interface->name);
			interface->deprecated = FALSE;
		} else {
			nih_return_error (-1, INTERFACE_ILLEGAL_DEPRECATED,
					  _(INTERFACE_ILLEGAL_DEPRECATED_STR));
		}

	} else if (! strcmp (name, "com.netsplit.Nih.Symbol")) {
		if (symbol_valid (value)) {
			if (interface->symbol)
				nih_unref (interface->symbol, interface);

			interface->symbol = nih_strdup (interface, value);
			if (! interface->symbol)
				nih_return_no_memory_error (-1);

			nih_debug ("Set %s interface symbol to %s",
				   interface->name, interface->symbol);
		} else {
			nih_return_error (-1, INTERFACE_INVALID_SYMBOL,
					  _(INTERFACE_INVALID_SYMBOL_STR));
		}

	} else {
		nih_error_raise_printf (INTERFACE_UNKNOWN_ANNOTATION,
					"%s: %s: %s",
					_(INTERFACE_UNKNOWN_ANNOTATION_STR),
					interface->name, name);
		return -1;
	}

	return 0;
}


/**
 * interface_methods_array:
 * @parent: parent object for new string,
 * @prefix: prefix for array name,
 * @interface: interface to generate array for,
 * @with_handlers: whether to include handler pointers.
 *
 * Generates C code to declare an array of NihDBusMethod variables
 * containing information about the methods of the interface @interface;
 * this will also include array definitions for the arguments of each
 * method, since these are referred to by the returned array.
 *
 * If @with_handlers is TRUE the returned array will contain pointers to
 * handler functions that should be already defined (or at least prototyped);
 * when FALSE this member will be NULL.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
interface_methods_array (const void *parent,
			 const char *prefix,
			 Interface * interface,
			 int         with_handlers)
{
	nih_local char *name = NULL;
	size_t          max_name = 0;
	size_t          max_args = 0;
	size_t          max_handler = 0;
	nih_local char *args = NULL;
	nih_local char *block = NULL;
	char *          code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);

	name = symbol_impl (NULL, prefix, interface->name, NULL, "methods");
	if (! name)
		return NULL;

	/* Figure out the longest method name, arguments array variable name
	 * and handler function name.
	 */
	NIH_LIST_FOREACH (&interface->methods, iter) {
		Method *        method = (Method *)iter;
		nih_local char *args_name = NULL;
		nih_local char *handler_name = NULL;

		if (strlen (method->name) > max_name)
			max_name = strlen (method->name);

		args_name = symbol_impl (NULL, prefix, interface->name,
					 method->name, "args");
		if (! args_name)
			return NULL;

		if (strlen (args_name) > max_args)
			max_args = strlen (args_name);

		if (with_handlers) {
			handler_name = symbol_impl (NULL, prefix,
						    interface->name,
						    method->name, "method");
			if (! handler_name)
				return NULL;

			if (strlen (handler_name) > max_handler)
				max_handler = strlen (handler_name);
		} else {
			if (max_handler < 4)
				max_handler = 4;
		}
	}

	/* Append each method such that the names, args variable names and
	 * handler function names are all lined up with each other.
	 */
	NIH_LIST_FOREACH (&interface->methods, iter) {
		Method *        method = (Method *)iter;
		nih_local char *line = NULL;
		char *          dest;
		nih_local char *args_name = NULL;
		nih_local char *handler_name = NULL;
		nih_local char *args_array = NULL;

		line = nih_alloc (NULL, max_name + max_args + max_handler + 13);
		if (! line)
			return NULL;

		dest = line;

		memcpy (dest, "{ \"", 3);
		dest += 3;

		memcpy (dest, method->name, strlen (method->name));
		dest += strlen (method->name);

		memcpy (dest, "\", ", 3);
		dest += 3;

		memset (dest, ' ', max_name - strlen (method->name));
		dest += max_name - strlen (method->name);

		args_name = symbol_impl (NULL, prefix, interface->name,
					 method->name, "args");
		if (! args_name)
			return NULL;

		memcpy (dest, args_name, strlen (args_name));
		dest += strlen (args_name);

		memcpy (dest, ", ", 2);
		dest += 2;

		memset (dest, ' ', max_args - strlen (args_name));
		dest += max_args - strlen (args_name);

		if (with_handlers) {
			handler_name = symbol_impl (NULL, prefix,
						    interface->name,
						    method->name, "method");
			if (! handler_name)
				return NULL;

			memcpy (dest, handler_name, strlen (handler_name));
			dest += strlen (handler_name);

			memset (dest, ' ', max_handler - strlen (handler_name));
			dest += max_handler - strlen (handler_name);
		} else {
			memcpy (dest, "NULL", 4);
			dest += 4;

			memset (dest, ' ', max_handler - 4);
			dest += max_handler - 4;
		}

		memcpy (dest, " },\n", 4);
		dest += 4;

		*dest = '\0';

		if (! nih_strcat (&block, NULL, line))
			return NULL;

		/* Prepend the variables for the arguments array */
		args_array = method_args_array (NULL, prefix, interface,
						method);
		if (! args_array)
			return NULL;

		if (! nih_strcat_sprintf (&args, NULL,
					  "%s"
					  "\n",
					  args_array))
			return NULL;
	}

	/* Append the final element to the block of elements, indent and
	 * surround with the structure definition.
	 */
	if (! nih_strcat (&block, NULL, "{ NULL }\n"))
		return NULL;

	if (! indent (&block, NULL, 1))
		return NULL;

	code = nih_sprintf (parent,
			    "%s"
			    "static const %s[] = {\n"
			    "%s"
			    "};\n",
			    args ? args : "",
			    name,
			    block);
	if (! code)
		return NULL;

	return code;
}
