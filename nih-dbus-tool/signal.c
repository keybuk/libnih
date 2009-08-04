/* nih-dbus-tool
 *
 * signal.c - signal parsing and generation
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
#include "marshal.h"
#include "demarshal.h"
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
		  const char *  tag,
		  char * const *attr)
{
	ParseContext *    context;
	ParseStack *      parent;
	nih_local Signal *signal = NULL;
	char * const *    key;
	char * const *    value;
	const char *      name = NULL;

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
	ParseStack *  entry;
	ParseStack *  parent;
	Signal *      signal;
	Signal *      conflict;
	Interface *   interface;

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
	conflict = signal_lookup (interface, signal->symbol);
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
signal_annotation (Signal *    signal,
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
 * signal_lookup:
 * @interface: interface to search,
 * @symbol: signal symbol to find.
 *
 * Finds a signal in @interface's signals list which has the generated
 * or supplied C symbol @symbol.
 *
 * Returns: signal found or NULL if no signal matches.
 **/
Signal *
signal_lookup (Interface * interface,
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
signal_lookup_argument (Signal *    signal,
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


/**
 * signal_object_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @signal,
 * @signal: signal to generate function for,
 * @prototypes: list to append function prototypes to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function to emit a signal @signal on @interface by
 * marshalling the arguments.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * If any of the arguments require a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @signal.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
signal_object_function (const void *parent,
			const char *prefix,
			Interface * interface,
			Signal *    signal,
			NihList *   prototypes,
			NihList *   structs)
{
	NihList             locals;
	NihList             signal_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	NihListEntry *      attrib;
	nih_local char *    assert_block = NULL;
	nih_local TypeVar * signal_var = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local char *    marshal_block = NULL;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code = NULL;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (signal != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (structs != NULL);

	nih_list_init (&locals);
	nih_list_init (&signal_structs);

	/* The function returns an integer, and accepts an argument for
	 * the connection and origin path.  The integer indicates whether
	 * an error occurred, so we want a warning if the result isn't used.
	 */
	name = symbol_extern (NULL, prefix, interface->symbol, "emit",
			      signal->symbol, NULL);
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

	arg = type_var_new (func, "DBusConnection *", "connection");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (connection != NULL);\n"))
		return NULL;

	arg = type_var_new (func, "const char *", "origin_path");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (origin_path != NULL);\n"))
		return NULL;

	/* The function requires a message pointer, which we allocate,
	 * and an iterator for it to append the arguments.  Rather than
	 * deal with these by hand, it's far easier to put them on the
	 * locals list and deal with them along with the rest.
	 */
	signal_var = type_var_new (NULL, "DBusMessage *", "signal");
	if (! signal_var)
		return NULL;

	nih_list_add (&locals, &signal_var->entry);

	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);


	/* Create the signal and set up the iterator to append to it. */
	if (! nih_strcat_sprintf (&marshal_block, NULL,
				  "/* Construct the message. */\n"
				  "signal = dbus_message_new_signal (origin_path, \"%s\", \"%s\");\n"
				  "if (! signal)\n"
				  "\treturn -1;\n"
				  "\n"
				  "dbus_message_iter_init_append (signal, &iter);\n"
				  "\n",
				  interface->name, signal->name))
		return NULL;

	/* Iterate over the signal's output arguments, for each one we
	 * append the code to the marshalling code and at the same time
	 * build up our own expected arguments themselves.
	 */
	NIH_LIST_FOREACH (&signal->arguments, iter) {
		Argument *        argument = (Argument *)iter;
		NihList           arg_vars;
		NihList           arg_locals;
		NihList           arg_structs;
		DBusSignatureIter iter;
		nih_local char *  oom_error_code = NULL;
		nih_local char *  type_error_code = NULL;
		nih_local char *  block = NULL;

		if (argument->direction != NIH_DBUS_ARG_OUT)
			continue;

		nih_list_init (&arg_vars);
		nih_list_init (&arg_locals);
		nih_list_init (&arg_structs);

		dbus_signature_iter_init (&iter, argument->type);

		/* In case of out of memory, simply return; the caller
		 * can try again.
		 */
		oom_error_code = nih_strdup (NULL,
					     "dbus_message_unref (signal);\n"
					     "return -1;\n");
		if (! oom_error_code)
			return NULL;

		block = marshal (NULL, &iter, "iter", argument->symbol,
				 oom_error_code,
				 &arg_vars, &arg_locals,
				 prefix, interface->symbol,
				 signal->symbol, argument->symbol,
				 &arg_structs);
		if (! block)
			return NULL;

		if (! nih_strcat_sprintf (&marshal_block, NULL,
					  "%s"
					  "\n",
					  block))
			return NULL;

		/* We take a parameter of the expected type and name of
		 * the marshal input variable; if it's a pointer, we
		 * assert that it's not NULL and make sure it's const.
		 */
		NIH_LIST_FOREACH_SAFE (&arg_vars, iter) {
			TypeVar *var = (TypeVar *)iter;

			if (! type_to_const (&var->type, var))
				return NULL;

			if (! type_strcat_assert (&assert_block, NULL, var,
						  func->args.prev != &func->args ? (TypeVar *)func->args.prev : NULL,
						  _iter.next != &arg_vars ? (TypeVar *)_iter.next : NULL))
				return NULL;

			nih_list_add (&func->args, &var->entry);
			nih_ref (var, func);
		}

		NIH_LIST_FOREACH_SAFE (&arg_locals, iter) {
			TypeVar *var = (TypeVar *)iter;

			nih_list_add (&locals, &var->entry);
			nih_ref (var, marshal_block);
		}

		NIH_LIST_FOREACH_SAFE (&arg_structs, iter) {
			TypeStruct *structure = (TypeStruct *)iter;

			nih_list_add (&signal_structs, &structure->entry);
			nih_ref (structure, marshal_block);
		}
	}

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
				  "/* Send the signal, appending it to the outgoing queue. */\n"
				  "if (! dbus_connection_send (connection, signal, NULL)) {\n"
				  "\tdbus_message_unref (signal);\n"
				  "\treturn -1;\n"
				  "}\n"
				  "\n"
				  "dbus_message_unref (signal);\n"
				  "\n"
				  "return 0;\n",
				  vars_block,
				  assert_block,
				  marshal_block))
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

	NIH_LIST_FOREACH_SAFE (&signal_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}


/**
 * signal_proxy_function:
 * @parent: parent object for new string.
 * @prefix: prefix for function name,
 * @interface: interface of @signal,
 * @signal: signal to generate function for,
 * @prototypes: list to append function prototypes to,
 * @typedefs: list to append function pointer typedef definitions to,
 * @structs: list to append structure definitions to.
 *
 * Generates C code for a function that acts as a D-Bus connection filter
 * function checking that the incoming message matches @signal on @itnerface
 * and calls a handler function after demarshalling the arguments.
 *
 * The prototype of the returned function is returned as a TypeFunc object
 * appended to the @prototypes list.
 *
 * The typedef of the handler function is returned as a TypeFunc object
 * appended to the @typedefs list.
 *
 * If any of the arguments require a structure to be defined, the
 * definition is returned as a TypeStruct object appended to the @structs
 * list.  The name is generated from @prefix, @interface and @signal.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the return string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
signal_proxy_function  (const void *parent,
			const char *prefix,
			Interface * interface,
			Signal *    signal,
			NihList *   prototypes,
			NihList *   typedefs,
			NihList *   structs)
{
	NihList             locals;
	NihList             signal_structs;
	nih_local char *    name = NULL;
	nih_local TypeFunc *func = NULL;
	TypeVar *           arg;
	nih_local char *    assert_block = NULL;
	nih_local TypeVar * iter_var = NULL;
	nih_local TypeVar * parent_var = NULL;
	nih_local char *    demarshal_block = NULL;
	nih_local char *    call_block = NULL;
	nih_local char *    handler_type = NULL;
	nih_local char *    handler_name = NULL;
	nih_local TypeFunc *handler_func = NULL;
	NihListEntry *      attrib;
	nih_local char *    vars_block = NULL;
	nih_local char *    body = NULL;
	char *              code;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (signal != NULL);
	nih_assert (prototypes != NULL);
	nih_assert (typedefs != NULL);
	nih_assert (structs != NULL);

	nih_list_init (&locals);
	nih_list_init (&signal_structs);

	/* The function returns a D-Bus handler result, accepting arguments
	 * for the connection, received message and proxied signal structure.
	 */
	name = symbol_impl (NULL, prefix, interface->name,
			    signal->name, "signal");
	if (! name)
		return NULL;

	func = type_func_new (NULL, "DBusHandlerResult", name);
	if (! func)
		return NULL;

	arg = type_var_new (func, "DBusConnection *", "connection");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (connection != NULL);\n"))
		return NULL;

	arg = type_var_new (func, "DBusMessage *", "signal");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (signal != NULL);\n"))
		return NULL;

	arg = type_var_new (func, "NihDBusProxySignal *", "proxied");
	if (! arg)
		return NULL;

	nih_list_add (&func->args, &arg->entry);

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (proxied != NULL);\n"))
		return NULL;

	if (! nih_strcat (&assert_block, NULL,
			  "nih_assert (connection == proxied->proxy->connection);\n"))
		return NULL;

	/* The function requires a message context to act as the parent of
	 * arguments and is passed to the handler function; we also need
	 * an iterator for it.
	 */
	iter_var = type_var_new (NULL, "DBusMessageIter", "iter");
	if (! iter_var)
		return NULL;

	nih_list_add (&locals, &iter_var->entry);

	parent_var = type_var_new (NULL, "NihDBusMessage *", "message");
	if (! parent_var)
		return NULL;

	nih_list_add (&locals, &parent_var->entry);

	/* Begin the demarshalling block by checking that the message is
	 * the signal we're looking for, and then if it is, allocating a
	 * message context and iterating the arguments.
	 */
	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "if (! dbus_message_is_signal (signal, proxied->interface->name, proxied->signal->name))\n"
				  "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				  "\n"
				  "if (! dbus_message_has_path (signal, proxied->proxy->path))\n"
				  "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				  "\n"
				  "if (proxied->proxy->name)\n"
				  "\tif (! dbus_message_has_sender (signal, proxied->proxy->owner))\n"
				  "\t\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				  "\n"
				  "message = nih_dbus_message_new (NULL, connection, signal);\n"
				  "if (! message)\n"
				  "\treturn DBUS_HANDLER_RESULT_NEED_MEMORY;\n"
				  "\n"
				  "/* Iterate the arguments to the signal and demarshal into arguments\n"
				  " * for our own function call.\n"
				  " */\n"
				  "dbus_message_iter_init (message->message, &iter);\n"
				  "\n"))
		return NULL;

	/* Begin the handler calling block, which is not permitted to
	 * reply.  Build up the typedef for it, which we mostly use for
	 * type passing reasons.
	 */
	handler_type = symbol_typedef (NULL, prefix, interface->symbol, NULL,
				       signal->symbol, "Handler");
	if (! handler_type)
		return NULL;

	if (! nih_strcat_sprintf (&call_block, NULL,
				  "/* Call the handler function */\n"
				  "nih_error_push_context ();\n"
				  "((%s)proxied->handler) (proxied->data, message",
				  handler_type))
		return NULL;

	handler_name = nih_sprintf (NULL, "(*%s)", handler_type);
	if (! handler_name)
		return NULL;

	handler_func = type_func_new (NULL, "typedef void", handler_name);
	if (! handler_func)
		return NULL;

	if (signal->deprecated) {
		attrib = nih_list_entry_new (handler_func);
		if (! attrib)
			return NULL;

		attrib->str = nih_strdup (attrib, "deprecated");
		if (! attrib->str)
			return NULL;

		nih_list_add (&handler_func->attribs, &attrib->entry);
	}


	arg = type_var_new (handler_func, "void *", "data");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	arg = type_var_new (handler_func, "NihDBusMessage *", "message");
	if (! arg)
		return NULL;

	nih_list_add (&handler_func->args, &arg->entry);

	/* Iterate over the signal arguments, for each output argument
	 * we append the code to the the demarshalling code.  At the
	 * same time, we build up the handler call itself and transfer
	 * the actual arguments to the locals list.
	 */
	NIH_LIST_FOREACH (&signal->arguments, iter) {
		Argument *        argument = (Argument *)iter;
		NihList           arg_vars;
		NihList           arg_locals;
		NihList           arg_structs;
		DBusSignatureIter iter;
		nih_local char *  local_name = NULL;
		nih_local char *  oom_error_code = NULL;
		nih_local char *  type_error_code = NULL;
		nih_local char *  block = NULL;

		if (argument->direction != NIH_DBUS_ARG_OUT)
			continue;

		nih_list_init (&arg_vars);
		nih_list_init (&arg_locals);
		nih_list_init (&arg_structs);

		dbus_signature_iter_init (&iter, argument->type);

		/* In case of out of memory, we just return and D-Bus will
		 * call us again.  In case of type error, just ignore the
		 * signal entirely.
		 */
		oom_error_code = nih_sprintf (NULL,
					      "nih_free (message);\n"
					      "return DBUS_HANDLER_RESULT_NEED_MEMORY;\n");
		if (! oom_error_code)
			return NULL;

		type_error_code = nih_strdup (NULL,
					      "nih_free (message);\n"
					      "return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n");
		if (! type_error_code)
			return NULL;

		block = demarshal (NULL, &iter, "message", "iter",
				   argument->symbol,
				   oom_error_code,
				   type_error_code,
				   &arg_vars, &arg_locals,
				   prefix, interface->symbol,
				   signal->symbol, argument->symbol,
				   &arg_structs);
		if (! block)
			return NULL;

		if (! nih_strcat (&block, NULL, "\n"))
			return NULL;

		NIH_LIST_FOREACH_SAFE (&arg_vars, iter) {
			TypeVar *var = (TypeVar *)iter;
			TypeVar *arg;

			if (! nih_strcat_sprintf (&call_block, NULL,
						  ", %s",
						  var->name))
				return NULL;

			nih_list_add (&locals, &var->entry);
			nih_ref (var, call_block);

			/* Handler argument is const */
			arg = type_var_new (handler_func,
					    var->type, var->name);
			if (! arg)
				return NULL;

			if (! type_to_const (&arg->type, arg))
				return NULL;

			nih_list_add (&handler_func->args,
				      &arg->entry);
		}

		NIH_LIST_FOREACH_SAFE (&arg_locals, iter) {
			TypeVar *var = (TypeVar *)iter;

			nih_list_add (&locals, &var->entry);
			nih_ref (var, demarshal_block);
		}

		NIH_LIST_FOREACH_SAFE (&arg_structs, iter) {
			TypeStruct *structure = (TypeStruct *)iter;

			nih_list_add (&signal_structs, &structure->entry);
			nih_ref (structure, demarshal_block);
		}

		if (! nih_strcat (&demarshal_block, NULL, block))
			return NULL;
	}

	/* Complete the demarshalling block, checking for any unexpected
	 * arguments which we also want to error on.
	 */
	if (! nih_strcat_sprintf (&demarshal_block, NULL,
				  "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INVALID) {\n"
				  "\tnih_free (message);\n"
				  "\treturn DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n"
				  "}\n"
				  "\n"))
	    return NULL;

	/* Complete the call block */
	if (! nih_strcat (&call_block, NULL, ");\n"
			  "nih_error_pop_context ();\n"
			  "nih_free (message);\n"
			  "\n"))
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
				  "return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;\n",
				  vars_block,
				  assert_block,
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

	/* Append the functions to the prototypes and typedefs lists */
	nih_list_add (prototypes, &func->entry);
	nih_ref (func, code);

	nih_list_add (typedefs, &handler_func->entry);
	nih_ref (handler_func, code);

	NIH_LIST_FOREACH_SAFE (&signal_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	return code;
}


/**
 * signal_args_array:
 * @parent: parent object for new string,
 * @prefix: prefix for array name,
 * @interface: interface of @signal,
 * @signal: signal to generate array for,
 * @prototypes: list to append prototype to.
 *
 * Generates C code to declare an array of NihDBusArg variables containing
 * information about the arguments of the signal @signal on @interface.
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
signal_args_array (const void *parent,
		   const char *prefix,
		   Interface * interface,
		   Signal *    signal,
		   NihList *   prototypes)
{
	nih_local char *name = NULL;
	size_t          max_name = 0;
	size_t          max_type = 0;
	nih_local char *block = NULL;
	char *          code;
	TypeVar *       var;

	nih_assert (prefix != NULL);
	nih_assert (interface != NULL);
	nih_assert (signal != NULL);
	nih_assert (prototypes != NULL);

	name = symbol_impl (NULL, prefix, interface->name,
			    signal->name, "signal_args");
	if (! name)
		return NULL;

	/* Figure out the longest argument name and signature */
	NIH_LIST_FOREACH (&signal->arguments, iter) {
		Argument *argument = (Argument *)iter;

		if (argument->name) {
			if (strlen (argument->name) > max_name)
				max_name = strlen (argument->name);
		} else {
			if (max_name < 4)
				max_name = 4;
		}

		if (strlen (argument->type) > max_type)
			max_type = strlen (argument->type);
	}

	/* Append each argument such that the names, types and directions
	 * are all lined up with each other.
	 */
	NIH_LIST_FOREACH (&signal->arguments, iter) {
		Argument *      argument = (Argument *)iter;
		nih_local char *line = NULL;
		char *          dest;

		line = nih_alloc (NULL, max_name + max_type + 31);
		if (! line)
			return NULL;

		dest = line;

		memcpy (dest, "{ ", 2);
		dest += 2;

		if (argument->name) {
			memcpy (dest, "\"", 1);
			dest += 1;

			memcpy (dest, argument->name, strlen (argument->name));
			dest += strlen (argument->name);

			memcpy (dest, "\", ", 3);
			dest += 3;

			memset (dest, ' ', max_name - strlen (argument->name));
			dest += max_name - strlen (argument->name);
		} else {
			memcpy (dest, "NULL, ", 6);
			dest += 6;

			memset (dest, ' ', max_name - 2);
			dest += max_name - 2;
		}

		memcpy (dest, "\"", 1);
		dest += 1;

		memcpy (dest, argument->type, strlen (argument->type));
		dest += strlen (argument->type);

		memcpy (dest, "\", ", 3);
		dest += 3;

		memset (dest, ' ', max_type - strlen (argument->type));
		dest += max_type - strlen (argument->type);

		memcpy (dest, "NIH_DBUS_ARG_OUT", 16);
		dest += 16;

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
			    "const NihDBusArg %s[] = {\n"
			    "%s"
			    "};\n",
			    name,
			    block);
	if (! code)
		return NULL;

	/* Append the prototype to the list */
	var = type_var_new (code, "const NihDBusArg", name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	var->array = TRUE;

	nih_list_add (prototypes, &var->entry);

	return code;
}
