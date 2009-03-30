/* nih-dbus-tool
 *
 * method.c - method parsing and generation
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
#include "interface.h"
#include "method.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"


/**
 * method_name_valid:
 * @name: Member name to verify.
 *
 * Verifies whether @name matches the specification for a D-Bus interface
 * member name, and thus is valid for a method.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
method_name_valid (const char *name)
{
	nih_assert (name != NULL);

	/* We can get away with just using strlen() here even through name
	 * is in UTF-8 because all the valid characters are ASCII.
	 */
	for (size_t i = 0; i < strlen (name); i++) {
		/* Names may contain digits, but not at the beginning. */
		if ((name[i] >= '0') && (name[i] <= '9')) {
			if (i == 0)
				return FALSE;

			continue;
		}

		/* Valid characters anywhere are [A-Za-z_] */
		if (   ((name[i] < 'A') || (name[i] > 'Z'))
		    && ((name[i] < 'a') || (name[i] > 'z'))
		    && (name[i] != '_'))
			return FALSE;
	}

	/* Name must be at least 1 character and no more than 255 characters */
	if ((strlen (name) < 1) || (strlen (name) > 255))
		return FALSE;

	return TRUE;
}


/**
 * method_new:
 * @parent: parent object for new method,
 * @name: D-Bus name of method.
 *
 * Allocates a new D-Bus object Method data structure, with the D-Bus name
 * set to @name.  The returned structure is not placed into any list and
 * has no arguments.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned method.  When all parents
 * of the returned method are freed, the returned method will also be
 * freed.
 *
 * Returns: the new method or NULL if the allocation failed.
 **/
Method *
method_new (const void *parent,
	    const char *name)
{
	Method *method;

	nih_assert (name != NULL);

	method = nih_new (parent, Method);
	if (! method)
		return NULL;

	nih_list_init (&method->entry);

	nih_alloc_set_destructor (method, nih_list_destroy);

	method->name = nih_strdup (method, name);
	if (! method->name) {
		nih_free (method);
		return NULL;
	}

	method->symbol = NULL;
	method->async = FALSE;
	method->no_reply = FALSE;
	method->deprecated = FALSE;

	nih_list_init (&method->arguments);

	return method;
}


/**
 * method_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for a "method"
 * start tag, a child of the "interface" tag that defines a method the
 * D-Bus interface specifies.
 *
 * If the method does not appear within an interface tag a warning is
 * emitted and the tag will be ignored.
 *
 * Methods must have a "name" attribute containing the D-Bus name of the
 * method.
 *
 * Any unknown attributes result in a warning and will be ignored.
 *
 * A Method object will be allocated and pushed onto the stack, this is
 * not added to the interface until the end tag is found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
method_start_tag (XML_Parser    xmlp,
		  const char   *tag,
		  char * const *attr)
{
	ParseContext *context;
	ParseStack   *parent;
	Method       *method;
	char * const *key;
	char * const *value;
	const char   *name = NULL;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Methods should only appear inside interfaces. */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || (parent->type != PARSE_INTERFACE)) {
		nih_warn ("%s:%zi:%zi: %s", context->filename,
			  XML_GetCurrentLineNumber (xmlp),
			  XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unexpected <method> tag"));

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
				  _("Ignored unknown <method> attribute"),
				  *key);
		}
	}

	/* Check we have a name and that it's valid */
	if (! name)
		nih_return_error (-1, METHOD_MISSING_NAME,
				  _(METHOD_MISSING_NAME_STR));
	if (! method_name_valid (name))
		nih_return_error (-1, METHOD_INVALID_NAME,
				  _(METHOD_INVALID_NAME_STR));

	/* Allocate a Method object and push onto the stack */
	method = method_new (NULL, name);
	if (! method)
		nih_return_system_error (-1);

	if (! parse_stack_push (NULL, &context->stack, PARSE_METHOD, method)) {
		nih_error_raise_system ();
		nih_free (method);
		return -1;
	}

	return 0;
}

/**
 * method_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for a "method" end
 * tag, and matches a call to method_start_tag() made at the same parsing
 * level.
 *
 * The method is added to the list of methods defined by the parent
 * interface.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
method_end_tag (XML_Parser  xmlp,
		const char *tag)
{
	ParseContext *context;
	ParseStack   *entry, *parent;
	Method       *method, *conflict;
	Interface    *interface;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	nih_assert (entry->type == PARSE_METHOD);
	method = entry->method;

	/* Generate a symbol from the name */
	if (! method->symbol) {
		method->symbol = symbol_from_name (method, method->name);
		if (! method->symbol)
			nih_return_no_memory_error (-1);
	}

	nih_list_remove (&entry->entry);
	parent = parse_stack_top (&context->stack);
	nih_assert (parent != NULL);
	nih_assert (parent->type == PARSE_INTERFACE);
	interface = parent->interface;

	/* Make sure there's not a conflict before adding the method */
	conflict = interface_lookup_method (interface, method->symbol);
	if (conflict) {
		nih_error_raise_printf (METHOD_DUPLICATE_SYMBOL,
					_(METHOD_DUPLICATE_SYMBOL_STR),
					method->symbol, conflict->name);
		return -1;
	}

	nih_debug ("Add %s method to %s interface",
		   method->name, interface->name);
	nih_list_add (&interface->methods, &method->entry);
	nih_ref (method, interface);

	nih_free (entry);

	return 0;
}


/**
 * method_annotation:
 * @method: method object annotation applies to,
 * @name: annotation name,
 * @value: annotation value.
 *
 * Handles applying the annotation @name with value @value to the method
 * @method.  Methods may be annotated as deprecated, that a client should
 * expect no reply, an alternate symbol name may be specified or that the
 * object implementation will be asynchronous.
 *
 * Unknown annotations or illegal values to the known annotations result
 * in an error being raised.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
method_annotation (Method     *method,
		   const char *name,
		   const char *value)
{
	nih_assert (method != NULL);
	nih_assert (name != NULL);
	nih_assert (value != NULL);

	if (! strcmp (name, "org.freedesktop.DBus.Deprecated")) {
		if (! strcmp (value, "true")) {
			nih_debug ("Marked %s method as deprecated",
				   method->name);
			method->deprecated = TRUE;
		} else if (! strcmp (value, "false")) {
			nih_debug ("Marked %s method as not deprecated",
				   method->name);
			method->deprecated = FALSE;
		} else {
			nih_return_error (-1, METHOD_ILLEGAL_DEPRECATED,
					  _(METHOD_ILLEGAL_DEPRECATED_STR));
		}

	} else if (! strcmp (name, "org.freedesktop.DBus.Method.NoReply")) {
		if (! strcmp (value, "true")) {
			nih_debug ("Marked %s method to expect no reply",
				   method->name);
			method->no_reply = TRUE;
		} else if (! strcmp (value, "false")) {
			nih_debug ("Marked %s method to expect a reply",
				   method->name);
			method->no_reply = FALSE;
		} else {
			nih_return_error (-1, METHOD_ILLEGAL_NO_REPLY,
					  _(METHOD_ILLEGAL_NO_REPLY_STR));
		}

	} else if (! strcmp (name, "com.netsplit.Nih.Symbol")) {
		if (symbol_valid (value)) {
			if (method->symbol)
				nih_unref (method->symbol, method);

			method->symbol = nih_strdup (method, value);
			if (! method->symbol)
				nih_return_no_memory_error (-1);

			nih_debug ("Set %s method symbol to %s",
				   method->name, method->symbol);
		} else {
			nih_return_error (-1, METHOD_INVALID_SYMBOL,
					  _(METHOD_INVALID_SYMBOL_STR));
		}

	} else if (! strcmp (name, "com.netsplit.Nih.Method.Async")) {
		if (! strcmp (value, "true")) {
			nih_debug ("Marked %s method as async",
				   method->name);
			method->async = TRUE;
		} else if (! strcmp (value, "false")) {
			nih_debug ("Marked %s method as non-async",
				   method->name);
			method->async = FALSE;
		} else {
			nih_return_error (-1, METHOD_ILLEGAL_ASYNC,
					  _(METHOD_ILLEGAL_ASYNC_STR));
		}

	} else {
		nih_error_raise_printf (METHOD_UNKNOWN_ANNOTATION,
					"%s: %s: %s",
					_(METHOD_UNKNOWN_ANNOTATION_STR),
					method->name, name);
		return -1;
	}

	return 0;
}


/**
 * method_lookup_argument:
 * @method: method to search,
 * @symbol: argument symbol to find.
 *
 * Finds a argument in @method's arguments list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: argument found or NULL if no argument matches.
 **/
Argument *
method_lookup_argument (Method  *method,
			 const char *symbol)
{
	nih_assert (method != NULL);
	nih_assert (symbol != NULL);

	NIH_LIST_FOREACH (&method->arguments, iter) {
		Argument *argument = (Argument *)iter;

		if (argument->symbol
		    && (! strcmp (argument->symbol, symbol)))
			return argument;
	}

	return NULL;
}
