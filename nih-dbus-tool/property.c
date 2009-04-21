/* nih-dbus-tool
 *
 * property.c - property parsing and generation
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

#include <nih-dbus/dbus_object.h>

#include "symbol.h"
#include "indent.h"
#include "type.h"
#include "marshal.h"
#include "demarshal.h"
#include "property.h"
#include "interface.h"
#include "property.h"
#include "parse.h"
#include "errors.h"


/**
 * property_name_valid:
 * @name: Member name to verify.
 *
 * Verifies whether @name matches the specification for a D-Bus interface
 * member name, and thus is valid for a property.
 *
 * Returns: TRUE if valid, FALSE if not.
 **/
int
property_name_valid (const char *name)
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
 * property_new:
 * @parent: parent object for new property,
 * @name: D-Bus name of property,
 * @type: D-Bus type signature,
 * @access: access to property.
 *
 * Allocates a new D-Bus object Property data structure, with the D-Bus name
 * set to @name and the D-Bus type signature set to @type.  The returned
 * structure is not placed into any list.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned property.  When all parents
 * of the returned property are freed, the returned property will also be
 * freed.
 *
 * Returns: the new property or NULL if the allocation failed.
 **/
Property *
property_new (const void *  parent,
	      const char *  name,
	      const char *  type,
	      NihDBusAccess access)
{
	Property *property;

	nih_assert (name != NULL);

	property = nih_new (parent, Property);
	if (! property)
		return NULL;

	nih_list_init (&property->entry);

	nih_alloc_set_destructor (property, nih_list_destroy);

	property->name = nih_strdup (property, name);
	if (! property->name) {
		nih_free (property);
		return NULL;
	}

	property->symbol = NULL;

	property->type = nih_strdup (property, type);
	if (! property->type) {
		nih_free (property);
		return NULL;
	}

	property->access = access;
	property->deprecated = FALSE;

	return property;
}


/**
 * property_start_tag:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed,
 * @attr: NULL-terminated array of attribute name and value pairs.
 *
 * This function is called by parse_start_tag() for a "property"
 * start tag, a child of the "interface" tag that defines a property the
 * D-Bus interface specifies.
 *
 * If the property does not appear within an interface tag a warning is
 * emitted and the tag will be ignored.
 *
 * Properties must have a "name" attribute containing the D-Bus name
 * of the interface, a "type" attribute containing the D-Bus type
 * signature and an "access" attribute specifying whether the property
 * is read-only, write-only or read/write.
 *
 * Any unknown attributes result in a warning and will be ignored; an
 * unknown value for the "access" attribute results in an error.
 *
 * A Property object will be allocated and pushed onto the stack, this is
 * not added to the interface until the end tag is found.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
property_start_tag (XML_Parser    xmlp,
		    const char *  tag,
		    char * const *attr)
{
	ParseContext *context;
	ParseStack *  parent;
	Property *    property;
	char * const *key;
	char * const *value;
	const char *  name = NULL;
	const char *  type = NULL;
	const char *  access_str = NULL;
	NihDBusAccess access;
	DBusError     error;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);
	nih_assert (attr != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	/* Properties should only appear inside interfaces. */
	parent = parse_stack_top (&context->stack);
	if ((! parent) || (parent->type != PARSE_INTERFACE)) {
		nih_warn ("%s:%zu:%zu: %s", context->filename,
			  (size_t)XML_GetCurrentLineNumber (xmlp),
			  (size_t)XML_GetCurrentColumnNumber (xmlp),
			  _("Ignored unexpected <property> tag"));

		if (! parse_stack_push (NULL, &context->stack,
					PARSE_IGNORED, NULL))
			nih_return_system_error (-1);

		return 0;
	}

	/* Retrieve the name, type and access from the attributes */
	for (key = attr; key && *key; key += 2) {
		value = key + 1;
		nih_assert (value && *value);

		if (! strcmp (*key, "name")) {
			name = *value;
		} else if (! strcmp (*key, "type")) {
			type = *value;
		} else if (! strcmp (*key, "access")) {
			access_str = *value;
		} else {
			nih_warn ("%s:%zu:%zu: %s: %s", context->filename,
				  (size_t)XML_GetCurrentLineNumber (xmlp),
				  (size_t)XML_GetCurrentColumnNumber (xmlp),
				  _("Ignored unknown <property> attribute"),
				  *key);
		}
	}

	/* Check we have a name, type and access and that they are valid */
	if (! name)
		nih_return_error (-1, PROPERTY_MISSING_NAME,
				  _(PROPERTY_MISSING_NAME_STR));
	if (! property_name_valid (name))
		nih_return_error (-1, PROPERTY_INVALID_NAME,
				  _(PROPERTY_INVALID_NAME_STR));

	if (! type)
		nih_return_error (-1, PROPERTY_MISSING_TYPE,
				  _(PROPERTY_MISSING_TYPE_STR));

	dbus_error_init (&error);
	if (! dbus_signature_validate_single (type, &error)) {
		nih_error_raise_printf (PROPERTY_INVALID_TYPE, "%s: %s",
					_(PROPERTY_INVALID_TYPE_STR),
					error.message);
		dbus_error_free (&error);
		return -1;
	}

	if (! access_str)
		nih_return_error (-1, PROPERTY_MISSING_ACCESS,
				  _(PROPERTY_MISSING_ACCESS_STR));

	if (! strcmp (access_str, "read")) {
		access = NIH_DBUS_READ;
	} else if (! strcmp (access_str, "write")) {
		access = NIH_DBUS_WRITE;
	} else if (! strcmp (access_str, "readwrite")) {
		access = NIH_DBUS_READWRITE;
	} else {
		nih_return_error (-1, PROPERTY_ILLEGAL_ACCESS,
				  _(PROPERTY_ILLEGAL_ACCESS_STR));
	}

	/* Allocate a Property object and push onto the stack */
	property = property_new (NULL, name, type, access);
	if (! property)
		nih_return_system_error (-1);

	if (! parse_stack_push (NULL, &context->stack,
				PARSE_PROPERTY, property)) {
		nih_error_raise_system ();
		nih_free (property);
		return -1;
	}

	return 0;
}

/**
 * property_end:
 * @xmlp: XML parser,
 * @tag: name of XML tag being parsed.
 *
 * This function is called by parse_end_tag() for a "property" end
 * tag, and matches a call to property_start_tag() made at the same
 * parsing level.
 *
 * The property is added to the list of properties defined by the parent
 * interface.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
property_end_tag (XML_Parser  xmlp,
		  const char *tag)
{
	ParseContext *context;
	ParseStack *  entry;
	ParseStack *  parent;
	Property *    property;
	Property *    conflict;
	Interface *   interface;

	nih_assert (xmlp != NULL);
	nih_assert (tag != NULL);

	context = XML_GetUserData (xmlp);
	nih_assert (context != NULL);

	entry = parse_stack_top (&context->stack);
	nih_assert (entry != NULL);
	nih_assert (entry->type == PARSE_PROPERTY);
	property = entry->property;

	/* Generate a symbol from the name */
	if (! property->symbol) {
		property->symbol = symbol_from_name (property, property->name);
		if (! property->symbol)
			nih_return_no_memory_error (-1);
	}

	nih_list_remove (&entry->entry);
	parent = parse_stack_top (&context->stack);
	nih_assert (parent != NULL);
	nih_assert (parent->type == PARSE_INTERFACE);
	interface = parent->interface;

	/* Make sure there's not a conflict before adding the property */
	conflict = interface_lookup_property (interface, property->symbol);
	if (conflict) {
		nih_error_raise_printf (PROPERTY_DUPLICATE_SYMBOL,
					_(PROPERTY_DUPLICATE_SYMBOL_STR),
					property->symbol, conflict->name);
		return -1;
	}

	nih_debug ("Add %s property to %s interface",
		   property->name, interface->name);
	nih_list_add (&interface->properties, &property->entry);
	nih_ref (property, interface);

	nih_free (entry);

	return 0;
}


/**
 * property_annotation:
 * @property: property object annotation applies to,
 * @name: annotation name,
 * @value: annotation value.
 *
 * Handles applying the annotation @name with value @value to the property
 * @property.  Properties may be annotated as deprecated or may have an
 * alternate symbol name specified.
 *
 * Unknown annotations or illegal values to the known annotations result
 * in an error being raised.
 *
 * Returns: zero on success, negative value on raised error.
 **/
int
property_annotation (Property *  property,
		     const char *name,
		     const char *value)
{
	nih_assert (property != NULL);
	nih_assert (name != NULL);
	nih_assert (value != NULL);

	if (! strcmp (name, "org.freedesktop.DBus.Deprecated")) {
		if (! strcmp (value, "true")) {
			nih_debug ("Marked %s property as deprecated",
				   property->name);
			property->deprecated = TRUE;
		} else if (! strcmp (value, "false")) {
			nih_debug ("Marked %s property as not deprecated",
				   property->name);
			property->deprecated = FALSE;
		} else {
			nih_return_error (-1, PROPERTY_ILLEGAL_DEPRECATED,
					  _(PROPERTY_ILLEGAL_DEPRECATED_STR));
		}

	} else if (! strcmp (name, "com.netsplit.Nih.Symbol")) {
		if (symbol_valid (value)) {
			if (property->symbol)
				nih_unref (property->symbol, property);

			property->symbol = nih_strdup (property, value);
			if (! property->symbol)
				nih_return_no_memory_error (-1);

			nih_debug ("Set %s property symbol to %s",
				   property->name, property->symbol);
		} else {
			nih_return_error (-1, PROPERTY_INVALID_SYMBOL,
					  _(PROPERTY_INVALID_SYMBOL_STR));
		}

	} else {
		nih_error_raise_printf (PROPERTY_UNKNOWN_ANNOTATION,
					"%s: %s: %s",
					_(PROPERTY_UNKNOWN_ANNOTATION_STR),
					property->name, name);
		return -1;
	}

	return 0;
}


/**
 * property_object_get_function:
 * @parent: parent object for new string,
 * @property: property to generate function for,
 * @name: name of function to generate,
 * @handler_name: name of handler function to call.
 *
 * Generates C code for a function called @name that will append a variant
 * containing the value of property @property to a D-Bus message iterator.
 * The value of the property is obtained by calling a function named
 * @handler_name.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_object_get_function (const void *parent,
			      Property *  property,
			      const char *name,
			      const char *handler_name)
{
	DBusSignatureIter  iter;
	NihList            inputs;
	NihList            locals;
	nih_local TypeVar *iter_var = NULL;
	nih_local char *   code_block = NULL;
	nih_local char *   oom_error_code = NULL;
	nih_local char *   block = NULL;
	nih_local char *   vars_block = NULL;
	nih_local char *   body = NULL;
	char *             code;

	nih_assert (property != NULL);
	nih_assert (name != NULL);
	nih_assert (handler_name != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&inputs);
	nih_list_init (&locals);

	/* The function requires a local iterator for the variant.  Rather
	 * than deal with it by hand, it's far easier to put it on the
	 * locals list and deal with it along with the rest.
	 */
	iter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	/* In case of out of memory, simply return and let the caller
	 * decide what to do.
	 */
	oom_error_code = nih_strdup (NULL,
				     "return -1;\n");
	if (! oom_error_code)
		return NULL;

	block = marshal (NULL, &iter, "variter", "value",
			 oom_error_code,
			 &inputs, &locals);
	if (! block)
		return NULL;

	/* Begin the handler calling block */
	if (! nih_strcat_sprintf (&code_block, NULL,
				  "/* Call the handler function */\n"
				  "if (%s (object->data, message",
				  handler_name))
		return NULL;

	/* Each of the inputs to the marshalling code becomes a local
	 * variable to our function that we pass the address of to the
	 * implementation function.
	 */
	NIH_LIST_FOREACH_SAFE (&inputs, iter) {
		TypeVar *var = (TypeVar *)iter;

		if (! nih_strcat_sprintf (&code_block, NULL,
					  ", &%s",
					  var->name))
			return NULL;

		nih_list_add (&locals, &var->entry);
	}

	/* Finish up the calling block, in case of error we again just
	 * return and let our caller deal with it.
	 */
	if (! nih_strcat_sprintf (&code_block, NULL, ") < 0)\n"
				  "\treturn -1;\n"
				  "\n"))
		return NULL;

	/* Surround the marshalling code by appending a variant onto the
	 * passed-in message iterator, and closing it once complete.
	 */
	if (! nih_strcat_sprintf (&code_block, NULL,
				  "/* Append a variant onto the message to contain the property value. */\n"
				  "if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"%s\", &variter))\n"
				  "\treturn -1;\n"
				  "\n"
				  "%s"
				  "\n"
				  "/* Finish the variant */\n"
				  "if (! dbus_message_iter_close_container (iter, &variter))\n"
				  "\treturn -1;\n",
				  property->type,
				  block))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	/* FIXME have a function to do this! */
	NIH_LIST_FOREACH (&locals, iter) {
		TypeVar *var = (TypeVar *)iter;

		if (! nih_strcat_sprintf (&vars_block, NULL, "%s %s;\n",
					  var->type,
					  var->name))
			return NULL;
	}

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "nih_assert (object != NULL);\n"
				  "nih_assert (message != NULL);\n"
				  "nih_assert (iter != NULL);\n"
				  "\n"
				  "%s"
				  "\n"
				  "return 0;\n",
				  vars_block,
				  code_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* FIXME have a function to do this */
	code = nih_sprintf (parent,
			    "int\n"
			    "%s (NihDBusObject * object, NihDBusMessage *message, DBusMessageIter *iter)\n"
			    "{\n"
			    "%s"
			    "}\n",
			    name,
			    body);
	if (! code)
		return NULL;

	return code;
}

/**
 * property_object_set_function:
 * @parent: parent object for new string,
 * @property: property to generate function for,
 * @name: name of function to generate,
 * @handler_name: name of handler function to call.
 *
 * Generates C code for a function called @name that will extract the new
 * value of a property @property from a variant at the D-Bus message iterator
 * passed.  The new value of the property is then passed to a function named
 * @handler_name to set it.  An empty reply is sent on success.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_object_set_function (const void *parent,
			      Property *  property,
			      const char *name,
			      const char *handler_name)
{
	DBusSignatureIter  iter;
	NihList            outputs;
	NihList            locals;
	nih_local TypeVar *iter_var = NULL;
	nih_local TypeVar *reply_var = NULL;
	nih_local char *   demarshal_block = NULL;
	nih_local char *   oom_error_code = NULL;
	nih_local char *   type_error_code = NULL;
	nih_local char *   block = NULL;
	nih_local char *   call_block = NULL;
	nih_local char *   vars_block = NULL;
	nih_local char *   body = NULL;
	char *             code;

	nih_assert (property != NULL);
	nih_assert (name != NULL);
	nih_assert (handler_name != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&outputs);
	nih_list_init (&locals);

 	/* The function requires a local iterator for the variant and a
	 * reply message pointer.  Rather than deal with these by hand,
	 * it's far easier to put them on the locals list and deal with
	 * them along with the rest.
	 */
	iter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	reply_var = type_var_new (NULL, "DBusMessage *", "reply");
	if (! reply_var)
		return NULL;

	nih_list_add (&locals, &reply_var->entry);

	/* Make sure that the iterator points to a variant, then open the
	 * variant.
	 */
	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "/* Recurse into the variant */\n"
				  "if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				  "\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				  "\t                                _(\"Invalid arguments to %s property\"));\n"
				  "\tif (! reply)\n"
				  "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				  "\n"
				  "\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				  "\t\tdbus_message_unref (reply);\n"
				  "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				  "\t}\n"
				  "\n"
				  "\tdbus_message_unref (reply);\n"
				  "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				  "}\n"
				  "\n"
				  "dbus_message_iter_recurse (iter, &variter);\n"
				  "\n",
				  property->name))
		return NULL;

	/* In case of out of memory, return and let D-Bus decide what to do.
	 * In case of type error we return the error to the D-Bus caller.
	 */
	oom_error_code = nih_strdup (NULL,
				     "return DBUS_HANDLER_RESULT_NEED_MEMORY;\n");
	if (! oom_error_code)
		return NULL;

	type_error_code = nih_sprintf (NULL,
				       "reply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				       "                                _(\"Invalid arguments to %s property\"));\n"
				       "if (! reply)\n"
				       "\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				       "\n"
				       "if (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				       "\tdbus_message_unref (reply);\n"
				       "\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				       "}\n"
				       "\n"
				       "dbus_message_unref (reply);\n"
				       "return DBUS_HANDLER_RESULT_HANDLED;\n",
				       property->name);
	if (! type_error_code)
		return NULL;

	block = demarshal (NULL, &iter, "message", "variter", "value",
			   oom_error_code,
			   type_error_code,
			   &outputs, &locals);
	if (! block)
		return NULL;

	/* Complete the demarshalling block, checking for any unexpected
	 * arguments which we also want to error on and begin the handler
	 * calling block.
	 */
	if (! nih_strcat_sprintf (&call_block, NULL,
				  "dbus_message_iter_next (iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				  "\treply = dbus_message_new_error (message->message, DBUS_ERROR_INVALID_ARGS,\n"
				  "\t                                _(\"Invalid arguments to %s method\"));\n"
				  "\tif (! reply)\n"
				  "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				  "\n"
				  "\tif (! dbus_connection_send (message->conn, reply, NULL)) {\n"
				  "\t\tdbus_message_unref (reply);\n"
				  "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				  "\t}\n"
				  "\n"
				  "\tdbus_message_unref (reply);\n"
				  "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				  "}\n"
				  "\n"
				  "/* Call the handler function */\n"
				  "if (%s (object->data, message",
				  property->name,
				  handler_name))
		return NULL;

	/* Each of the outputs from the demarshalling code becomes a local
	 * variable to our function that we pass to the implementation
	 * function.
	 */
	NIH_LIST_FOREACH_SAFE (&outputs, iter) {
		TypeVar *var = (TypeVar *)iter;

		if (! nih_strcat_sprintf (&call_block, NULL,
					  ", %s",
					  var->name))
			return NULL;

		nih_list_add (&locals, &var->entry);
	}

	/* Finish up the calling block, in case of out of memory error we
	 * return and let D-Bus deal with it, other errors generate an
	 * error reply.
	 */
	if (! nih_strcat_sprintf (&call_block, NULL, ") < 0) {\n"
				  "\tNihError *err;\n"
				  "\n"
				  "\terr = nih_error_get ();\n"
				  "\tif (err->number == ENOMEM) {\n"
				  "\t\tnih_free (err);\n"
				  "\n"
				  "\t\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				  "\t} else if (err->number == NIH_DBUS_ERROR) {\n"
				  "\t\tNihDBusError *dbus_err = (NihDBusError *)err;\n"
				  "\n"
				  "\t\treply = NIH_MUST (dbus_message_new_error (message->message, dbus_err->name, err->message));\n"
				  "\t\tnih_free (err);\n"
				  "\n"
				  "\t\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				  "\n"
				  "\t\tdbus_message_unref (reply);\n"
				  "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				  "\t} else {\n"
				  "\t\treply = NIH_MUST (dbus_message_new_error (message->message, DBUS_ERROR_FAILED, err->message));\n"
				  "\t\tnih_free (err);\n"
				  "\n"
				  "\t\tNIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				  "\n"
				  "\t\tdbus_message_unref (reply);\n"
				  "\t\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				  "\t}\n"
				  "}\n"))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	/* FIXME have a function to do this! */
	NIH_LIST_FOREACH (&locals, iter) {
		TypeVar *var = (TypeVar *)iter;

		if (! nih_strcat_sprintf (&vars_block, NULL, "%s %s;\n",
					  var->type,
					  var->name))
			return NULL;
	}

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "nih_assert (object != NULL);\n"
				  "nih_assert (message != NULL);\n"
				  "nih_assert (iter != NULL);\n"
				  "\n"
				  "%s"
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "/* If the sender doesn't care about a reply, don't bother wasting\n"
				  " * effort constructing and sending one.\n"
				  " */\n"
				  "if (dbus_message_get_no_reply (message->message))\n"
				  "\treturn DBUS_HANDLER_RESULT_HANDLED;\n"
				  "\n"
				  "/* Send the reply */\n"
				  "reply = NIH_MUST (dbus_message_new_method_return (message->message));\n"
				  "NIH_MUST (dbus_connection_send (message->conn, reply, NULL));\n"
				  "\n"
				  "dbus_message_unref (reply);\n"
				  "return DBUS_HANDLER_RESULT_HANDLED;\n",
				  vars_block,
				  demarshal_block,
				  block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* FIXME have a function to do this */
	code = nih_sprintf (parent,
			    "DBusHandlerResult\n"
			    "%s (NihDBusObject * object, NihDBusMessage *message, DBusMessageIter *iter)\n"
			    "{\n"
			    "%s"
			    "}\n",
			    name,
			    body);
	if (! code)
		return NULL;

	return code;
}
