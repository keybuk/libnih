/* nih-dbus-tool
 *
 * property.c - property parsing and generation
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
	ParseContext *      context;
	ParseStack *        parent;
	nih_local Property *property = NULL;
	char * const *      key;
	char * const *      value;
	const char *        name = NULL;
	const char *        type = NULL;
	const char *        access_str = NULL;
	NihDBusAccess       access;
	DBusError           error;

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
	conflict = property_lookup (interface, property->symbol);
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
 * property_lookup:
 * @interface: interface to search,
 * @symbol: property symbol to find.
 *
 * Finds a property in @interface's properties list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: property found or NULL if no property matches.
 **/
Property *
property_lookup (Interface * interface,
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


/**
 * property_object_get_function:
 * @parent: parent object for new string,
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @handlers: list to append definitions of required handlers to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that will append a variant containing
 * the value of property @property on @interface to a D-Bus message iterator,
 * the value being obtained from a handler function.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The prototype for the handler function is returned as a TypeFunc object
 * added to the @handlers list.
 *
 * The names of both the returned function and handled function prototype
 * will be generated using information in @interface and @property, prefixed
 * with @prefix.
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
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
			      const char *prefix,
			      Interface * interface,
			      Property *  property,
			      NihList *   prototypes,
			      NihList *   handlers,
			      NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             inputs;
	NihList             locals;
	NihList             property_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	nih_local TypeVar * iter_var = NULL;
	nih_local char *    code_block = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    block = NULL;
	nih_local char *    handler_name = NULL;
	nih_local TypeFunc *handler_func = NULL;
	NihListEntry *      attrib;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (handlers != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&inputs);
	nih_list_init (&locals);
	nih_list_init (&property_structs);

	/* The function returns an integer, and accepts an arguments for
	 * the D-Bus object, message and a message iterator.
	 */
	name = symbol_impl (NULL, prefix, interface->name,
			    property->name, "get");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "int", name);
	if (! func)
		return NULL;

	arg = type_var_new (func, "NihDBusObject *", "object");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "DBusMessageIter *", "iter");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

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
				     "dbus_message_iter_abandon_container (iter, &variter);\n"
				     "nih_error_raise_no_memory ();\n"
				     "return -1;\n");
	if (! oom_error_code)
		return NULL;

	block = marshal (NULL, &iter, "variter", "value",
			 oom_error_code,
			 &inputs, &locals,
			 prefix, interface->symbol,
			 property->symbol, NULL,
			 &property_structs);
	if (! block)
		return NULL;

	/* Begin the handler calling block */
	handler_name = symbol_extern (NULL, prefix, interface->symbol, "get",
				      property->symbol, NULL);
	if (! handler_name)
		return NULL;

	if (! nih_strcat_sprintf (&code_block, NULL,
				  "/* Call the handler function */\n"
				  "if (%s (object->data, message",
				  handler_name))
		return NULL;

	handler_func = type_func_new (NULL, "int", handler_name);
	if (! handler_func)
		return NULL;

	attrib = nih_list_entry_new (handler_func);
	if (! attrib)
		return NULL;

	attrib->str = nih_strdup (attrib, "warn_unused_result");
	if (! attrib->str)
		return NULL;

	nih_list_add (&handler_func->attribs, &attrib->entry);

	arg = type_var_new (handler_func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	arg = type_var_new (handler_func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

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

		/* Handler argument is pointer */
		arg = type_var_new (handler_func, var->type, var->name);
		if (! arg)
			return NULL;

		if (! type_to_pointer (&arg->type, arg))
			return NULL;

		nih_list_add (&handler_func->args, &arg->entry);
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
				  "if (! dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, \"%s\", &variter)) {\n"
				  "\tnih_error_raise_no_memory ();\n"
				  "\treturn -1;\n"
				  "}\n"
				  "\n"
				  "%s"
				  "\n"
				  "/* Finish the variant */\n"
				  "if (! dbus_message_iter_close_container (iter, &variter)) {\n"
				  "\tnih_error_raise_no_memory ();\n"
				  "\treturn -1;\n"
				  "}\n",
				  property->type,
				  block))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

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

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the functions to the prototypes and handlers lists */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	nih_list_add (handlers, &handler_func->entry);
	nih_ref (handler_func, code);

	NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}

/**
 * property_object_set_function:
 * @parent: parent object for new string,
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @handlers: list to append definitions of required handlers to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that will extract the new value of
 * property @property on @interface from a variant at the D-Bus message
 * iterator passed.  The new value of the property is then passed to
 * a handler function.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The prototype for the handler function is returned as a TypeFunc object
 * added to the @handlers list.
 *
 * The names of both the returned function and handled function prototype
 * will be generated using information in @interface and @property, prefixed
 * with @prefix.
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
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
			      const char *prefix,
			      Interface * interface,
			      Property *  property,
			      NihList *   prototypes,
			      NihList *   handlers,
			      NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             outputs;
	NihList             locals;
	NihList             property_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * reply_var = NULL;
	nih_local char *    demarshal_block = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    type_error_code = NULL;
	nih_local char *    block = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    handler_name = NULL;
	nih_local TypeFunc *handler_func = NULL;
	NihListEntry *      attrib;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (handlers != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&outputs);
	nih_list_init (&locals);
	nih_list_init (&property_structs);

	/* The function returns an integer, which means success when zero
	 * or a raised error when non-zero and accepts arguments for the
	 * D-Bus object, message and a message iterator.
	 */
	name = symbol_impl (NULL, prefix, interface->name,
			    property->name, "set");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "int", name);
	if (! func)
		return NULL;

	arg = type_var_new (func, "NihDBusObject *", "object");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "DBusMessageIter *", "iter");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	/* The function requires a local iterator for the variant.  Rather
	 * than deal with this by hand, it's far easier to put it on the
	 * locals list and deal with them along with the rest.
	 */
	iter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	/* Make sure that the iterator points to a variant, then open the
	 * variant.
	 */
	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "/* Recurse into the variant */\n"
				  "if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_VARIANT) {\n"
				  "\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				  "\t                             \"Invalid arguments to %s property\");\n"
				  "\treturn -1;\n"
				  "}\n"
				  "\n"
				  "dbus_message_iter_recurse (iter, &variter);\n"
				  "\n",
				  property->name))
		return NULL;

	/* In case of out of memory, or type error, return a raised error
	 * to the caller.
	 */
	oom_error_code = nih_strdup (NULL,
				     "nih_error_raise_no_memory ();\n"
				     "return -1;\n");
	if (! oom_error_code)
		return NULL;

	type_error_code = nih_sprintf (NULL,
				       "nih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				       "                             \"Invalid arguments to %s property\");\n"
				       "return -1;\n",
				       property->name);
	if (! type_error_code)
		return NULL;

	block = demarshal (NULL, &iter, "message", "variter", "value",
			   oom_error_code,
			   type_error_code,
			   &outputs, &locals,
			   prefix, interface->symbol,
			   property->symbol, NULL,
			   &property_structs);
	if (! block)
		return NULL;

	/* Complete the demarshalling block, checking for any unexpected
	 * arguments which we also want to error on and begin the handler
	 * calling block.
	 */
	handler_name = symbol_extern (NULL, prefix, interface->symbol, "set",
				      property->symbol, NULL);
	if (! handler_name)
		return NULL;

	if (! nih_strcat_sprintf (&call_block, NULL,
				  "dbus_message_iter_next (iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (iter) != DBUS_TYPE_INVALID) {\n"
				  "\tnih_dbus_error_raise_printf (DBUS_ERROR_INVALID_ARGS,\n"
				  "\t                             \"Invalid arguments to %s property\");\n"
				  "\treturn -1;\n"
				  "}\n"
				  "\n"
				  "/* Call the handler function */\n"
				  "if (%s (object->data, message",
				  property->name,
				  handler_name))
		return NULL;

	handler_func = type_func_new (NULL, "int", handler_name);
	if (! handler_func)
		return NULL;

	attrib = nih_list_entry_new (handler_func);
	if (! attrib)
		return NULL;

	attrib->str = nih_strdup (attrib, "warn_unused_result");
	if (! attrib->str)
		return NULL;

	nih_list_add (&handler_func->attribs, &attrib->entry);

	arg = type_var_new (handler_func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	arg = type_var_new (handler_func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

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

		/* Handler argument is const */
		arg = type_var_new (handler_func, var->type, var->name);
		if (! arg)
			return NULL;

		if (! type_to_const (&arg->type, arg))
			return NULL;

		nih_list_add (&handler_func->args, &arg->entry);
	}

	/* Finish up the calling block, in case of out of memory error we
	 * return and let D-Bus deal with it, other errors generate an
	 * error reply.
	 */
	if (! nih_strcat_sprintf (&call_block, NULL, ") < 0)\n"
				  "\treturn -1;\n"))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

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
				  "return 0;\n",
				  vars_block,
				  demarshal_block,
				  block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the functions to the prototypes and handlers lists */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	nih_list_add (handlers, &handler_func->entry);
	nih_ref (handler_func, code);

	NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}


/**
 * property_proxy_get_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that will make an asynchronous method
 * call to obtain the value of the property @property on @interface,
 * calling a notify function when the method call completes.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The names of both the returned function and notify function prototype
 * will be generated using information in @interface and @property, prefixed
 * with @prefix.
 *
 * The notify function will call a handler function passed in if the
 * reply is valid.  The name and type for this can be obtained from
 * property_proxy_get_notify_function().
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_proxy_get_function (const void *parent,
			     const char *prefix,
			     Interface * interface,
			     Property *  property,
			     NihList *   prototypes,
			     NihList *   structs)
{
	NihList             locals;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	NihListEntry *      attrib;
	nih_local char *    assert_block = NULL;
	nih_local char *    handler_type = NULL;
	nih_local TypeVar * message_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * pending_var = NULL;
	nih_local TypeVar * data_var = NULL;
	nih_local TypeVar * interface_var = NULL;
	nih_local TypeVar * property_var = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    notify_name = NULL;
	nih_local char *    block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (structs != NULL);

	nih_list_init (&locals);

	/* The function returns a pending call, and takes the proxy object
	 * as the only argument.  The pending call also indicates whether
	 * an error occurred, so we want warning if the result isn't used.
	 * We don't have a malloc attribute, since we can't guarantee that
	 * D-Bus doesn't cache them.  Since this is used by the client, we
	 * also add a deprecated attribute if the property is deprecated.
	 */
	name = symbol_extern (NULL, prefix, interface->symbol, "get",
			      property->symbol, NULL);
	if (! name)
		return NULL;

	func = type_func_new (NULL, "DBusPendingCall *", name);
	if (! func)
		return NULL;

	attrib = nih_list_entry_new (func);
	if (! attrib)
		return NULL;

	attrib->str = nih_strdup (attrib, "warn_unused_result");
	if (! attrib->str)
		return NULL;

	nih_list_add (&func->attribs, &attrib->entry);

	if (property->deprecated) {
		attrib = nih_list_entry_new (func);
		if (! attrib)
			return NULL;

		attrib->str = nih_strdup (attrib, "deprecated");
		if (! attrib->str)
			return NULL;

		nih_list_add (&func->attribs, &attrib->entry);
	}

	arg = type_var_new (func, "NihDBusProxy *", "proxy");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (proxy != NULL);\n"))
		return NULL;

	/* We also require a handler (which receives the property value),
	 * error handler (in case of error) and data arguments to pass to
	 * both as well as a timeout for the method call.  Unlike the
	 * method call case, we don't allow for no-reply calls since
	 * they're nonsensical.
	 */
	handler_type = symbol_typedef (NULL, prefix, interface->symbol,
				       "Get", property->symbol, "Reply");
	if (! handler_type)
		return NULL;

	arg = type_var_new (func, handler_type, "handler");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "NihDBusErrorHandler", "error_handler");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert ((handler != NULL) && (error_handler != NULL));\n"))
		return NULL;

	arg = type_var_new (func, "int", "timeout");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);


	/* The function requires a message pointer, which we allocate,
	 * and an iterator for it to append the arguments.  We also need
	 * a return pending call pointer and data structure as well.
	 * Rather than deal with these by hand, it's far easier to put them
	 * on the locals list and deal with them along with the rest.
	 */
	message_var = type_var_new (NULL, "DBusMessage *", "method_call");
	if (! message_var)
		return NULL;

	nih_list_add (&locals, &message_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	pending_var = type_var_new (NULL, "DBusPendingCall *", "pending_call");
	if (! pending_var)
		return NULL;

	nih_list_add (&locals, &pending_var->entry);

	data_var = type_var_new (NULL, "NihDBusPendingData *", "pending_data");
	if (! data_var)
		return NULL;

	nih_list_add (&locals, &data_var->entry);

	/* Annoyingly we also need variables for the interface and
	 * property names, since D-Bus wants their address and can't just
	 * take a constant string.
	 */
	interface_var = type_var_new (NULL, "const char *", "interface");
	if (! interface_var)
		return NULL;

	nih_list_add (&locals, &interface_var->entry);

	property_var = type_var_new (NULL, "const char *", "property");
	if (! property_var)
		return NULL;

	nih_list_add (&locals, &property_var->entry);


	/* Create the method call to get the property, the property
	 * interface gets specified as an argument - the method call
	 * interface is the D-Bus properties one.
	 */
	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Construct the method call message. */\n"
				  "method_call = dbus_message_new_method_call (proxy->name, proxy->path, \"%s\", \"Get\");\n"
				  "if (! method_call)\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "\n"
				  "dbus_message_set_auto_start (method_call, proxy->auto_start);\n"
				  "\n"
				  "dbus_message_iter_init_append (method_call, &iter);\n"
				  "\n"
				  "interface = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "property = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n",
				  DBUS_INTERFACE_PROPERTIES,
				  interface->name,
				  property->name))
		return NULL;

	/* FIXME autostart? */

	/* Complete the marshalling block by sending the message and checking
	 * for error replies.
	 */
	notify_name = symbol_impl (NULL, prefix, interface->name,
				   property->name, "get_notify");
	if (! notify_name)
		return NULL;

	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Send the message and set up the reply notification. */\n"
				  "pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,\n"
				  "                                          (NihDBusReplyHandler)handler,\n"
				  "                                          error_handler, data);\n"
				  "if (! pending_data) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "pending_call = NULL;\n"
				  "if (! dbus_connection_send_with_reply (proxy->connection, method_call,\n"
				  "                                       &pending_call, timeout)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_free (pending_data);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "dbus_message_unref (method_call);\n"
				  "\n"
				  "if (! pending_call) {\n"
				  "\tnih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,\n"
				  "\t                      \"Connection is closed\");\n"
				  "\tnih_free (pending_data);\n"
				  "\treturn NULL;\n"
				  "}\n"
				  "\n"
				  "NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)%s,\n"
				  "                                        pending_data, (DBusFreeFunction)nih_discard));\n",
				  notify_name))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "return pending_call;\n",
				  vars_block,
				  assert_block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the function to the prototypes list */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	return code;
}

/**
 * property_proxy_get_notify_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @typedefs: list to append function pointer typedef definitions to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function to handle the notification of
 * a complete pending call to obtain the value of the property @property
 * on @interface by calling either the handler function on success or
 * error function on failure.
 *
 * The notify function will call a handler function passed in if the
 * reply is valid, the typedef name for this handler must be passed as
 * @handler_type.  The actual type for this can be obtained from the
 * entry added to @typedefs.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The typedef for the handler function is returned as a TypeFunc object
 * added to the @typedefs list.
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_proxy_get_notify_function (const void *parent,
				    const char *prefix,
				    Interface * interface,
				    Property *  property,
				    NihList *   prototypes,
				    NihList *   typedefs,
				    NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             outputs;
	NihList             locals;
	NihList             property_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	nih_local char *    assert_block = NULL;
	nih_local TypeVar * reply_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * variter_var = NULL;
	nih_local TypeVar * error_var = NULL;
	nih_local TypeVar * parent_var = NULL;
	nih_local char *    steal_block = NULL;
	nih_local char *    demarshal_block = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    handler_type = NULL;
	nih_local char *    handler_name = NULL;
	nih_local TypeFunc *handler_func = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    type_error_code = NULL;
	nih_local char *    block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (typedefs != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&outputs);
	nih_list_init (&locals);
	nih_list_init (&property_structs);

	/* The function takes the pending call being notified and the
	 * associated data structure.  We don't mark the function deprecated
	 * since it's used internally, it's enough to mark the method
	 * call function deprecated.
	 */
	name = symbol_impl (NULL, prefix, interface->name,
			    property->name, "get_notify");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "void", name);
	if (! func)
		return NULL;

	arg = type_var_new (func, "DBusPendingCall *", "pending_call");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (pending_call != NULL);\n"))
		return NULL;

	arg = type_var_new (func, "NihDBusPendingData *", "pending_data");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (pending_data != NULL);\n"))
		return NULL;


	/* The function requires a message pointer, stolen from the
	 * pending call and iterators for the message and variant.  We
	 * also need a parent message context for any allocations we make,
	 * as well as an error object.
	 */
	reply_var = type_var_new (NULL, "DBusMessage *", "reply");
	if (! reply_var)
		return NULL;

	nih_list_add (&locals, &reply_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	variter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! variter_var)
		return NULL;

	nih_list_add (&locals, &variter_var->entry);

	parent_var = type_var_new (NULL, "NihDBusMessage *", "message");
	if (! parent_var)
		return NULL;

	nih_list_add (&locals, &parent_var->entry);

	error_var = type_var_new (NULL, "DBusError", "error");
	if (! error_var)
		return NULL;

	nih_list_add (&locals, &error_var->entry);

	/* Assert that the pending call is, in fact, complete then
	 * steal the message from it; handling it immediately if it's an
	 * error.
	 */
	if (! nih_strcat (&steal_block, NULL,
			  "nih_assert (dbus_pending_call_get_completed (pending_call));\n"
			  "\n"
			  "/* Steal the reply from the pending call. */\n"
			  "reply = dbus_pending_call_steal_reply (pending_call);\n"
			  "nih_assert (reply != NULL);\n"
			  "\n"
			  "/* Handle error replies */\n"
			  "if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {\n"
			  "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
			  "\n"
			  "\tdbus_error_init (&error);\n"
			  "\tdbus_set_error_from_message (&error, message->message);\n"
			  "\n"
			  "\tnih_error_push_context ();\n"
			  "\tnih_dbus_error_raise (error.name, error.message);\n"
			  "\tpending_data->error_handler (pending_data->data, message);\n"
			  "\tnih_error_pop_context ();\n"
			  "\n"
			  "\tdbus_error_free (&error);\n"
			  "\tnih_free (message);\n"
			  "\tdbus_message_unref (reply);\n"
			  "\treturn;\n"
			  "}\n"
			  "\n"
			  "nih_assert (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_METHOD_RETURN);\n"
			  "\n"))
		return NULL;

	/* To deal with out-of-memory situations, we have to loop until we've
	 * extracted all of the arguments, so this now happens in a different
	 * code block.  Create a message context and initialise the iterator,
	 * recursing into the variant.
	 */
	if (! nih_strcat (&demarshal_block, NULL,
			  "/* Create a message context for the reply, and iterate\n"
			  " * over and recurse into the arguments.\n"
			  " */\n"
			  "message = nih_dbus_message_new (pending_data, pending_data->connection, reply);\n"
			  "if (! message)\n"
			  "\tgoto enomem;\n"
			  "\n"
			  "dbus_message_iter_init (message->message, &iter);\n"
			  "\n"
			  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
			  "\tnih_error_push_context ();\n"
			  "\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
			  "\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
			  "\tpending_data->error_handler (pending_data->data, message);\n"
			  "\tnih_error_pop_context ();\n"
			  "\n"
			  "\tnih_free (message);\n"
			  "\tdbus_message_unref (reply);\n"
			  "\treturn;\n"
			  "}\n"
			  "\n"
			  "dbus_message_iter_recurse (&iter, &variter);\n"
			  "\n"))
		return NULL;

	/* Begin the handler calling block, the handler is not permitted
	 * to reply.
	 */
	handler_type = symbol_typedef (NULL, prefix, interface->symbol, "Get",
				       property->symbol, "Reply");
	if (! handler_type)
		return NULL;

	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Call the handler function */\n"
				  "nih_error_push_context ();\n"
				  "((%s)pending_data->handler) (pending_data->data, message",
				  handler_type))
		return NULL;

	handler_name = nih_sprintf (NULL, "(*%s)", handler_type);
	if (! handler_name)
		return NULL;

	handler_func = type_func_new (NULL, "typedef void", handler_name);
	if (! handler_func)
		return NULL;

	arg = type_var_new (handler_func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	arg = type_var_new (handler_func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	/* In case of out of memory, we can't just return because we've
	 * already made the method call so we loop over the code instead.
	 * But in case of type error in the returned arguments, all we
	 * can do is return an error.
	 */
	oom_error_code = nih_sprintf (NULL,
				      "nih_free (message);\n"
				      "message = NULL;\n"
				      "goto enomem;\n");
	if (! oom_error_code)
		return NULL;

	type_error_code = nih_strdup (NULL,
				      "nih_error_push_context ();\n"
				      "nih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				      "                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				      "pending_data->error_handler (pending_data->data, message);\n"
				      "nih_error_pop_context ();\n"
				      "\n"
				      "nih_free (message);\n"
				      "dbus_message_unref (reply);\n"
				      "return;\n");
	if (! type_error_code)
		return NULL;

	block = demarshal (NULL, &iter, "message", "variter", "value",
			   oom_error_code,
			   type_error_code,
			   &outputs, &locals,
			   prefix, interface->symbol,
			   property->symbol, NULL,
			   &property_structs);
	if (! block)
		return NULL;

	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "%s"
				  "\n",
				  block))
		return NULL;

	/* Each of the outputs from the demarshalling code becomes a local
	 * variable to our function that we store the value in, and passed
	 * to the handler function.
	 */
	NIH_LIST_FOREACH_SAFE (&outputs, iter) {
		TypeVar *var = (TypeVar *)iter;
		TypeVar *arg;

		if (! nih_strcat_sprintf (&call_block, NULL,
					  ", %s",
					  var->name))
			return NULL;

		nih_list_add (&locals, &var->entry);
		nih_ref (var, demarshal_block);

		/* Handler arg is const */
		arg = type_var_new (handler_func, var->type, var->name);
		if (! arg)
			return NULL;

		if (! type_to_const (&arg->type, arg))
			return NULL;

		nih_list_add (&handler_func->args, &arg->entry);
	}

	/* Complete the demarshalling block, checking for any unexpected
	 * reply arguments which we also want to error on.
	 */
	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "dbus_message_iter_next (&iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				  "\tnih_error_push_context ();\n"
				  "\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				  "\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				  "\tpending_data->error_handler (pending_data->data, message);\n"
				  "\tnih_error_pop_context ();\n"
				  "\n"
				  "\tnih_free (message);\n"
				  "\tdbus_message_unref (reply);\n"
				  "\treturn;\n"
				  "}\n"
				  "\n"))
		return NULL;

	/* Complete the call block. */
	if (! nih_strcat (&call_block, NULL, ");\n"
			  "nih_error_pop_context ();\n"
			  "\n"
			  "nih_free (message);\n"
			  "dbus_message_unref (reply);\n"))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

	if (! indent (&demarshal_block, NULL, 1))
		return NULL;

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s"
				  "do {\n"
				  "\t__label__ enomem;\n"
				  "\n"
				  "%s"
				  "enomem: __attribute__ ((unused));\n"
				  "} while (! message);\n"
				  "\n"
				  "%s",
				  vars_block,
				  assert_block,
				  steal_block,
				  demarshal_block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the functions to the prototypes and typedefs list */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	nih_list_add (typedefs, &handler_func->entry);
	nih_ref (handler_func, code);

	NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}


/**
 * property_proxy_set_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that will make an asynchronous method
 * call to set the value of the property @property on @interface,
 * calling a notify function when the method call completes.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The names of both the returned function and notify function prototype
 * will be generated using information in @interface and @property, prefixed
 * with @prefix.
 *
 * The notify function will call a handler function passed in if the
 * reply is valid.  The name and type for this can be obtained from
 * property_proxy_set_notify_function().
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_proxy_set_function (const void *parent,
			     const char *prefix,
			     Interface * interface,
			     Property *  property,
			     NihList *   prototypes,
			     NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             inputs;
	NihList             locals;
	NihList             property_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	NihListEntry *      attrib;
	nih_local char *    assert_block = NULL;
	nih_local char *    handler_type = NULL;
	nih_local TypeVar * message_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * variter_var = NULL;
	nih_local TypeVar * pending_var = NULL;
	nih_local TypeVar * data_var = NULL;
	nih_local TypeVar * interface_var = NULL;
	nih_local TypeVar * property_var = NULL;
	nih_local char *    marshal_block = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    notify_name = NULL;
	nih_local char *    block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&inputs);
	nih_list_init (&locals);
	nih_list_init (&property_structs);

	/* The function returns a pending call, and takes the proxy object
	 * as argument along with the new property value.  The pending call
	 * also indicates whether an error occurred, so we want warning if
	 * the result isn't used.  We don't have a malloc attribute, since
	 * we can't guarantee that D-Bus doesn't cache them.  Since this is
	 * used by the client, we also add a deprecated attribute if the
	 * property is deprecated.
	 */
	name = symbol_extern (NULL, prefix, interface->symbol, "set",
			      property->symbol, NULL);
	if (! name)
		return NULL;

	func = type_func_new (NULL, "DBusPendingCall *", name);
	if (! func)
		return NULL;

	attrib = nih_list_entry_new (func);
	if (! attrib)
		return NULL;

	attrib->str = nih_strdup (attrib, "warn_unused_result");
	if (! attrib->str)
		return NULL;

	nih_list_add (&func->attribs, &attrib->entry);

	if (property->deprecated) {
		attrib = nih_list_entry_new (func);
		if (! attrib)
			return NULL;

		attrib->str = nih_strdup (attrib, "deprecated");
		if (! attrib->str)
			return NULL;

		nih_list_add (&func->attribs, &attrib->entry);
	}

	arg = type_var_new (func, "NihDBusProxy *", "proxy");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (proxy != NULL);\n"))
		return NULL;

	/* The function requires a message pointer, which we allocate,
	 * and an iterator for it to append the arguments, as well as an
	 * iterator for the variant.  We also need a return pending call
	 * pointer and data structure as well.
	 */
	message_var = type_var_new (NULL, "DBusMessage *", "method_call");
	if (! message_var)
		return NULL;

	nih_list_add (&locals, &message_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	variter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! variter_var)
		return NULL;

	nih_list_add (&locals, &variter_var->entry);

	pending_var = type_var_new (NULL, "DBusPendingCall *", "pending_call");
	if (! pending_var)
		return NULL;

	nih_list_add (&locals, &pending_var->entry);

	data_var = type_var_new (NULL, "NihDBusPendingData *", "pending_data");
	if (! data_var)
		return NULL;

	nih_list_add (&locals, &data_var->entry);

	/* Annoyingly we also need variables for the interface and
	 * property names, since D-Bus wants their address and can't just
	 * take a constant string.
	 */
	interface_var = type_var_new (NULL, "const char *", "interface");
	if (! interface_var)
		return NULL;

	nih_list_add (&locals, &interface_var->entry);

	property_var = type_var_new (NULL, "const char *", "property");
	if (! property_var)
		return NULL;

	nih_list_add (&locals, &property_var->entry);


	/* Create the method call to get the property, the property
	 * interface gets specified as an argument - the method call
	 * interface is the D-Bus properties one.
	 */
	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "/* Construct the method call message. */\n"
				  "method_call = dbus_message_new_method_call (proxy->name, proxy->path, \"%s\", \"Set\");\n"
				  "if (! method_call)\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "\n"
				  "dbus_message_set_auto_start (method_call, proxy->auto_start);\n"
				  "\n"
				  "dbus_message_iter_init_append (method_call, &iter);\n"
				  "\n"
				  "interface = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "property = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"%s\", &variter)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n",
				  DBUS_INTERFACE_PROPERTIES,
				  interface->name,
				  property->name,
				  property->type))
		return NULL;

	/* FIXME autostart? */

	/* In case of out of memory, we just return the error to the caller
	 * since we haven't made the method call yet.
	 */
	oom_error_code = nih_sprintf (NULL,
				      "dbus_message_iter_abandon_container (&iter, &variter);\n"
				      "dbus_message_unref (method_call);\n"
				      "nih_return_no_memory_error (NULL);\n");
	if (! oom_error_code)
		return NULL;

	block = marshal (NULL, &iter, "variter", "value",
			 oom_error_code,
			 &inputs, &locals,
			 prefix, interface->symbol,
			 property->symbol, NULL,
			 &property_structs);
	if (! block)
		return NULL;

	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "%s"
				  "\n",
				  block))
		return NULL;

	/* Each of the inputs of the marshalling code becomes a const
	 * argument to our function that we obtain the value from.
	 */
	NIH_LIST_FOREACH_SAFE (&inputs, iter) {
		TypeVar *var = (TypeVar *)iter;

		if (! type_to_const (&var->type, var))
			return NULL;

		if (! type_strcat_assert (&assert_block, NULL, var,
					  func->args.prev != &func->args ? (TypeVar *)func->args.prev : NULL,
					  _iter.next != &inputs ? (TypeVar *)_iter.next : NULL))
			return NULL;

		nih_list_add (&func->args, &var->entry);
		nih_ref (var, func);
	}

	/* Complete the marshalling block by closing the container. */
	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "if (! dbus_message_iter_close_container (&iter, &variter)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"))
		return NULL;

	/* We also have an argument for an optional handler that notifies
	 * of a successful property set and an error handler which notifies
	 * of an error.  The error handler is optional only if the handler
	 * itself is NULL.  A data argument is passed to both, and we also
	 * have the timeout for the method call.
	 */
	handler_type = symbol_typedef (NULL, prefix, interface->symbol,
				       "Set", property->symbol, "Reply");
	if (! handler_type)
		return NULL;

	arg = type_var_new (func, handler_type, "handler");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "NihDBusErrorHandler", "error_handler");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert ((handler == NULL) || (error_handler != NULL));\n"))
		return NULL;

	arg = type_var_new (func, "int", "timeout");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	/* Send the message and check for error replies, or arguments
	 * in the reply (which is an error).
	 */
	notify_name = symbol_impl (NULL, prefix, interface->name,
				   property->name, "set_notify");
	if (! notify_name)
		return NULL;

	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Handle a fire-and-forget message */\n"
				  "if (! error_handler) {\n"
				  "\tdbus_message_set_no_reply (method_call, TRUE);\n"
				  "\tif (! dbus_connection_send (proxy->connection, method_call, NULL)) {\n"
				  "\t\tdbus_message_unref (method_call);\n"
				  "\t\tnih_return_no_memory_error (NULL);\n"
				  "\t}\n"
				  "\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\treturn (DBusPendingCall *)TRUE;\n"
				  "}\n"
				  "\n"
				  "/* Send the message and set up the reply notification. */\n"
				  "pending_data = nih_dbus_pending_data_new (NULL, proxy->connection,\n"
				  "                                          (NihDBusReplyHandler)handler,\n"
				  "                                          error_handler, data);\n"
				  "if (! pending_data) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "pending_call = NULL;\n"
				  "if (! dbus_connection_send_with_reply (proxy->connection, method_call,\n"
				  "                                       &pending_call, timeout)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_free (pending_data);\n"
				  "\tnih_return_no_memory_error (NULL);\n"
				  "}\n"
				  "\n"
				  "dbus_message_unref (method_call);\n"
				  "\n"
				  "if (! pending_call) {\n"
				  "\tnih_dbus_error_raise (DBUS_ERROR_DISCONNECTED,\n"
				  "\t                      \"Connection is closed\");\n"
				  "\tnih_free (pending_data);\n"
				  "\treturn NULL;\n"
				  "}\n"
				  "\n"
				  "NIH_MUST (dbus_pending_call_set_notify (pending_call, (DBusPendingCallNotifyFunction)%s,\n"
				  "                                        pending_data, (DBusFreeFunction)nih_discard));\n",
				  notify_name))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s"
				  "%s"
				  "\n"
				  "return pending_call;\n",
				  vars_block,
				  assert_block,
				  marshal_block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the function to the prototypes list */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}

/**
 * property_proxy_set_notify_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @typedefs: list to append function pointer typedef definitions to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function to handle the notification of
 * a complete pending call to set the value of the property @property
 * on @interface by calling either the handler function on success or
 * error function on failure.
 *
 * The notify function will call a handler function passed in if the
 * reply is valid, the typedef name for this handler must be passed as
 * @handler_type.  The actual type for this can be obtained from the
 * entry added to @typedefs.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The typedef for the handler function is returned as a TypeFunc object
 * added to the @typedefs list.
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_proxy_set_notify_function (const void *parent,
				    const char *prefix,
				    Interface * interface,
				    Property *  property,
				    NihList *   prototypes,
				    NihList *   typedefs,
				    NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             outputs;
	NihList             locals;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	nih_local char *    assert_block = NULL;
	nih_local TypeVar * reply_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * error_var = NULL;
	nih_local TypeVar * parent_var = NULL;
	nih_local char *    steal_block = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    handler_type = NULL;
	nih_local char *    handler_name = NULL;
	nih_local TypeFunc *handler_func = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    type_error_code = NULL;
	nih_local char *    block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (typedefs != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&outputs);
	nih_list_init (&locals);

	/* The function takes the pending call being notified and the
	 * associated data structure.  We don't mark the function deprecated
	 * since it's used internally, it's enough to mark the method
	 * call function deprecated.
	 */
	name = symbol_impl (NULL, prefix, interface->name,
			    property->name, "set_notify");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "void", name);
	if (! func)
		return NULL;

	arg = type_var_new (func, "DBusPendingCall *", "pending_call");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (pending_call != NULL);\n"))
		return NULL;

	arg = type_var_new (func, "NihDBusPendingData *", "pending_data");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (pending_data != NULL);\n"))
		return NULL;


	/* The function requires a message pointer, stolen from the
	 * pending call and an iterator for the message.  We also need
	 * a parent message context for any allocations we make, as well
	 * as an error object.
	 */
	reply_var = type_var_new (NULL, "DBusMessage *", "reply");
	if (! reply_var)
		return NULL;

	nih_list_add (&locals, &reply_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	parent_var = type_var_new (NULL, "NihDBusMessage *", "message");
	if (! parent_var)
		return NULL;

	nih_list_add (&locals, &parent_var->entry);

	error_var = type_var_new (NULL, "DBusError", "error");
	if (! error_var)
		return NULL;

	nih_list_add (&locals, &error_var->entry);

	/* Assert that the pending call is, in fact, complete then
	 * steal the message from it; handling it immediately if it's an
	 * error.
	 */
	if (! nih_strcat (&steal_block, NULL,
			  "nih_assert (dbus_pending_call_get_completed (pending_call));\n"
			  "\n"
			  "/* Steal the reply from the pending call. */\n"
			  "reply = dbus_pending_call_steal_reply (pending_call);\n"
			  "nih_assert (reply != NULL);\n"
			  "\n"
			  "/* Handle error replies */\n"
			  "if (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_ERROR) {\n"
			  "\tmessage = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
			  "\n"
			  "\tdbus_error_init (&error);\n"
			  "\tdbus_set_error_from_message (&error, message->message);\n"
			  "\n"
			  "\tnih_error_push_context ();\n"
			  "\tnih_dbus_error_raise (error.name, error.message);\n"
			  "\tpending_data->error_handler (pending_data->data, message);\n"
			  "\tnih_error_pop_context ();\n"
			  "\n"
			  "\tdbus_error_free (&error);\n"
			  "\tnih_free (message);\n"
			  "\tdbus_message_unref (reply);\n"
			  "\treturn;\n"
			  "}\n"
			  "\n"
			  "nih_assert (dbus_message_get_type (reply) == DBUS_MESSAGE_TYPE_METHOD_RETURN);\n"
			  "\n"))
		return NULL;

	/* Create a message context, and check that the reply had no
	 * arguments before calling the handler.
	 */
	handler_type = symbol_typedef (NULL, prefix, interface->symbol, "Set",
				       property->symbol, "Reply");
	if (! handler_type)
		return NULL;

	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Create a message context for the reply, and check\n"
				  " * there are no arguments.\n"
				  " */\n"
				  "message = NIH_MUST (nih_dbus_message_new (pending_data, pending_data->connection, reply));\n"
				  "dbus_message_iter_init (message->message, &iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				  "\tnih_error_push_context ();\n"
				  "\tnih_error_raise (NIH_DBUS_INVALID_ARGS,\n"
				  "\t                 _(NIH_DBUS_INVALID_ARGS_STR));\n"
				  "\tpending_data->error_handler (pending_data->data, message);\n"
				  "\tnih_error_pop_context ();\n"
				  "\n"
				  "\tnih_free (message);\n"
				  "\tdbus_message_unref (reply);\n"
				  "\treturn;\n"
				  "}\n"
				  "\n"
				  "/* Call the handler function */\n"
				  "if (pending_data->handler) {\n"
				  "\tnih_error_push_context ();\n"
				  "\t((%s)pending_data->handler) (pending_data->data, message);\n"
				  "\tnih_error_pop_context ();\n"
				  "}\n"
				  "\n"
				  "nih_free (message);\n"
				  "dbus_message_unref (reply);\n",
				  handler_type))
		return NULL;

	handler_name = nih_sprintf (NULL, "(*%s)", handler_type);
	if (! handler_name)
		return NULL;

	handler_func = type_func_new (NULL, "typedef void", handler_name);
	if (! handler_func)
		return NULL;

	arg = type_var_new (handler_func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	arg = type_var_new (handler_func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);


	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s"
				  "%s",
				  vars_block,
				  assert_block,
				  steal_block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the functions to the prototypes and typedefs list */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	nih_list_add (typedefs, &handler_func->entry);
	nih_ref (handler_func, code);

	return code;
}


/**
 * property_proxy_get_sync_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that will make a synchronous method
 * call to obtain the value of the property @property on @interface.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list, with the name as @name itself.
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_proxy_get_sync_function (const void *parent,
				  const char *prefix,
				  Interface * interface,
				  Property *  property,
				  NihList *   prototypes,
				  NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             outputs;
	NihList             locals;
	NihList             property_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	NihListEntry *      attrib;
	nih_local char *    assert_block = NULL;
	nih_local TypeVar * message_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * variter_var = NULL;
	nih_local TypeVar * error_var = NULL;
	nih_local TypeVar * reply_var = NULL;
	nih_local TypeVar * interface_var = NULL;
	nih_local TypeVar * property_var = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    demarshal_block = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    type_error_code = NULL;
	nih_local char *    block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&outputs);
	nih_list_init (&locals);
	nih_list_init (&property_structs);

	/* The function returns an integer, and takes a parent object and
	 * the proxy object as the argument along with an output argument
	 * for the property value.  The integer is negative if a raised
	 * error occurred, so we want warning if the result isn't used.
	 * Since this is used by the client, we also add a deprecated
	 * attribute if the property is deprecated.
	 */
	name = symbol_extern (NULL, prefix, interface->symbol, "get",
			      property->symbol, "sync");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "int", name);
	if (! func)
		return NULL;

	attrib = nih_list_entry_new (func);
	if (! attrib)
		return NULL;

	attrib->str = nih_strdup (attrib, "warn_unused_result");
	if (! attrib->str)
		return NULL;

	nih_list_add (&func->attribs, &attrib->entry);

	if (property->deprecated) {
		attrib = nih_list_entry_new (func);
		if (! attrib)
			return NULL;

		attrib->str = nih_strdup (attrib, "deprecated");
		if (! attrib->str)
			return NULL;

		nih_list_add (&func->attribs, &attrib->entry);
	}

	arg = type_var_new (func, "const void *", "parent");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "NihDBusProxy *", "proxy");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (proxy != NULL);\n"))
		return NULL;


	/* The function requires a message pointer, which we allocate,
	 * and an iterator for it to append the arguments.  We also need
	 * a reply message pointer as well and an error object.
	 * Rather than deal with these by hand, it's far easier to put them
	 * on the locals list and deal with them along with the rest.
	 */
	message_var = type_var_new (NULL, "DBusMessage *", "method_call");
	if (! message_var)
		return NULL;

	nih_list_add (&locals, &message_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	variter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! variter_var)
		return NULL;

	nih_list_add (&locals, &variter_var->entry);

	error_var = type_var_new (NULL, "DBusError", "error");
	if (! error_var)
		return NULL;

	nih_list_add (&locals, &error_var->entry);

	reply_var = type_var_new (NULL, "DBusMessage *", "reply");
	if (! reply_var)
		return NULL;

	nih_list_add (&locals, &reply_var->entry);

	/* Annoyingly we also need variables for the interface and
	 * property names, since D-Bus wants their address and can't just
	 * take a constant string.
	 */
	interface_var = type_var_new (NULL, "const char *", "interface");
	if (! interface_var)
		return NULL;

	nih_list_add (&locals, &interface_var->entry);

	property_var = type_var_new (NULL, "const char *", "property");
	if (! property_var)
		return NULL;

	nih_list_add (&locals, &property_var->entry);


	/* Create the method call to get the property, the property
	 * interface gets specified as an argument - the method call
	 * interface is the D-Bus properties one.
	 */
	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Construct the method call message. */\n"
				  "method_call = dbus_message_new_method_call (proxy->name, proxy->path, \"%s\", \"Get\");\n"
				  "if (! method_call)\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "\n"
				  "dbus_message_set_auto_start (method_call, proxy->auto_start);\n"
				  "\n"
				  "dbus_message_iter_init_append (method_call, &iter);\n"
				  "\n"
				  "interface = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "}\n"
				  "\n"
				  "property = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "}\n"
				  "\n",
				  DBUS_INTERFACE_PROPERTIES,
				  interface->name,
				  property->name))
		return NULL;

	/* FIXME autostart? */

	/* Complete the marshalling block by sending the message and checking
	 * for error replies.
	 */
	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Send the message, and wait for the reply. */\n"
				  "dbus_error_init (&error);\n"
				  "\n"
				  "reply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				  "if (! reply) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\n"
				  "\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				  "\t\tnih_error_raise_no_memory ();\n"
				  "\t} else {\n"
				  "\t\tnih_dbus_error_raise (error.name, error.message);\n"
				  "\t}\n"
				  "\n"
				  "\tdbus_error_free (&error);\n"
				  "\treturn -1;\n"
				  "}\n"
				  "\n"))
		return NULL;

	/* Begin the demarshalling block, making sure the first argument
	 * is a variant and recursing into it and also making sure that
	 * there are no subsequent arguments before we allocate the
	 * return value.
	 */
	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "dbus_message_unref (method_call);\n"
				  "\n"
				  "/* Iterate the method arguments, recursing into the variant */\n"
				  "dbus_message_iter_init (reply, &iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_VARIANT) {\n"
				  "\tdbus_message_unref (reply);\n"
				  "\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				  "\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				  "}\n"
				  "\n"
				  "dbus_message_iter_recurse (&iter, &variter);\n"
				  "\n"
				  "dbus_message_iter_next (&iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				  "\tdbus_message_unref (reply);\n"
				  "\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				  "\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				  "}\n"
				  "\n"))
		return NULL;

	/* In case of out of memory, we can't just return because we've
	 * already made the method call so we loop over the code instead.
	 * But in case of type error in the returned arguments, all we
	 * can do is return an error.
	 */
	oom_error_code = nih_sprintf (NULL,
				      "*value = NULL;\n"
				      "goto enomem;\n");
	if (! oom_error_code)
		return NULL;

	type_error_code = nih_strdup (NULL,
				      "dbus_message_unref (reply);\n"
				      "nih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				      "                  _(NIH_DBUS_INVALID_ARGS_STR));\n");
	if (! type_error_code)
		return NULL;

	block = demarshal (NULL, &iter, "parent", "variter", "local",
			   oom_error_code,
			   type_error_code,
			   &outputs, &locals,
			   prefix, interface->symbol,
			   property->symbol, NULL,
			   &property_structs);
	if (! block)
		return NULL;

	if (! nih_strcat (&block, NULL, "\n"))
		return NULL;

	/* Each of the outputs from the demarshalling code becomes a local
	 * variable to our function that we store the value in, and an
	 * argument to the function that we set when done.
	 */
	NIH_LIST_FOREACH_SAFE (&outputs, iter) {
		TypeVar *       var = (TypeVar *)iter;
		nih_local char *arg_type = NULL;
		const char *    suffix;
		nih_local char *arg_name = NULL;
		TypeVar *       arg;

		/* Output variable */
		arg_type = nih_strdup (NULL, var->type);
		if (! arg_type)
			return NULL;

		if (! type_to_pointer (&arg_type, NULL))
			return NULL;

		nih_assert (! strncmp (var->name, "local", 5));
		suffix = var->name + 5;

		arg_name = nih_sprintf (NULL, "value%s", suffix);
		if (! arg_name)
			return NULL;

		arg = type_var_new (func, arg_type, arg_name);
		if (! arg)
			return NULL;

		nih_list_add (&func->args, &arg->entry);

		if (! nih_strcat_sprintf (&assert_block, NULL,
					  "nih_assert (%s != NULL);\n",
					  arg->name))
			return NULL;

		/* Copy from local variable to output */
		if (! nih_strcat_sprintf (&block, NULL,
					  "*%s = %s;\n",
					  arg->name, var->name))
			return NULL;

		nih_list_add (&locals, &var->entry);
		nih_ref (var, demarshal_block);
	}

	/* Loop over the demarshalling code for out-of-memory situations */
	if (! indent (&block, NULL, 1))
		return NULL;

	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "do {\n"
				  "\t__label__ enomem;\n"
				  "\n"
				  "%s"
				  "enomem: __attribute__ ((unused));\n"
				  "} while (! *value);\n"
				  "\n"
				  "dbus_message_unref (reply);\n",
				  block))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s"
				  "%s"
				  "\n"
				  "return 0;\n",
				  vars_block,
				  assert_block,
				  call_block,
				  demarshal_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the function to the prototypes list */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}

/**
 * property_proxy_set_sync_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @property,
 * @property: property to generate function for,
 * @prototypes: list to append function prototypes to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that will make a synchronous method
 * call to set the value of the property @property on @interface.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list, with the name as @name itself.
 *
 * If the property type requires a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @property.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
property_proxy_set_sync_function (const void *parent,
				  const char *prefix,
				  Interface * interface,
				  Property *  property,
				  NihList *   prototypes,
				  NihList *   structs)
{
	DBusSignatureIter   iter;
	NihList             inputs;
	NihList             locals;
	NihList             property_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	NihListEntry *      attrib;
	nih_local char *    assert_block = NULL;
	nih_local TypeVar * message_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * variter_var = NULL;
	nih_local TypeVar * error_var = NULL;
	nih_local TypeVar * reply_var = NULL;
	nih_local TypeVar * interface_var = NULL;
	nih_local TypeVar * property_var = NULL;
	nih_local char *    marshal_block = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    oom_error_code = NULL;
	nih_local char *    block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (property != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_init (&iter, property->type);

	nih_list_init (&inputs);
	nih_list_init (&locals);
	nih_list_init (&property_structs);

	/* The function returns an integer, and takes the proxy object
	 * as the argument along with an input argument for the property
	 * value.  The integer is negative if a raised error occurred,
	 * so we want warning if the result isn't used.  Since this is
	 * used by the client, we also add a deprecated attribute if
	 * the property is deprecated.
	 */
	name = symbol_extern (NULL, prefix, interface->symbol, "set",
			      property->symbol, "sync");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "int", name);
	if (! func)
		return NULL;

	attrib = nih_list_entry_new (func);
	if (! attrib)
		return NULL;

	attrib->str = nih_strdup (attrib, "warn_unused_result");
	if (! attrib->str)
		return NULL;

	nih_list_add (&func->attribs, &attrib->entry);

	if (property->deprecated) {
		attrib = nih_list_entry_new (func);
		if (! attrib)
			return NULL;

		attrib->str = nih_strdup (attrib, "deprecated");
		if (! attrib->str)
			return NULL;

		nih_list_add (&func->attribs, &attrib->entry);
	}

	arg = type_var_new (func, "const void *", "parent");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	arg = type_var_new (func, "NihDBusProxy *", "proxy");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (proxy != NULL);\n"))
		return NULL;


	/* The function requires a message pointer, which we allocate,
	 * and an iterator for it to append the arguments.  We also need
	 * a reply message pointer as well and an error object.
	 * Rather than deal with these by hand, it's far easier to put them
	 * on the locals list and deal with them along with the rest.
	 */
	message_var = type_var_new (NULL, "DBusMessage *", "method_call");
	if (! message_var)
		return NULL;

	nih_list_add (&locals, &message_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	variter_var = type_var_new (NULL, "DBusMessageIter", "variter");
	if (! variter_var)
		return NULL;

	nih_list_add (&locals, &variter_var->entry);

	error_var = type_var_new (NULL, "DBusError", "error");
	if (! error_var)
		return NULL;

	nih_list_add (&locals, &error_var->entry);

	reply_var = type_var_new (NULL, "DBusMessage *", "reply");
	if (! reply_var)
		return NULL;

	nih_list_add (&locals, &reply_var->entry);

	/* Annoyingly we also need variables for the interface and
	 * property names, since D-Bus wants their address and can't just
	 * take a constant string.
	 */
	interface_var = type_var_new (NULL, "const char *", "interface");
	if (! interface_var)
		return NULL;

	nih_list_add (&locals, &interface_var->entry);

	property_var = type_var_new (NULL, "const char *", "property");
	if (! property_var)
		return NULL;

	nih_list_add (&locals, &property_var->entry);


	/* Create the method call to set the property, the property
	 * interface gets specified as an argument - the method call
	 * interface is the D-Bus properties one.  Append a variant
	 * which is where we put the new value.
	 */
	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "/* Construct the method call message. */\n"
				  "method_call = dbus_message_new_method_call (proxy->name, proxy->path, \"%s\", \"Set\");\n"
				  "if (! method_call)\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "\n"
				  "dbus_message_set_auto_start (method_call, proxy->auto_start);\n"
				  "\n"
				  "dbus_message_iter_init_append (method_call, &iter);\n"
				  "\n"
				  "interface = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &interface)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "}\n"
				  "\n"
				  "property = \"%s\";\n"
				  "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &property)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "}\n"
				  "\n"
				  "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_VARIANT, \"%s\", &variter)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "}\n"
				  "\n",
				  DBUS_INTERFACE_PROPERTIES,
				  interface->name,
				  property->name,
				  property->type))
		return NULL;

	/* FIXME autostart? */

	/* In case of out of memory, we just return the error to the caller
	 * since we haven't made the method call yet.
	 */
	oom_error_code = nih_sprintf (NULL,
				      "dbus_message_iter_abandon_container (&iter, &variter);\n"
				      "dbus_message_unref (method_call);\n"
				      "nih_return_no_memory_error (-1);\n");
	if (! oom_error_code)
		return NULL;

	block = marshal (NULL, &iter, "variter", "value",
			 oom_error_code,
			 &inputs, &locals,
			 prefix, interface->symbol,
			 property->symbol, NULL,
			 &property_structs);
	if (! block)
		return NULL;

	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "%s"
				  "\n",
				  block))
		return NULL;

	/* Each of the inputs of the marshalling code becomes a const
	 * argument to our function that we obtain the value from.
	 */
	NIH_LIST_FOREACH_SAFE (&inputs, iter) {
		TypeVar *var = (TypeVar *)iter;

		if (! type_to_const (&var->type, var))
			return NULL;

		if (! type_strcat_assert (&assert_block, NULL, var,
					  func->args.prev != &func->args ? (TypeVar *)func->args.prev : NULL,
					  _iter.next != &inputs ? (TypeVar *)_iter.next : NULL))
			return NULL;

		nih_list_add (&func->args, &var->entry);
		nih_ref (var, func);
	}

	/* Complete the marshalling block by closing the container. */
	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "if (! dbus_message_iter_close_container (&iter, &variter)) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\tnih_return_no_memory_error (-1);\n"
				  "}\n"
				  "\n"))
		return NULL;

	/* Send the message and check for error replies, or arguments
	 * in the reply (which is an error).
	 */
	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Send the message, and wait for the reply. */\n"
				  "dbus_error_init (&error);\n"
				  "\n"
				  "reply = dbus_connection_send_with_reply_and_block (proxy->connection, method_call, -1, &error);\n"
				  "if (! reply) {\n"
				  "\tdbus_message_unref (method_call);\n"
				  "\n"
				  "\tif (dbus_error_has_name (&error, DBUS_ERROR_NO_MEMORY)) {\n"
				  "\t\tnih_error_raise_no_memory ();\n"
				  "\t} else {\n"
				  "\t\tnih_dbus_error_raise (error.name, error.message);\n"
				  "\t}\n"
				  "\n"
				  "\tdbus_error_free (&error);\n"
				  "\treturn -1;\n"
				  "}\n"
				  "\n"
				  "/* Check the reply has no arguments */\n"
				  "dbus_message_unref (method_call);\n"
				  "dbus_message_iter_init (reply, &iter);\n"
				  "\n"
				  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				  "\tdbus_message_unref (reply);\n"
				  "\tnih_return_error (-1, NIH_DBUS_INVALID_ARGS,\n"
				  "\t                  _(NIH_DBUS_INVALID_ARGS_STR));\n"
				  "}\n"
				  "\n"
				  "dbus_message_unref (reply);\n"))
		return NULL;

	/* Lay out the function body, indenting it all before placing it
	 * in the function code.
	 */
	vars_block = type_var_layout (NULL, &locals);
	if (! vars_block)
		return NULL;

	if (! nih_strcat_sprintf (&body, NULL,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s"
				  "%s"
				  "\n"
				  "return 0;\n",
				  vars_block,
				  assert_block,
				  marshal_block,
				  call_block))
		return NULL;

	if (! indent (&body, NULL, 1))
		return NULL;

	/* Function header */
	code = type_func_to_string (parent, func);
	if (! code)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "{\n"
				  "%s"
				  "}\n",
				  body)) {
		nih_free (code);
		return NULL;
	}

	/* Append the function to the prototypes list */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	NIH_LIST_FOREACH_SAFE (&property_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}
