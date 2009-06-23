/* nih-dbus-tool
 *
 * parse.c - parse XML introspection data and tool-specific annotations
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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/error.h>
#include <nih/logging.h>

#include "node.h"
#include "interface.h"
#include "method.h"
#include "signal.h"
#include "property.h"
#include "argument.h"
#include "annotation.h"
#include "parse.h"
#include "errors.h"


/**
 * BUF_SIZE:
 *
 * Size of buffer we use when parsing.
 **/
#define BUF_SIZE 80


/**
 * parse_stack_push:
 * @parent: parent object of new stack entry,
 * @stack: stack to push onto,
 * @type: type of object to push,
 * @data: pointer to object data.
 *
 * Allocates a new Stack object with the @type and @data specified
 * and pushes it onto @stack.
 *
 * The entry can be removed from the stack by freeing it, though this will
 * not free the associated @data unless you arrange that by references.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned entry.  When all parents
 * of the returned node are freed, the returned entry will also be
 * freed.
 *
 * Returns: new entry or NULL if the allocation failed.
 **/
ParseStack *
parse_stack_push (const void *   parent,
		  NihList *      stack,
		  ParseStackType type,
		  void *         data)
{
	ParseStack *entry;

	nih_assert (stack != NULL);

	entry = nih_new (parent, ParseStack);
	if (! entry)
		return NULL;

	nih_list_init (&entry->entry);

	nih_alloc_set_destructor (entry, nih_list_destroy);

	entry->type = type;
	entry->data = data;

	if ((entry->type != PARSE_IGNORED)
	    && (entry->type != PARSE_ANNOTATION)) {
		nih_assert (entry->data != NULL);
		nih_ref (entry->data, entry);
	} else {
		nih_assert (entry->data == NULL);
	}

	nih_list_add_after (stack, &entry->entry);

	return entry;
}

/**
 * parse_stack_top:
 * @stack: stack to return from.
 *
 * Returns: the top entry in @stack or NULL if the stack is empty.
 **/
ParseStack *
parse_stack_top (NihList *stack)
{
	nih_assert (stack != NULL);

	if (! NIH_LIST_EMPTY (stack)) {
		return (ParseStack *)stack->next;
	} else {
		return NULL;
	}
}


/**
 * parse_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is intended to be used as the start element handler for
 * the XML parser @xmlp and called by that parser.  It looks at the tag
 * name @tag and calls one of the other *_start_tag() functions to
 * handle the tag.
 *
 * Unknown tags result in a warning and are otherwise ignored, the stack
 * contains an ignore element and the content of those tags will also be
 * ignored with no warnings generated.
 *
 * Errors are raised and reported by stopping the parser, other element
 * handlers should check that the parsing status is not finished as they
 * may be called as part of the unwinding process.  XML_ParseBuffer will
 * return to indicate an error, the XML error code will be XML_ERROR_ABORTED
 * and the actual error can be retrieved with nih_error_get().
 **/
void
parse_start_tag (XML_Parser    xmlp,
		 const char *  tag,
		 char * const *attr)
{
	XML_ParsingStatus status;
	ParseContext *    context;
	ParseStack *      parent;
	int               ret = 0;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	XML_GetParsingStatus (xmlp, &status);
	if (status.parsing == XML_FINISHED)
		return;

	nih_debug ("Parsed '%s' tag", tag);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Ignore any tag inside an ignored tag */
	parent = parse_stack_top (&context->stack);
	if (parent && (parent->type == PARSE_IGNORED)) {
 		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL)) {
			nih_error_raise_system ();
			ret = -1;
		}

		goto exit;
	}

	/* Otherwise call out to handle the tag */
	if (! strcmp (tag, "node")) {
		ret = node_start_tag (xmlp, tag, attr);
	} else if (! strcmp (tag, "interface")) {
		ret = interface_start_tag (xmlp, tag, attr);
	} else if (! strcmp (tag, "method")) {
		ret = method_start_tag (xmlp, tag, attr);
	} else if (! strcmp (tag, "signal")) {
		ret = signal_start_tag (xmlp, tag, attr);
	} else if (! strcmp (tag, "property")) {
		ret = property_start_tag (xmlp, tag, attr);
	} else if (! strcmp (tag, "arg")) {
		ret = argument_start_tag (xmlp, tag, attr);
	} else if (! strcmp (tag, "annotation")) {
		ret = annotation_start_tag (xmlp, tag, attr);
	} else {
		nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
			  (size_t)XML_GetCurrentLineNumber (xmlp),
			  (size_t)XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unknown tag"),
			  tag);

 		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL)) {
			nih_error_raise_system ();
			ret = -1;
		}
	}

exit:
	if (ret < 0)
		nih_assert (XML_StopParser (xmlp, FALSE) == XML_STATUS_OK);
}

/**
 * parse_end_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is intended to be used as the end element handler for
 * the XML parser @xmlp and called by that parser.  It looks at the tag
 * name @tag and calls one of the other *_end_tag() functions to
 * handle the tag.
 *
 * The end tags whose start was ignored are ignored without any warning.
 *
 * Errors are raised and reported by stopping the parser, other element
 * handlers should check that the parsing status is not finished as they
 * may be called as part of the unwinding process.  XML_ParseBuffer will
 * return to indicate an error, the XML error code will be XML_ERROR_ABORTED
 * and the actual error can be retrieved with nih_error_get().
 **/
void
parse_end_tag (XML_Parser  xmlp,
	       const char *tag)
{
	XML_ParsingStatus status;
	ParseContext *    context;
	ParseStack *      entry;
	int               ret = 0;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	XML_GetParsingStatus (xmlp, &status);
	if (status.parsing == XML_FINISHED)
		return;

	nih_debug ("Parsed '%s' end tag", tag);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Ignore the end tag of any ignored tag */
	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	if (entry->type == PARSE_IGNORED) {
		nih_free (entry);
		goto exit;
	}

	/* Otherwise call out to handle the tag */
	if (! strcmp (tag, "node")) {
		ret = node_end_tag (xmlp, tag);
	} else if (! strcmp (tag, "interface")) {
		ret = interface_end_tag (xmlp, tag);
	} else if (! strcmp (tag, "method")) {
		ret = method_end_tag (xmlp, tag);
	} else if (! strcmp (tag, "signal")) {
		ret = signal_end_tag (xmlp, tag);
	} else if (! strcmp (tag, "property")) {
		ret = property_end_tag (xmlp, tag);
	} else if (! strcmp (tag, "arg")) {
		ret = argument_end_tag (xmlp, tag);
	} else if (! strcmp (tag, "annotation")) {
		ret = annotation_end_tag (xmlp, tag);
	} else {
		nih_assert_not_reached ();
	}

exit:
	if (ret < 0)
		nih_assert (XML_StopParser (xmlp, FALSE) == XML_STATUS_OK);
}


/**
 * parse_xml:
 * @parent: parent object of new node,
 * @fd: file descriptor to parse from,
 * @filename: filename for error reporting.
 *
 * Parse XML data from @fd according to the D-Bus Introspection
 * specification, returning the top-level Node which contains the
 * Interfaces defined by that object.
 *
 * Errors in parsing are output within this function, since it has the
 * line and column number available to it.  @filename is used when reporting
 * these errors.
 *
 * In general, the parser is fairly liberal and will ignore unexpected tags,
 * attributes and any character data.  However it is strict about restrictions
 * in the specification, for example it will not allow missing attributes or
 * unknown values in them.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned node.  When all parents
 * of the returned node are freed, the returned node will also be
 * freed.
 *
 * Returns: newly allocated Node on success, NULL on error.
 **/
Node *
parse_xml (const void *parent,
	   int         fd,
	   const char *filename)
{
	ParseContext context;
	XML_Parser   xmlp;
	ssize_t      len;

	context.parent = parent;
	nih_list_init (&context.stack);
	context.filename = filename;
	context.node = NULL;

	xmlp = XML_ParserCreate ("UTF-8");
	if (! xmlp) {
		nih_error ("%s: %s", _("Unable to create XML Parser"),
			   strerror (ENOMEM));
		return NULL;
	}

	XML_SetUserData (xmlp, &context);
	XML_UseParserAsHandlerArg (xmlp);
	XML_SetElementHandler (xmlp,
			       (XML_StartElementHandler)parse_start_tag,
			       (XML_EndElementHandler)parse_end_tag);

	do {
		enum XML_Status status;
		enum XML_Error  error;
		void *          buf;

		buf = XML_GetBuffer (xmlp, BUF_SIZE);
		if (! buf) {
			error = XML_GetErrorCode (xmlp);
			nih_error ("%s: %s", _("Unable to allocate parsing buffer"),
				   XML_ErrorString (error));
			goto error;
		}

		len = read (fd, buf, BUF_SIZE);
		if (len < 0) {
			nih_error ("%s: %s: %s", context.filename,
				   _("Read error"), strerror (errno));
			goto error;
		}

		status = XML_ParseBuffer (xmlp, len, len == 0 ? TRUE : FALSE);
		if (status != XML_STATUS_OK) {
			error = XML_GetErrorCode (xmlp);

			if (error == XML_ERROR_ABORTED) {
				NihError *err;

				err = nih_error_get ();
				nih_error ("%s:%zu:%zu: %s", context.filename,
					   (size_t)XML_GetCurrentLineNumber (xmlp),
					   (size_t)XML_GetCurrentColumnNumber (xmlp),
					   err->message);
				nih_free (err);
			} else {
				nih_error ("%s:%zu:%zu: %s: %s",
					   context.filename,
					   (size_t)XML_GetCurrentLineNumber (xmlp),
					   (size_t)XML_GetCurrentColumnNumber (xmlp),
					   _("XML parse error"),
					   XML_ErrorString (error));
			}

			goto error;
		}
	} while (len > 0);

	nih_assert (NIH_LIST_EMPTY (&context.stack));

	if (! context.node) {
		nih_error ("%s: %s", context.filename,
			   _("No node present"));
		goto error;
	}

	XML_ParserFree (xmlp);

	return context.node;

error:
	NIH_LIST_FOREACH_SAFE (&context.stack, iter) {
		ParseStack *entry = (ParseStack *)iter;

		nih_free (entry);
	}

	if (context.node)
		nih_free (context.node);

	XML_ParserFree (xmlp);

	return NULL;
}
