/* nih-dbus-tool
 *
 * annotation.c - annotation parsing and handling
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

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/logging.h>

#include "annotation.h"
#include "parse.h"
#include "errors.h"


/**
 * annotation_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for an "annotation"
 * start tag, this may be a child of the "interface", "method", "signal",
 * "property" or (nih extension) "argument" tags and specifies a further
 * property not defined by the Introspection specification.
 *
 * If the annotation does not appear within one of the permitted tags a
 * warning is emitted and the tag will be ignored.
 *
 * Annotations must have a "name" attribute containing the well-known
 * annotation name and a "value" attribute contianing the value.
 *
 * Any unknown attributes result in a warning and will be ignored.
 *
 * The appropriate *_annotation() function is called to handle
 * identifying the annotation and applying it to the parent object.  This
 * may result in a warning being emitted if the annotation is unknown or
 * an error if the value is not permitted.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
annotation_start_tag (XML_Parser    xmlp,
		      const char *  tag,
		      char * const *attr)
{
	ParseContext *context;
	ParseStack *  parent;
	char * const *key;
	char * const *value;
	const char *  name = NULL;
	const char *  val = NULL;
	int           ret;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Annotations apply to their parent tag */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || (   (parent->type != PARSE_INTERFACE)
			   && (parent->type != PARSE_METHOD)
			   && (parent->type != PARSE_SIGNAL)
			   && (parent->type != PARSE_PROPERTY)
			   && (parent->type != PARSE_ARGUMENT)))
	{
		nih_warn ("%s:%zu:%zu: %s", context->filename,
			  (size_t)XML_GetCurrentLineNumber (xmlp),
			  (size_t)XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unexpected <annotation> tag"));

		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL))
			nih_return_system_error (-1);

		return 0;
	}

	/* Retrieve the name and value from the attributes */
	for (key = attr; key && *key; key += 2) {
		value = key + 1;
		nih_assert (value && *value);

		if (! strcmp (*key, "name")) {
			name = *value;
		} else if (! strcmp (*key, "value")) {
			val = *value;
		} else {
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown <annotation> attribute"),
				  *key);
		}
	}

	/* Check we have a name and value */
	if (! name)
		nih_return_error (-1, ANNOTATION_MISSING_NAME,
				  _(ANNOTATION_MISSING_NAME_STR));
	if (! val)
		nih_return_error (-1, ANNOTATION_MISSING_VALUE,
				  _(ANNOTATION_MISSING_VALUE_STR));

	/* Meaning of the annotation depends on the parent */
	switch (parent->type) {
	case PARSE_INTERFACE:
		ret = interface_annotation (parent->interface, name, val);
		break;
	case PARSE_METHOD:
		ret = method_annotation (parent->method, name, val);
		break;
	case PARSE_SIGNAL:
		ret = signal_annotation (parent->signal, name, val);
		break;
	case PARSE_PROPERTY:
		ret = property_annotation (parent->property, name, val);
		break;
	case PARSE_ARGUMENT:
		ret = argument_annotation (parent->argument, name, val);
		break;
	default:
		nih_assert_not_reached ();
	}

	if (ret < 0) {
		NihError *err;

		err = nih_error_get ();
		switch (err->number) {
		case INTERFACE_UNKNOWN_ANNOTATION:
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown interface annotation"),
				  name);
			break;
		case METHOD_UNKNOWN_ANNOTATION:
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown method annotation"),
				  name);
			break;
		case SIGNAL_UNKNOWN_ANNOTATION:
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown signal annotation"),
				  name);
			break;
		case PROPERTY_UNKNOWN_ANNOTATION:
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown property annotation"),
				  name);
			break;
		case ARGUMENT_UNKNOWN_ANNOTATION:
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown argument annotation"),
				  name);
			break;
		default:
			return -1;
		}
		nih_free (err);

		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL)) {
			nih_error_raise_system ();
			return -1;
		}
	} else {
		if (! parse_stack_push (NULL, &context->stack,
					PARSE_ANNOTATION, NULL)) {
			nih_error_raise_system ();
			return -1;
		}
	}

	return 0;
}

/**
 * annotation_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for an "annotation" end
 * tag, and matches a call to annotation_start_tag() made at the same
 * parsing level.
 *
 * The object on the stack is always discarded.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
annotation_end_tag (XML_Parser  xmlp,
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
	nih_assert (entry->type == PARSE_ANNOTATION);

	nih_free (entry);

	return 0;
}
