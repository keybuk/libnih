/* nih-dbus-tool
 *
 * argument.c - argument parsing and handling
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

#include <nih-dbus/dbus_object.h>

#include "symbol.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"


/**
 * argument_name_valid:
 * @name: Member name to verify.
 *
 * Verifies whether @name matches the specification for a D-Bus interface
 * member name, and thus is valid for a argument.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
argument_name_valid (const char *name)
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

	/* Name must be at least 1 character */
	if (strlen (name) < 1)
		return FALSE;

	return TRUE;
}


/**
 * argument_new:
 * @parent: parent object for new argument,
 * @name: D-Bus name of argument,
 * @type: D-Bus type argument,
 * @direction: argument direction.
 *
 * Allocates a new D-Bus object Argument data structure, with the D-Bus name
 * optionally set to @name and the D-Bus type signature set to @type.  The
 * returned structure is not placed into any list.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned argument.  When all parents
 * of the returned argument are freed, the returned argument will also be
 * freed.
 *
 * Returns: the new argument or NULL if the allocation failed.
 **/
Argument *
argument_new (const void *  parent,
	      const char *  name,
	      const char *  type,
	      NihDBusArgDir direction)
{
	Argument *argument;

	nih_assert (type != NULL);

	argument = nih_new (parent, Argument);
	if (! argument)
		return NULL;

	nih_list_init (&argument->entry);

	nih_alloc_set_destructor (argument, nih_list_destroy);

	if (name) {
		argument->name = nih_strdup (argument, name);
		if (! argument->name) {
			nih_free (argument);
			return NULL;
		}
	} else {
		argument->name = NULL;
	}

	argument->symbol = NULL;

	argument->type = nih_strdup (argument, type);
	if (! argument->type) {
		nih_free (argument);
		return NULL;
	}

	argument->direction = direction;

	return argument;
}


/**
 * argument_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for an "argument"
 * start tag, which may be a child of either the "method" or "signal" tags
 * defining an argument for the method or signal.
 *
 * If the argument does not appear within a method or signal tag a warning
 * is emitted and the tag will be ignored.
 *
 * Arguments must have a "type" attribute containing the D-Bus type
 * signature, they usually have a "name" attribute specifying the D-Bus name
 * but it's technically optional and they may also have a "direction"
 * attribute specifying whether the argument is input (default for methods)
 * or output (default for signals).
 *
 * Any unknown attributes result in a warning and will be ignored.
 *
 * An Argument object will be allocated and pushed onto the stack, this is
 * not added to the method or signal until the end tag is found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
argument_start_tag (XML_Parser    xmlp,
		    const char *  tag,
		    char * const *attr)
{
	ParseContext *      context;
	ParseStack *        parent;
	nih_local Argument *argument = NULL;
	char * const *      key;
	char * const *      value;
	const char *        name = NULL;
	const char *        type = NULL;
	const char *        direction_str = NULL;
	NihDBusArgDir       direction;
	DBusError           error;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Arguments should only appear inside methods or signals. */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || ((parent->type != PARSE_METHOD)
			   && (parent->type != PARSE_SIGNAL)))
	{
		nih_warn ("%s:%zu:%zu: %s", context->filename,
			  (size_t)XML_GetCurrentLineNumber (xmlp),
			  (size_t)XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unexpected <arg> tag"));

		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL))
			nih_return_system_error (-1);

		return 0;
	}

	/* Retrieve the name, type and direction from the attributes */
	for (key = attr; key && *key; key += 2) {
		value = key + 1;
		nih_assert (value && *value);

		if (! strcmp (*key, "name")) {
			name = *value;
		} else if (! strcmp (*key, "type")) {
			type = *value;
		} else if (! strcmp (*key, "direction")) {
			direction_str = *value;
		} else {
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown <arg> attribute"),
				  *key);
		}
	}

	/* Check we have a type and that it's valid (name and direction
	 * are optional).  We also check the name is valid according to
	 * member rules, strictly speaking there is no such restriction,
	 * but we hereby invent one.
	 */
	if (name && (! argument_name_valid (name)))
		nih_return_error (-1, ARGUMENT_INVALID_NAME,
				  _(ARGUMENT_INVALID_NAME_STR));

	if (! type)
		nih_return_error (-1, ARGUMENT_MISSING_TYPE,
				  _(ARGUMENT_MISSING_TYPE_STR));

	dbus_error_init (&error);
	if (! dbus_signature_validate_single (type, &error)) {
		nih_error_raise_printf (ARGUMENT_INVALID_TYPE, "%s: %s",
					_(ARGUMENT_INVALID_TYPE_STR),
					error.message);
		dbus_error_free (&error);
		return -1;
	}

	switch (parent->type) {
	case PARSE_METHOD:
		if (! direction_str) {
			direction = NIH_DBUS_ARG_IN;
		} else if (! strcmp (direction_str, "in")) {
			direction = NIH_DBUS_ARG_IN;
		} else if (! strcmp (direction_str, "out")) {
			direction = NIH_DBUS_ARG_OUT;
		} else {
			nih_return_error (-1, ARGUMENT_ILLEGAL_METHOD_DIRECTION,
					  _(ARGUMENT_ILLEGAL_METHOD_DIRECTION_STR));
		}
		break;
	case PARSE_SIGNAL:
		if (! direction_str) {
			direction = NIH_DBUS_ARG_OUT;
		} else if (! strcmp (direction_str, "out")) {
			direction = NIH_DBUS_ARG_OUT;
		} else {
			nih_return_error (-1, ARGUMENT_ILLEGAL_SIGNAL_DIRECTION,
					  _(ARGUMENT_ILLEGAL_SIGNAL_DIRECTION_STR));
		}
		break;
	default:
		nih_assert_not_reached ();
	}

	/* Allocate an Argument object and push onto the stack */
	argument = argument_new (NULL, name, type, direction);
	if (! argument)
		nih_return_system_error (-1);

	if (! parse_stack_push (NULL, &context->stack,
				PARSE_ARGUMENT, argument)) {
		nih_error_raise_system ();
		return -1;
	}

	return 0;
}

/**
 * argument_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for an "argument" end
 * tag, and matches a call to argument_start_tag() made at the same
 * parsing level.
 *
 * The argument is added to the list of arguments for the parent method
 * or signal.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
argument_end_tag (XML_Parser  xmlp,
		  const char *tag)
{
	ParseContext *context;
	ParseStack *  entry;
	ParseStack *  parent;
	Argument *    argument;
	Argument *    conflict;
	Method *      method;
	Signal *      signal;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	nih_assert (entry->type == PARSE_ARGUMENT);
	argument = entry->argument;

	/* Generate a symbol from the name if we have one */
	if (argument->name && (! argument->symbol)) {
		argument->symbol = symbol_from_name (argument, argument->name);
		if (! argument->symbol)
			nih_return_no_memory_error (-1);
	}

	nih_list_remove (&entry->entry);
	parent = parse_stack_top (&context->stack);
	nih_assert (parent != NULL);

	switch (parent->type) {
	case PARSE_METHOD:
		method = parent->method;

		/* Otherwise generate a symbol from the argument count */
		if (! argument->symbol) {
			size_t count = 0;

			NIH_LIST_FOREACH (&method->arguments, iter)
				count++;

			argument->symbol = nih_sprintf (argument, "arg%zu",
							++count);
			if (! argument->symbol) {
				nih_list_add_after (&context->stack,
						    &entry->entry);
				nih_return_no_memory_error (-1);
			}
		}

		/* Make sure there's not a conflict before adding the arg */
		conflict = method_lookup_argument (method, argument->symbol);
		if (conflict) {
			nih_error_raise_printf (ARGUMENT_DUPLICATE_SYMBOL,
						_(ARGUMENT_DUPLICATE_SYMBOL_STR),
						argument->symbol, conflict->name);
			return -1;
		}

		nih_debug ("Add %s argument to %s method",
			   argument->name ?: "(unknown)",
			   method->name);
		nih_list_add (&method->arguments, &argument->entry);
		nih_ref (argument, method);
		break;
	case PARSE_SIGNAL:
		signal = parent->signal;

		/* Otherwise generate a symbol from the argument count */
		if (! argument->symbol) {
			size_t count = 0;

			NIH_LIST_FOREACH (&signal->arguments, iter)
				count++;

			argument->symbol = nih_sprintf (argument, "arg%zu",
							++count);
			if (! argument->symbol) {
				nih_list_add_after (&context->stack,
						    &entry->entry);
				nih_return_no_memory_error (-1);
			}
		}

		/* Make sure there's not a conflict before adding the arg */
		conflict = signal_lookup_argument (signal, argument->symbol);
		if (conflict) {
			nih_error_raise_printf (ARGUMENT_DUPLICATE_SYMBOL,
						_(ARGUMENT_DUPLICATE_SYMBOL_STR),
						argument->symbol, conflict->name);
			return -1;
		}

		nih_debug ("Add %s argument to %s signal",
			   argument->name ?: "(unknown)",
			   signal->name);
		nih_list_add (&signal->arguments, &argument->entry);
		nih_ref (argument, signal);
		break;
	default:
		nih_assert_not_reached ();
	}

	nih_free (entry);

	return 0;
}


/**
 * argument_annotation:
 * @argument: argument object annotation applies to,
 * @name: annotation name,
 * @value: annotation value.
 *
 * Handles applying the annotation @name with value @value to the argument
 * @argument.  While the D-Bus Introspection specification does not permit
 * annotations for arguments, this is an nih-dbus-tool extension.  Arguments
 * may be annotated with an alternate symbol name specified.
 *
 * Unknown annotations or illegal values to the known annotations result
 * in an error being raised.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
argument_annotation (Argument *  argument,
		     const char *name,
		     const char *value)
{
	nih_assert (argument != NULL);
	nih_assert (name != NULL);
	nih_assert (value != NULL);

	if (! strcmp (name, "com.netsplit.Nih.Symbol")) {
		if (symbol_valid (value)) {
			if (argument->symbol)
				nih_unref (argument->symbol, argument);

			argument->symbol = nih_strdup (argument, value);
			if (! argument->symbol)
				nih_return_no_memory_error (-1);

			nih_debug ("Set %s argument symbol to %s",
				   argument->name ?: "(unknown)", argument->symbol);
		} else {
			nih_return_error (-1, ARGUMENT_INVALID_SYMBOL,
					  _(ARGUMENT_INVALID_SYMBOL_STR));
		}
	} else {
		nih_error_raise_printf (ARGUMENT_UNKNOWN_ANNOTATION,
					"%s: %s: %s",
					_(ARGUMENT_UNKNOWN_ANNOTATION_STR),
					argument->name ?: "(unnamed)",
					name);
		return -1;
	}

	return 0;
}
