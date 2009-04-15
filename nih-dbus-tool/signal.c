/* nih-dbus-tool
 *
 * signal.c - signal parsing and generation
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
#include "signal.h"
#include "argument.h"
#include "parse.h"
#include "errors.h"


/**
 * signal_name_valid:
 * @name: Member name to verify.
 *
 * Verifies whether @name matches the specification for a D-Bus interface
 * member name, and thus is valid for a signal.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
signal_name_valid (const char *name)
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
 * signal_new:
 * @parent: parent object for new signal,
 * @name: D-Bus name of signal.
 *
 * Allocates a new D-Bus object Signal data structure, with the D-Bus name
 * set to @name.  The returned structure is not placed into any list and
 * has no arguments.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned signal.  When all parents
 * of the returned signal are freed, the returned signal will also be
 * freed.
 *
 * Returns: the new signal or NULL if the allocation failed.
 **/
Signal *
signal_new (const void *parent,
	    const char *name)
{
	Signal *signal;

	nih_assert (name != NULL);

	signal = nih_new (parent, Signal);
	if (! signal)
		return NULL;

	nih_list_init (&signal->entry);

	nih_alloc_set_destructor (signal, nih_list_destroy);

	signal->name = nih_strdup (signal, name);
	if (! signal->name) {
		nih_free (signal);
		return NULL;
	}

	signal->symbol = NULL;
	signal->deprecated = FALSE;

	nih_list_init (&signal->arguments);

	return signal;
}


/**
 * signal_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for a "signal"
 * start tag, a child of the "interface" tag that defines a signal the
 * D-Bus interface specifies.
 *
 * If the method does not appear within an interface tag a warning is
 * emitted and the tag will be ignored.
 *
 * Signals must have a "name" attribute containing the D-Bus name
 * of the signal.
 *
 * Any unknown attributes result in a warning and will be ignored.
 *
 * A Signal object will be allocated and pushed onto the stack, this is
 * not added to the interface until the end tag is found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
signal_start_tag (XML_Parser    xmlp,
		  const char   *tag,
		  char * const *attr)
{
	ParseContext *context;
	ParseStack   *parent;
	Signal       *signal;
	char * const *key;
	char * const *value;
	const char   *name = NULL;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Signals should only appear inside interfaces. */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || (parent->type != PARSE_INTERFACE)) {
		nih_warn ("%s:%zu:%zu: %s", context->filename,
			  (size_t)XML_GetCurrentLineNumber (xmlp),
			  (size_t)XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unexpected <signal> tag"));

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
				  _("Ignored unknown <signal> attribute"),
				  *key);
		}
	}

	/* Check we have a name and that it's valid */
	if (! name)
		nih_return_error (-1, SIGNAL_MISSING_NAME,
				  _(SIGNAL_MISSING_NAME_STR));
	if (! signal_name_valid (name))
		nih_return_error (-1, SIGNAL_INVALID_NAME,
				  _(SIGNAL_INVALID_NAME_STR));

	/* Allocate a Signal object and push onto the stack */
	signal = signal_new (NULL, name);
	if (! signal)
		nih_return_system_error (-1);

	if (! parse_stack_push (NULL, &context->stack, PARSE_SIGNAL, signal)) {
		nih_error_raise_system ();
		nih_free (signal);
		return -1;
	}

	return 0;
}

/**
 * signal_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for a "signal" end
 * tag, and matches a call to signal_start_tag() made at the same parsing
 * level.
 *
 * The signal is added to the list of signals defined by the parent
 * interface.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
signal_end_tag (XML_Parser  xmlp,
		const char *tag)
{
	ParseContext *context;
	ParseStack   *entry, *parent;
	Signal       *signal, *conflict;
	Interface    *interface;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	nih_assert (entry->type == PARSE_SIGNAL);
	signal = entry->signal;

	/* Generate a symbol from the name */
	if (! signal->symbol) {
		signal->symbol = symbol_from_name (signal, signal->name);
		if (! signal->symbol)
			nih_return_no_memory_error (-1);
	}

	nih_list_remove (&entry->entry);
	parent = parse_stack_top (&context->stack);
	nih_assert (parent != NULL);
	nih_assert (parent->type == PARSE_INTERFACE);
	interface = parent->interface;

	/* Make sure there's not a conflict before adding the signal */
	conflict = interface_lookup_signal (interface, signal->symbol);
	if (conflict) {
		nih_error_raise_printf (SIGNAL_DUPLICATE_SYMBOL,
					_(SIGNAL_DUPLICATE_SYMBOL_STR),
					signal->symbol, conflict->name);
		return -1;
	}

	nih_debug ("Add %s signal to %s interface",
		   signal->name, interface->name);
	nih_list_add (&interface->signals, &signal->entry);
	nih_ref (signal, interface);

	nih_free (entry);

	return 0;
}


/**
 * signal_annotation:
 * @signal: signal object annotation applies to,
 * @name: annotation name,
 * @value: annotation value.
 *
 * Handles applying the annotation @name with value @value to the signal
 * @signal.  Signals may be annotated as deprecated or may have an alternate
 * symbol name specified.
 *
 * Unknown annotations or illegal values to the known annotations result
 * in an error being raised.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
signal_annotation (Signal     *signal,
		   const char *name,
		   const char *value)
{
	nih_assert (signal != NULL);
	nih_assert (name != NULL);
	nih_assert (value != NULL);

	if (! strcmp (name, "org.freedesktop.DBus.Deprecated")) {
		if (! strcmp (value, "true")) {
			nih_debug ("Marked %s signal as deprecated",
				   signal->name);
			signal->deprecated = TRUE;
		} else if (! strcmp (value, "false")) {
			nih_debug ("Marked %s signal as not deprecated",
				   signal->name);
			signal->deprecated = FALSE;
		} else {
			nih_return_error (-1, SIGNAL_ILLEGAL_DEPRECATED,
					  _(SIGNAL_ILLEGAL_DEPRECATED_STR));
		}

	} else if (! strcmp (name, "com.netsplit.Nih.Symbol")) {
		if (symbol_valid (value)) {
			if (signal->symbol)
				nih_unref (signal->symbol, signal);

			signal->symbol = nih_strdup (signal, value);
			if (! signal->symbol)
				nih_return_no_memory_error (-1);

			nih_debug ("Set %s signal symbol to %s",
				   signal->name, signal->symbol);
		} else {
			nih_return_error (-1, SIGNAL_INVALID_SYMBOL,
					  _(SIGNAL_INVALID_SYMBOL_STR));
		}

	} else {
		nih_error_raise_printf (SIGNAL_UNKNOWN_ANNOTATION,
					"%s: %s: %s",
					_(SIGNAL_UNKNOWN_ANNOTATION_STR),
					signal->name, name);
		return -1;
	}

	return 0;
}


/**
 * signal_lookup_argument:
 * @signal: signal to search,
 * @symbol: argument symbol to find.
 *
 * Finds a argument in @signal's arguments list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: argument found or NULL if no argument matches.
 **/
Argument *
signal_lookup_argument (Signal  *signal,
			 const char *symbol)
{
	nih_assert (signal != NULL);
	nih_assert (symbol != NULL);

	NIH_LIST_FOREACH (&signal->arguments, iter) {
		Argument *argument = (Argument *)iter;

		if (argument->symbol
		    && (! strcmp (argument->symbol, symbol)))
			return argument;
	}

	return NULL;
}
