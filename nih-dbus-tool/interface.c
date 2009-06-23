/* nih-dbus-tool
 *
 * interface.c - interface parsing and handling
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
	} else if (! strlen (interface->symbol)) {
		nih_unref (interface->symbol, interface);
		interface->symbol = NULL;
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
		if ((strlen (value) == 0) || symbol_valid (value)) {
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
 * @with_handlers: whether to include handler pointers,
 * @prototypes: list to append prototype to.
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
 * The prototype of the returned variable declaration is returned as a
 * TypeVar object appended to the @prototypes list.  The arguments array
 * prototypes are not returned since they are made static.
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
			 int         with_handlers,
			 NihList *   prototypes)
{
	nih_local char *name = NULL;
	NihList         vars;
	size_t          max_name = 0;
	size_t          max_args = 0;
	size_t          max_handler = 0;
	nih_local char *args = NULL;
	TypeVar *       var;
	nih_local char *block = NULL;
	char *          code;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (prototypes != NULL);

	name = symbol_impl (NULL, prefix, interface->name, NULL, "methods");
	if (! name)
		return NULL;

	nih_list_init (&vars);

	/* Figure out the longest method name, arguments array variable name
	 * and handler function name.
	 */
	NIH_LIST_FOREACH (&interface->methods, iter) {
		Method *        method = (Method *)iter;
		NihList         args_prototypes;
		nih_local char *args_array = NULL;
		nih_local char *handler_name = NULL;

		/* Obtain the arguments array for the method, giving us the
		 * name of the array.  Append it as a static to the block
		 * we prepend to our code.
		 */
		nih_list_init (&args_prototypes);
		args_array = method_args_array (NULL, prefix, interface,
						method, &args_prototypes);
		if (! args_array)
			return NULL;

		if (! nih_strcat_sprintf (&args, NULL,
					  "static %s"
					  "\n",
					  args_array))
			return NULL;

		nih_assert (! NIH_LIST_EMPTY (&args_prototypes));

		var = (TypeVar *)args_prototypes.next;
		nih_list_add (&vars, &var->entry);
		nih_ref (var, args);

		/* Calculate size of method name and args var name */
		if (strlen (method->name) > max_name)
			max_name = strlen (method->name);

		if (strlen (var->name) > max_args)
			max_args = strlen (var->name);

		/* Work out name of handler or leave space for "NULL" */
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
	var = (TypeVar *)vars.next;
	NIH_LIST_FOREACH (&interface->methods, iter) {
		Method *        method = (Method *)iter;
		nih_local char *line = NULL;
		char *          dest;
		nih_local char *handler_name = NULL;

		nih_assert (&var->entry != &vars);

		/* Allocate the line and fill in the values, padding out
		 * where necessary.
		 */
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

		memcpy (dest, var->name, strlen (var->name));
		dest += strlen (var->name);

		memcpy (dest, ", ", 2);
		dest += 2;

		memset (dest, ' ', max_args - strlen (var->name));
		dest += max_args - strlen (var->name);

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

		var = (TypeVar *)var->entry.next;
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
			    "const NihDBusMethod %s[] = {\n"
			    "%s"
			    "};\n",
			    args ?: "",
			    name,
			    block);
	if (! code)
		return NULL;

	/* Append the prototype to the list */
	var = type_var_new (code, "const NihDBusMethod", name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	var->array = TRUE;

	nih_list_add (prototypes, &var->entry);

	return code;
}

/**
 * interface_signals_array:
 * @parent: parent object for new string,
 * @prefix: prefix for array name,
 * @interface: interface to generate array for,
 * @with_filter: whether to include filter function pointers,
 * @prototypes: list to append prototype to.
 *
 * Generates C code to declare an array of NihDBusSignal variables
 * containing information about the signals of the interface @interface;
 * this will also include array definitions for the arguments of each
 * signals, since these are referred to by the returned array.
 *
 * If @with_filters is TRUE the returned array will contain pointers to
 * filter functions that should be already defined (or at least prototyped);
 * when FALSE this member will be NULL.
 *
 * The prototype of the returned variable declaration is returned as a
 * TypeVar object appended to the @prototypes list.  The arguments array
 * prototypes are not returned since they are made static.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
interface_signals_array (const void *parent,
			 const char *prefix,
			 Interface * interface,
			 int         with_filters,
			 NihList *   prototypes)
{
	nih_local char *name = NULL;
	NihList         vars;
	size_t          max_name = 0;
	size_t          max_args = 0;
	size_t          max_filter = 0;
	nih_local char *args = NULL;
	TypeVar *       var;
	nih_local char *block = NULL;
	char *          code;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (prototypes != NULL);

	name = symbol_impl (NULL, prefix, interface->name, NULL, "signals");
	if (! name)
		return NULL;

	nih_list_init (&vars);

	/* Figure out the longest signal name, arguments array variable name
	 * and filter function name.
	 */
	NIH_LIST_FOREACH (&interface->signals, iter) {
		Signal *        signal = (Signal *)iter;
		NihList         args_prototypes;
		nih_local char *args_array = NULL;
		nih_local char *filter_name = NULL;

		/* Obtain the arguments array for the signal, giving us the
		 * name of the array.  Append it as a static to the block
		 * we prepend to our code.
		 */
		nih_list_init (&args_prototypes);
		args_array = signal_args_array (NULL, prefix, interface,
						signal, &args_prototypes);
		if (! args_array)
			return NULL;

		if (! nih_strcat_sprintf (&args, NULL,
					  "static %s"
					  "\n",
					  args_array))
			return NULL;

		nih_assert (! NIH_LIST_EMPTY (&args_prototypes));

		var = (TypeVar *)args_prototypes.next;
		nih_list_add (&vars, &var->entry);
		nih_ref (var, args);

		/* Calculate size of signal name and args var name */
		if (strlen (signal->name) > max_name)
			max_name = strlen (signal->name);

		if (strlen (var->name) > max_args)
			max_args = strlen (var->name);

		/* Work out name of filter or leave space for "NULL" */
		if (with_filters) {
			filter_name = symbol_impl (NULL, prefix,
						    interface->name,
						    signal->name, "signal");
			if (! filter_name)
				return NULL;

			if (strlen (filter_name) > max_filter)
				max_filter = strlen (filter_name);
		} else {
			if (max_filter < 4)
				max_filter = 4;
		}
	}

	/* Append each signal such that the names, args variable names and
	 * filter function names are all lined up with each other.
	 */
	var = (TypeVar *)vars.next;
	NIH_LIST_FOREACH (&interface->signals, iter) {
		Signal *        signal = (Signal *)iter;
		nih_local char *line = NULL;
		char *          dest;
		nih_local char *filter_name = NULL;

		nih_assert (&var->entry != &vars);

		/* Allocate the line and fill in the values, padding out
		 * where necessary.
		 */
		line = nih_alloc (NULL, max_name + max_args + max_filter + 13);
		if (! line)
			return NULL;

		dest = line;

		memcpy (dest, "{ \"", 3);
		dest += 3;

		memcpy (dest, signal->name, strlen (signal->name));
		dest += strlen (signal->name);

		memcpy (dest, "\", ", 3);
		dest += 3;

		memset (dest, ' ', max_name - strlen (signal->name));
		dest += max_name - strlen (signal->name);

		memcpy (dest, var->name, strlen (var->name));
		dest += strlen (var->name);

		memcpy (dest, ", ", 2);
		dest += 2;

		memset (dest, ' ', max_args - strlen (var->name));
		dest += max_args - strlen (var->name);

		if (with_filters) {
			filter_name = symbol_impl (NULL, prefix,
						    interface->name,
						    signal->name, "signal");
			if (! filter_name)
				return NULL;

			memcpy (dest, filter_name, strlen (filter_name));
			dest += strlen (filter_name);

			memset (dest, ' ', max_filter - strlen (filter_name));
			dest += max_filter - strlen (filter_name);
		} else {
			memcpy (dest, "NULL", 4);
			dest += 4;

			memset (dest, ' ', max_filter - 4);
			dest += max_filter - 4;
		}

		memcpy (dest, " },\n", 4);
		dest += 4;

		*dest = '\0';

		if (! nih_strcat (&block, NULL, line))
			return NULL;

		var = (TypeVar *)var->entry.next;
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
			    "const NihDBusSignal %s[] = {\n"
			    "%s"
			    "};\n",
			    args ?: "",
			    name,
			    block);
	if (! code)
		return NULL;

	/* Append the prototype to the list */
	var = type_var_new (code, "const NihDBusSignal", name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	var->array = TRUE;

	nih_list_add (prototypes, &var->entry);

	return code;
}

/**
 * interface_properties_array:
 * @parent: parent object for new string,
 * @prefix: prefix for array name,
 * @interface: interface to generate array for,
 * @with_handlers: whether to include handler pointers,
 * @prototypes: list to append prototype to.
 *
 * Generates C code to declare an array of NihDBusProperty variables
 * containing information about the properties of the interface @interface.
 *
 * If @with_handlers is TRUE the returned array will contain pointers to
 * getter and setter functions that should be already defined (or at least
 * prototyped); when FALSE these members will be NULL.
 *
 * The prototype of the returned variable declaration is returned as a
 * TypeVar object appended to the @prototypes list.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
interface_properties_array (const void *parent,
			    const char *prefix,
			    Interface * interface,
			    int         with_handlers,
			    NihList *   prototypes)
{
	nih_local char *name = NULL;
	size_t          max_name = 0;
	size_t          max_type = 0;
	size_t          max_access = 0;
	size_t          max_getter = 0;
	size_t          max_setter = 0;
	nih_local char *block = NULL;
	char *          code;
	TypeVar *       var;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (prototypes != NULL);

	name = symbol_impl (NULL, prefix, interface->name, NULL, "properties");
	if (! name)
		return NULL;

	/* Figure out the longest property name, type, access variable,
	 * getter and setter function names.
	 */
	NIH_LIST_FOREACH (&interface->properties, iter) {
		Property *      property = (Property *)iter;
		nih_local char *getter_name = NULL;
		nih_local char *setter_name = NULL;

		if (strlen (property->name) > max_name)
			max_name = strlen (property->name);

		if (strlen (property->type) > max_type)
			max_type = strlen (property->type);

		switch (property->access) {
		case NIH_DBUS_READ:
			if (max_access < 13)
				max_access = 13;
			break;
		case NIH_DBUS_WRITE:
			if (max_access < 14)
				max_access = 14;
			break;
		case NIH_DBUS_READWRITE:
			if (max_access < 18)
				max_access = 18;
			break;
		default:
			nih_assert_not_reached ();
		}

		if (with_handlers && (property->access != NIH_DBUS_WRITE)) {
			getter_name = symbol_impl (NULL, prefix,
						   interface->name,
						   property->name, "get");
			if (! getter_name)
				return NULL;

			if (strlen (getter_name) > max_getter)
				max_getter = strlen (getter_name);
		} else {
			if (max_getter < 4)
				max_getter = 4;
		}

		if (with_handlers && (property->access != NIH_DBUS_READ)) {
			setter_name = symbol_impl (NULL, prefix,
						   interface->name,
						   property->name, "set");
			if (! setter_name)
				return NULL;

			if (strlen (setter_name) > max_setter)
				max_setter = strlen (setter_name);
		} else {
			if (max_setter < 4)
				max_setter = 4;
		}
	}

	/* Append each property such that the names, types, access enum,
	 * getter and setter function names are all lined up with each other.
	 */
	NIH_LIST_FOREACH (&interface->properties, iter) {
		Property *      property = (Property *)iter;
		nih_local char *line = NULL;
		char *          dest;
		nih_local char *getter_name = NULL;
		nih_local char *setter_name = NULL;

		line = nih_alloc (NULL, (max_name + max_type + max_access
					 + max_getter + max_setter + 19));
		if (! line)
			return NULL;

		dest = line;

		memcpy (dest, "{ \"", 3);
		dest += 3;

		memcpy (dest, property->name, strlen (property->name));
		dest += strlen (property->name);

		memcpy (dest, "\", ", 3);
		dest += 3;

		memset (dest, ' ', max_name - strlen (property->name));
		dest += max_name - strlen (property->name);


		memcpy (dest, "\"", 1);
		dest += 1;

		memcpy (dest, property->type, strlen (property->type));
		dest += strlen (property->type);

		memcpy (dest, "\", ", 3);
		dest += 3;

		memset (dest, ' ', max_type - strlen (property->type));
		dest += max_type - strlen (property->type);

		switch (property->access) {
		case NIH_DBUS_READ:
			memcpy (dest, "NIH_DBUS_READ, ", 15);
			dest += 15;

			memset (dest, ' ', max_access - 13);
			dest += max_access - 13;
			break;
		case NIH_DBUS_WRITE:
			memcpy (dest, "NIH_DBUS_WRITE, ", 16);
			dest += 16;

			memset (dest, ' ', max_access - 14);
			dest += max_access - 14;
			break;
		case NIH_DBUS_READWRITE:
			memcpy (dest, "NIH_DBUS_READWRITE, ", 20);
			dest += 20;

			memset (dest, ' ', max_access - 18);
			dest += max_access - 18;
			break;
		default:
			nih_assert_not_reached ();
		}

		if (with_handlers && (property->access != NIH_DBUS_WRITE)) {
			getter_name = symbol_impl (NULL, prefix,
						   interface->name,
						   property->name, "get");
			if (! getter_name)
				return NULL;

			memcpy (dest, getter_name, strlen (getter_name));
			dest += strlen (getter_name);

			memcpy (dest, ", ", 2);
			dest += 2;

			memset (dest, ' ', max_getter - strlen (getter_name));
			dest += max_getter - strlen (getter_name);
		} else {
			memcpy (dest, "NULL, ", 6);
			dest += 6;

			memset (dest, ' ', max_getter - 4);
			dest += max_getter - 4;
		}

		if (with_handlers && (property->access != NIH_DBUS_READ)) {
			setter_name = symbol_impl (NULL, prefix,
						   interface->name,
						   property->name, "set");
			if (! setter_name)
				return NULL;

			memcpy (dest, setter_name, strlen (setter_name));
			dest += strlen (setter_name);

			memset (dest, ' ', max_setter - strlen (setter_name));
			dest += max_setter - strlen (setter_name);
		} else {
			memcpy (dest, "NULL", 4);
			dest += 4;

			memset (dest, ' ', max_setter - 4);
			dest += max_setter - 4;
		}

		memcpy (dest, " },\n", 4);
		dest += 4;

		*dest = '\0';

		if (! nih_strcat (&block, NULL, line))
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
			    "const NihDBusProperty %s[] = {\n"
			    "%s"
			    "};\n",
			    name,
			    block);
	if (! code)
		return NULL;

	/* Append the prototype to the list */
	var = type_var_new (code, "const NihDBusProperty", name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	var->array = TRUE;

	nih_list_add (prototypes, &var->entry);

	return code;
}


/**
 * interface_struct:
 * @parent: parent object for new string,
 * @prefix: prefix for struct name,
 * @interface: interface to generate struct for,
 * @object: whether struct is for an object or proxy,
 * @prototypes: list to append prototype to.
 *
 * Generates C code to declare an NihDBusInterface structure variable for
 * the given interface @interface, the code includes the array definitions
 * for methods, signals, properties and their arguments.
 *
 * If @object is TRUE, the struct will be for an object definition so method
 * handler function and property getter and setter function pointers will
 * be filled in.  If @object is FALSE, the struct will be for a proxy
 * definition so the signal filter function pointers will be filled in.
 *
 * The prototype of the returned variable declaration is returned as a
 * TypeVar object appended to the @prototypes list.  The methods, signals
 * and properties array prototypes are not returned since they are made
 * static.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
interface_struct (const void *parent,
		  const char *prefix,
		  Interface * interface,
		  int         object,
		  NihList *   prototypes)
{
	nih_local char *name = NULL;
	nih_local char *block = NULL;
	nih_local char *arrays = NULL;
	TypeVar *       var;
	char *          ptr;
	NihList         methods_prototypes;
	nih_local char *methods_array = NULL;
	NihList         signals_prototypes;
	nih_local char *signals_array = NULL;
	NihList         properties_prototypes;
	nih_local char *properties_array = NULL;
	char *          code;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (prototypes != NULL);

	/* Work out the structure name, and append the interface name to the
	 * definition.
	 */
	name = symbol_impl (NULL, prefix, interface->name, NULL, NULL);
	if (! name)
		return NULL;

	if (! nih_strcat_sprintf (&block, NULL, "\"%s\",\n", interface->name))
		return NULL;

	/* Append the methods array to the arrays block, making it static
	 * in the process.
	 */
	nih_list_init (&methods_prototypes);

	methods_array = interface_methods_array (NULL, prefix, interface,
						 object ? TRUE : FALSE,
						 &methods_prototypes);
	if (! methods_array)
		return NULL;

	nih_assert (! NIH_LIST_EMPTY (&methods_prototypes));

	var = (TypeVar *)methods_prototypes.next;
	ptr = strstr (methods_array, var->type);
	nih_assert (ptr != NULL);

	if (! nih_strncat (&arrays, NULL, methods_array, ptr - methods_array))
		return NULL;

	if (! nih_strcat_sprintf (&arrays, NULL, "static %s\n", ptr))
		return NULL;

	if (! nih_strcat_sprintf (&block, NULL, "%s,\n", var->name))
		return NULL;

	/* Append the signals array to the arrays block, making it static
	 * in the process.
	 */
	nih_list_init (&signals_prototypes);

	signals_array = interface_signals_array (NULL, prefix, interface,
						 object ? FALSE : TRUE,
						 &signals_prototypes);
	if (! signals_array)
		return NULL;

	nih_assert (! NIH_LIST_EMPTY (&signals_prototypes));

	var = (TypeVar *)signals_prototypes.next;
	ptr = strstr (signals_array, var->type);
	nih_assert (ptr != NULL);

	if (! nih_strncat (&arrays, NULL, signals_array, ptr - signals_array))
		return NULL;

	if (! nih_strcat_sprintf (&arrays, NULL, "static %s\n", ptr))
		return NULL;

	if (! nih_strcat_sprintf (&block, NULL, "%s,\n", var->name))
		return NULL;

	/* Append the properties array to the arrays block, making it static
	 * in the process.
	 */
	nih_list_init (&properties_prototypes);

	properties_array = interface_properties_array (NULL, prefix, interface,
						       object ? TRUE : FALSE,
						       &properties_prototypes);
	if (! properties_array)
		return NULL;

	nih_assert (! NIH_LIST_EMPTY (&properties_prototypes));

	var = (TypeVar *)properties_prototypes.next;

	if (! nih_strcat_sprintf (&arrays, NULL, "static %s\n",
				  properties_array))
		return NULL;

	if (! nih_strcat_sprintf (&block, NULL, "%s\n", var->name))
		return NULL;

	/* Output the code */
	if (! indent (&block, NULL, 1))
		return NULL;

	code = nih_sprintf (parent,
			    "%s"
			    "const NihDBusInterface %s = {\n"
			    "%s"
			    "};\n",
			    arrays,
			    name,
			    block);
	if (! code)
		return NULL;

	/* Append the prototype to the list */
	var = type_var_new (code, "const NihDBusInterface", name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	nih_list_add (prototypes, &var->entry);

	return code;
}
