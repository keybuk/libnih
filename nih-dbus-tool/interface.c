/* nih-dbus-tool
 *
 * interface.c - interface parsing and handling
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

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/logging.h>
#include <nih/error.h>

#include "symbol.h"
#include "node.h"
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
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
		     const char   *tag,
		     char * const *attr)
{
	ParseContext *context;
	ParseStack   *parent;
	Interface    *interface;
	char * const *key;
	char * const *value;
	const char   *name = NULL;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Interfaces should only appear inside nodes. */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || (parent->type != PARSE_NODE)) {
		nih_warn ("%s:%zi:%zi: %s", context->filename,
			  XML_GetCurrentLineNumber (xmlp),
			  XML_GetCurrentColumnNumber (xmlp),
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
			nih_warn ("%s:%zi:%zi: %s: %s", context->filename,
				  XML_GetCurrentLineNumber (xmlp),
				  XML_GetCurrentColumnNumber (xmlp),
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
	ParseStack   *entry, *parent;
	Interface    *interface, *conflict;
	Node         *node;

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
interface_annotation (Interface  *interface,
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
 * interface_lookup_method:
 * @interface: interface to search,
 * @symbol: method symbol to find.
 *
 * Finds a method in @interface's methods list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: method found or NULL if no method matches.
 **/
Method *
interface_lookup_method (Interface  *interface,
			 const char *symbol)
{
	nih_assert (interface != NULL);
	nih_assert (symbol != NULL);

	NIH_LIST_FOREACH (&interface->methods, iter) {
		Method *method = (Method *)iter;

		if (method->symbol
		    && (! strcmp (method->symbol, symbol)))
			return method;
	}

	return NULL;
}

/**
 * interface_lookup_signal:
 * @interface: interface to search,
 * @symbol: signal symbol to find.
 *
 * Finds a signal in @interface's signals list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: signal found or NULL if no signal matches.
 **/
Signal *
interface_lookup_signal (Interface  *interface,
			 const char *symbol)
{
	nih_assert (interface != NULL);
	nih_assert (symbol != NULL);

	NIH_LIST_FOREACH (&interface->signals, iter) {
		Signal *signal = (Signal *)iter;

		if (signal->symbol
		    && (! strcmp (signal->symbol, symbol)))
			return signal;
	}

	return NULL;
}

/**
 * interface_lookup_property:
 * @interface: interface to search,
 * @symbol: property symbol to find.
 *
 * Finds a property in @interface's properties list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: property found or NULL if no property matches.
 **/
Property *
interface_lookup_property (Interface  *interface,
			   const char *symbol)
{
	nih_assert (interface != NULL);
	nih_assert (symbol != NULL);

	NIH_LIST_FOREACH (&interface->properties, iter) {
		Property *property = (Property *)iter;

		if (property->symbol
		    && (! strcmp (property->symbol, symbol)))
			return property;
	}

	return NULL;
}
