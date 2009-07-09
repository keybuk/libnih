/* nih-dbus-tool
 *
 * demarshal.c - type demarshalling
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


#include <dbus/dbus.h>

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/logging.h>

#include "indent.h"
#include "symbol.h"
#include "type.h"
#include "demarshal.h"


/* Prototypes for static functions */
static char *demarshal_basic  (const void *parent,
			       DBusSignatureIter *iter,
			       const char *parent_name,
			       const char *iter_name, const char *name,
			       const char *oom_error_code,
			       const char *type_error_code,
			       NihList *outputs, NihList *locals,
			       const char *prefix, const char *interface_symbol,
			       const char *member_symbol, const char *symbol,
			       NihList *structs)
	__attribute__ ((warn_unused_result, malloc));
static char *demarshal_array  (const void *parent,
			       DBusSignatureIter *iter,
			       const char *parent_name,
			       const char *iter_name, const char *name,
			       const char *oom_error_code,
			       const char *type_error_code,
			       NihList *outputs, NihList *locals,
			       const char *prefix, const char *interface_symbol,
			       const char *member_symbol, const char *symbol,
			       NihList *structs)
	__attribute__ ((warn_unused_result, malloc));
static char *demarshal_struct (const void *parent,
			       DBusSignatureIter *iter,
			       const char *parent_name,
			       const char *iter_name, const char *name,
			       const char *oom_error_code,
			       const char *type_error_code,
			       NihList *outputs, NihList *locals,
			       const char *prefix, const char *interface_symbol,
			       const char *member_symbol, const char *symbol,
			       NihList *structs)
	__attribute__ ((warn_unused_result, malloc));


/**
 * demarshal:
 * @parent: parent object for new string,
 * @signature: signature of type,
 * @parent_name: name of parent variable,
 * @iter_name: name of iterator variable,
 * @name: name of variable,
 * @oom_error_code: code to execute on OOM Error,
 * @type_error_code: code to exectute on type error,
 * @outputs: list to append output variables to,
 * @locals: list to append local variables to,
 * @interface_symbol: symbol of interface for structure names,
 * @member_symbol: symbol of interface member for structure names,
 * @symbol: symbol of argument or variable for structure names,
 * @structs: list to append structure definitions to.
 *
 * Generates C code to demarshal any D-Bus type from the D-Bus iterator
 * variable named @iter_name into an appropriately typed variable named
 * @name.
 *
 * The type should be the current element of the signature iterator @iter.
 * This then simply calls demarshal_fixed(), demarshal_string(),
 * demarshal_fixed_array(), demarshal_flexible_array() or demarshal_struct()
 * as appropriate.
 *
 * The generated code detects out-of-memory conditions but does not know
 * how to handle them, therefore you need to pass the appropriate handling
 * code in @oom_error_code.  This code will be inserted wherever an OOM
 * condition is detected.
 *
 * Likewise the code detects an invalid type in the iterator, but requires
 * that you pass the appropriate handling code in @type_error_code.
 *
 * The expected output variable types and names are given as TypeVar objects
 * appended to the @outputs list, each name is guaranteed to begin with @name
 * and the first member will always be @name itself.  Should the C code
 * require local variables, similar TypeVar objects will be appended to
 * the @locals list.
 *
 * If the variable requires a structure to be defined, the definition is
 * returned as a TypeStruct object appended to the @structs list.  The name
 * is generated from @prefix, @interface_symbol, @member_symbol and @symbol.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Demarshalling may require that memory is allocated, the parent object
 * is the variable named in @parent_name which may of course be "NULL".
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
char *
demarshal (const void *       parent,
	   DBusSignatureIter *iter,
	   const char *       parent_name,
	   const char *       iter_name,
	   const char *       name,
	   const char *       oom_error_code,
	   const char *       type_error_code,
	   NihList *          outputs,
	   NihList *          locals,
	   const char *       prefix,
	   const char *       interface_symbol,
	   const char *       member_symbol,
	   const char *       symbol,
	   NihList *          structs)
{
	int dbus_type;

	nih_assert (iter != NULL);
	nih_assert (parent_name != NULL);
	nih_assert (iter_name != NULL);
	nih_assert (name != NULL);
	nih_assert (oom_error_code != NULL);
	nih_assert (type_error_code != NULL);
	nih_assert (outputs != NULL);
	nih_assert (locals != NULL);
	nih_assert (prefix != NULL);
	nih_assert (member_symbol != NULL);
	nih_assert (structs != NULL);

	dbus_type = dbus_signature_iter_get_current_type (iter);

	if (dbus_type_is_basic (dbus_type)) {
		return demarshal_basic (parent, iter,
					parent_name,
					iter_name, name,
					oom_error_code, type_error_code,
					outputs, locals,
					prefix, interface_symbol,
					member_symbol, symbol,
					structs);
	} else if (dbus_type == DBUS_TYPE_ARRAY) {
		return demarshal_array (parent, iter,
					parent_name,
					iter_name, name,
					oom_error_code,
					type_error_code,
					outputs, locals,
					prefix, interface_symbol,
					member_symbol, symbol,
					structs);
	} else if ((dbus_type == DBUS_TYPE_STRUCT)
		   || (dbus_type == DBUS_TYPE_DICT_ENTRY)) {
		return demarshal_struct (parent, iter,
					 parent_name,
					 iter_name, name,
					 oom_error_code, type_error_code,
					 outputs, locals,
					 prefix, interface_symbol,
					 member_symbol, symbol,
					 structs);
	} else {
		nih_assert_not_reached ();
	}
}


/**
 * demarshal_basic:
 * @parent: parent object for new string,
 * @iter: D-Bus signature iterator,
 * @parent_name: name of parent variable,
 * @name: name of variable,
 * @oom_error_code: code to execute on OOM Error,
 * @type_error_code: code to exectute on type error,
 * @outputs: list to append output variables to,
 * @locals: list to append local variables to,
 * @interface_symbol: symbol of interface for structure names,
 * @member_symbol: symbol of interface member for structure names,
 * @symbol: symbol of argument or variable for structure names,
 * @structs: list to append structure definitions to.
 *
 * Generates C code to demarshal a D-Bus basic type (ie. numerics and
 * strings) from the D-Bus iterator variable named @iter_name into an
 * appropriately typed variable pointer named @name.
 *
 * The type should be the current element of the signature iterator @iter.
 *
 * The generated code detects out-of-memory conditions but does not know
 * how to handle them, therefore you need to pass the appropriate handling
 * code in @oom_error_code.  This code will be inserted wherever an OOM
 * condition is detected.
 *
 * Likewise the code detects an invalid type in the iterator, but requires
 * that you pass the appropriate handling code in @type_error_code.
 *
 * The expected output variable types and names are given as TypeVar objects
 * appended to the @outputs list, each name is guaranteed to begin with @name
 * and the first member will always be @name itself.  Should the C code
 * require local variables, similar TypeVar objects will be appended to
 * the @locals list.
 *
 * If the variable requires a structure to be defined, the definition is
 * returned as a TypeStruct object appended to the @structs list.  The name
 * is generated from @prefix, @interface_symbol, @member_symbol and @symbol.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Demarshalling may require that memory is allocated, the parent object
 * is the variable named in @parent_name which may of course be "NULL".
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
static char *
demarshal_basic (const void *       parent,
		 DBusSignatureIter *iter,
		 const char *       parent_name,
		 const char *       iter_name,
		 const char *       name,
		 const char *       oom_error_code,
		 const char *       type_error_code,
		 NihList *          outputs,
		 NihList *          locals,
		 const char *       prefix,
		 const char *       interface_symbol,
		 const char *       member_symbol,
		 const char *       symbol,
		 NihList *          structs)
{
	int             dbus_type;
	const char *    dbus_const;
	nih_local char *oom_error_block = NULL;
	nih_local char *type_error_block = NULL;
	nih_local char *c_type = NULL;
	char *          code = NULL;
	TypeVar *       var;

	nih_assert (iter != NULL);
	nih_assert (parent_name != NULL);
	nih_assert (iter_name != NULL);
	nih_assert (name != NULL);
	nih_assert (oom_error_code != NULL);
	nih_assert (type_error_code != NULL);
	nih_assert (outputs != NULL);
	nih_assert (locals != NULL);
	nih_assert (prefix != NULL);
	nih_assert (member_symbol != NULL);
	nih_assert (structs != NULL);

	dbus_type = dbus_signature_iter_get_current_type (iter);
	dbus_const = type_const (dbus_type);

	oom_error_block = nih_strdup (NULL, oom_error_code);
	if (! oom_error_block)
		return NULL;
	if (! indent (&oom_error_block, NULL, 1))
		return NULL;

	type_error_block = nih_strdup (NULL, type_error_code);
	if (! type_error_block)
		return NULL;
	if (! indent (&type_error_block, NULL, 1))
		return NULL;

	c_type = type_of (NULL, iter);
	if (! c_type)
		return NULL;

	if (! nih_strcat_sprintf (&code, parent,
				  "/* Demarshal a %s from the message */\n"
				  "if (dbus_message_iter_get_arg_type (&%s) != %s) {\n"
				  "%s"
				  "}\n"
				  "\n",
				  c_type,
				  iter_name, dbus_const,
				  type_error_block))
		return NULL;

	if (dbus_type_is_fixed (dbus_type)) {
		if (! nih_strcat_sprintf (&code, parent,
					  "dbus_message_iter_get_basic (&%s, &%s);\n"
					  "\n",
					  iter_name, name)) {
			nih_free (code);
			return NULL;
		}
	} else {
		nih_local char *local_name = NULL;
		nih_local char *local_type = NULL;

		/* We need a local variable to store the const value we get
		 * from D-Bus before we allocate the copy that we return.
		 */
		local_name = nih_sprintf (NULL, "%s_dbus", name);
		if (! local_name) {
			nih_free (code);
			return NULL;
		}

		local_type = nih_strdup (NULL, c_type);
		if (! local_type) {
			nih_free (code);
			return NULL;
		}

		if (! type_to_const (&local_type, NULL)) {
			nih_free (code);
			return NULL;
		}

		if (! nih_strcat_sprintf (&code, parent,
					  "dbus_message_iter_get_basic (&%s, &%s);\n"
					  "\n"
					  "%s = nih_strdup (%s, %s);\n"
					  "if (! %s) {\n"
					  "%s"
					  "}\n"
					  "\n",
					  iter_name, local_name,
					  name, parent_name, local_name,
					  name,
					  oom_error_block)) {
			nih_free (code);
			return NULL;
		}

		var = type_var_new (code, local_type, local_name);
		if (! var) {
			nih_free (code);
			return NULL;
		}

		nih_list_add (locals, &var->entry);
	}

	if (! nih_strcat_sprintf (&code, parent,
				  "dbus_message_iter_next (&%s);\n",
				  iter_name)) {
		nih_free (code);
		return NULL;
	}

	/* Append our required output variable */
	var = type_var_new (code, c_type, name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	nih_list_add (outputs, &var->entry);

	return code;
}


/**
 * demarshal_array:
 * @parent: parent object for new string,
 * @iter: D-Bus signature iterator,
 * @parent_name: name of parent variable,
 * @name: name of variable,
 * @oom_error_code: code to execute on OOM Error,
 * @type_error_code: code to exectute on type error,
 * @outputs: list to append output variables to,
 * @locals: list to append local variables to,
 * @interface_symbol: symbol of interface for structure names,
 * @member_symbol: symbol of interface member for structure names,
 * @symbol: symbol of argument or variable for structure names,
 * @structs: list to append structure definitions to.
 *
 * Generates C code to demarshal a D-Bus array type from the D-Bus
 * iterator variable named @iter_name into an appropriately typed,
 * NULL-terminated, array variable pointer named @name.    In the case
 * of arrays (of any number of levels) ultimately to a fixed type, an
 * additional input named "@name"_len is required of size_t type or an
 * appropriate number of pointers to it.
 *
 * The type should be the current element of the signature iterator @iter.
 *
 * The generated code detects out-of-memory conditions but does not know
 * how to handle them, therefore you need to pass the appropriate handling
 * code in @oom_error_code.  This code will be inserted wherever an OOM
 * condition is detected.
 *
 * Likewise the code detects an invalid type in the iterator, but requires
 * that you pass the appropriate handling code in @type_error_code.
 *
 * The expected output variable types and names are given as TypeVar objects
 * appended to the @outputs list, each name is guaranteed to begin with @name
 * and the first member will always be @name itself.  Should the C code
 * require local variables, similar TypeVar objects will be appended to
 * the @locals list.
 *
 * If the variable requires a structure to be defined, the definition is
 * returned as a TypeStruct object appended to the @structs list.  The name
 * is generated from @prefix, @interface_symbol, @member_symbol and @symbol.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Demarshalling may require that memory is allocated, the parent object
 * is the variable named in @parent_name which may of course be "NULL".
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
static char *
demarshal_array (const void *       parent,
		 DBusSignatureIter *iter,
		 const char *       parent_name,
		 const char *       iter_name,
		 const char *       name,
		 const char *       oom_error_code,
		 const char *       type_error_code,
		 NihList *          outputs,
		 NihList *          locals,
		 const char *       prefix,
		 const char *       interface_symbol,
		 const char *       member_symbol,
		 const char *       symbol,
		 NihList *          structs)
{
	nih_local char *   array_iter_name = NULL;
	nih_local char *   element_name = NULL;
	nih_local char *   element_symbol = NULL;
	nih_local char *   size_name = NULL;
	nih_local char *   oom_error_block = NULL;
	nih_local char *   child_oom_error_code = NULL;
	nih_local char *   child_oom_error_block = NULL;
	nih_local char *   type_error_block = NULL;
	nih_local char *   child_type_error_code = NULL;
	nih_local char *   child_type_error_block = NULL;
	DBusSignatureIter  subiter;
	int                element_type;
	char *             code = NULL;
	TypeVar *          array_iter_var;
	TypeVar *          size_var;
	NihList            element_outputs;
	NihList            element_locals;
	NihList            element_structs;
	nih_local char *   element_block = NULL;
	nih_local char *   alloc_block = NULL;
	nih_local char *   block = NULL;
	nih_local char *   vars_block = NULL;

	nih_assert (iter != NULL);
	nih_assert (parent_name != NULL);
	nih_assert (iter_name != NULL);
	nih_assert (name != NULL);
	nih_assert (oom_error_code != NULL);
	nih_assert (outputs != NULL);
	nih_assert (locals != NULL);
	nih_assert (prefix != NULL);
	nih_assert (member_symbol != NULL);
	nih_assert (structs != NULL);

	dbus_signature_iter_recurse (iter, &subiter);
	element_type = dbus_signature_iter_get_current_type (&subiter);

	array_iter_name = nih_sprintf (NULL, "%s_iter", name);
	if (! array_iter_name)
		return NULL;

	element_name = nih_sprintf (NULL, "%s_element", name);
	if (! element_name)
		return NULL;

	element_symbol = (symbol
			  ? nih_sprintf (NULL, "%s_element", symbol)
			  : nih_strdup (NULL, "element"));
	if (! element_symbol)
		return NULL;

	if (dbus_type_is_fixed (element_type)) {
		size_name = nih_sprintf (NULL, "%s_len", name);
		if (! size_name)
			return NULL;
	} else {
		size_name = nih_sprintf (NULL, "%s_size", name);
		if (! size_name)
			return NULL;
	}

	oom_error_block = nih_strdup (NULL, oom_error_code);
	if (! oom_error_block)
		return NULL;
	if (! indent (&oom_error_block, NULL, 1))
		return NULL;

	child_oom_error_code = nih_sprintf (NULL, ("if (%s)\n"
						   "\tnih_free (%s);\n"
						   "%s"),
					    name, name, oom_error_code);
	if (! child_oom_error_code)
		return NULL;

	child_oom_error_block = nih_strdup (NULL, child_oom_error_code);
	if (! child_oom_error_block)
		return NULL;
	if (! indent (&child_oom_error_block, NULL, 1))
		return NULL;

	type_error_block = nih_strdup (NULL, type_error_code);
	if (! type_error_block)
		return NULL;
	if (! indent (&type_error_block, NULL, 1))
		return NULL;

	child_type_error_code = nih_sprintf (NULL, ("if (%s)\n"
						    "\tnih_free (%s);\n"
						    "%s"),
					     name, name, type_error_code);
	if (! child_type_error_code)
		return NULL;

	child_type_error_block = nih_strdup (NULL, child_type_error_code);
	if (! child_type_error_block)
		return NULL;
	if (! indent (&child_type_error_block, NULL, 1))
		return NULL;

	/* Recurse into the array */
	if (! nih_strcat_sprintf (&code, parent,
				  "/* Demarshal an array from the message */\n"
				  "if (dbus_message_iter_get_arg_type (&%s) != DBUS_TYPE_ARRAY) {\n"
				  "%s"
				  "}\n"
				  "\n"
				  "dbus_message_iter_recurse (&%s, &%s);\n"
				  "\n",
				  iter_name,
				  type_error_block,
				  iter_name, array_iter_name))
		return NULL;

	array_iter_var = type_var_new (code, "DBusMessageIter",
				       array_iter_name);
	if (! array_iter_var) {
		nih_free (code);
		return NULL;
	}

	nih_list_add (locals, &array_iter_var->entry);

	/* We need a variable to keep track of the array sizes for
	 * allocation; for fixed types, this is an output but for non-fixed
	 * types it's a local.
	 */
	size_var = type_var_new (code, "size_t", size_name);
	if (! size_var) {
		nih_free (code);
		return NULL;
	}

	if (! dbus_type_is_fixed (element_type))
		nih_list_add (locals, &size_var->entry);


	if (! nih_strcat_sprintf (&code, parent,
				  "%s = 0;\n",
				  size_name)) {
		nih_free (code);
		return NULL;
	}

	/* Get the code that will demarshal the individual elements, and
	 * any output and local variables it needs.
	 */
	nih_list_init (&element_outputs);
	nih_list_init (&element_locals);
	nih_list_init (&element_structs);
	element_block = demarshal (NULL, &subiter,
				   name, array_iter_name, element_name,
				   child_oom_error_code,
				   child_type_error_code,
				   &element_outputs, &element_locals,
				   prefix, interface_symbol,
				   member_symbol, element_symbol,
				   &element_structs);
	if (! element_block) {
		nih_free (code);
		return NULL;
	}

	/* Each of the outputs of the demarshalling code equates to one
	 * of our own outputs, except that we add another level of pointers
	 * for the array; at the same time, we keep the suffix and append
	 * it to our own name.
	 *
	 * Since the outputs are all arrays, they need to be initialised
	 * or allocated before demarshalling begins.  Those of fixed types
	 * simply need to be set to NULL, those of pointer types need
	 * to be allocated with a single element containing the terminating
	 * NULL pointer.
	 *
	 * Instead of mucking around with pointers and structure members,
	 * we also append the outputs onto the local lists and fill in our
	 * variable from this.
	 */
	NIH_LIST_FOREACH_SAFE (&element_outputs, iter) {
		TypeVar *       output_var = (TypeVar *)iter;
		char *          ptr;
		nih_local char *var_type = NULL;
		const char *    suffix;
		nih_local char *var_name = NULL;
		TypeVar *       var;
		nih_local char *tmp_name = NULL;
		const char *    var_parent;

		/* Output variable */
		var_type = nih_strdup (NULL, output_var->type);
		if (! var_type) {
			nih_free (code);
			return NULL;
		}

		if (! type_to_pointer (&var_type, NULL)) {
			nih_free (code);
			return NULL;
		}

		nih_assert (! strncmp (output_var->name, element_name,
				       strlen (element_name)));
		suffix = output_var->name + strlen (element_name);

		var_name = nih_sprintf (NULL, "%s%s", name, suffix);
		if (! var_name) {
			nih_free (code);
			return NULL;
		}

		var = type_var_new (code, var_type, var_name);
		if (! var) {
			nih_free (code);
			return NULL;
		}

		nih_list_add (outputs, &var->entry);

		/* For array reallocation we need yet another variable to
		 * capture the output and make sure it didn't fail, we could
		 * just cheat and use void * but instead we use the proper
		 * type which is the same as the output variable.
		 */
		tmp_name = nih_sprintf (NULL, "%s_tmp", var_name);
		if (! tmp_name) {
			nih_free (code);
			return NULL;
		}

		var = type_var_new (element_block, var_type, tmp_name);
		if (! var) {
			nih_free (code);
			return NULL;
		}

		nih_list_add (&element_locals, &var->entry);

		/* Code to allocate and reallocate */
		var_parent = (*suffix ? name : parent_name);

		if (! nih_strcat_sprintf (&code, parent,
					  "%s = NULL;\n",
					  var_name)) {
			nih_free (code);
			return NULL;
		}

		ptr = output_var->type + strlen (output_var->type) - 1;
		if (*ptr != '*') {
			if (! nih_strcat_sprintf (&block, NULL,
						  "if (%s + 1 > SIZE_MAX / sizeof (%s)) {\n"
						  "%s"
						  "}\n"
						  "\n"
						  "%s = nih_realloc (%s, %s, sizeof (%s) * (%s + 1));\n"
						  "if (! %s) {\n"
						  "%s"
						  "}\n"
						  "\n"
						  "%s = %s;\n"
						  "%s[%s] = %s;\n"
						  "\n",
						  size_name, output_var->type,
						  child_type_error_block,
						  tmp_name, var_name, var_parent, output_var->type, size_name,
						  tmp_name,
						  child_oom_error_block,
						  var_name, tmp_name,
						  var_name, size_name, output_var->name)) {
				nih_free (code);
				return NULL;
			}
		} else {
			if (! nih_strcat_sprintf (&alloc_block, NULL,
						  "%s = nih_alloc (%s, sizeof (%s));\n"
						  "if (! %s) {\n"
						  "%s"
						  "}\n"
						  "\n"
						  "%s[%s] = NULL;\n"
						  "\n",
						  var_name, var_parent, output_var->type,
						  var_name,
						  (*suffix ? child_oom_error_block : oom_error_block),
						  var_name, size_name)) {
				nih_free (code);
				return NULL;
			}

			if (! nih_strcat_sprintf (&block, NULL,
						  "if (%s + 2 > SIZE_MAX / sizeof (%s)) {\n"
						  "%s"
						  "}\n"
						  "\n"
						  "%s = nih_realloc (%s, %s, sizeof (%s) * (%s + 2));\n"
						  "if (! %s) {\n"
						  "%s"
						  "}\n"
						  "\n"
						  "%s = %s;\n"
						  "%s[%s] = %s;\n"
						  "%s[%s + 1] = NULL;\n"
						  "\n",
						  size_name, output_var->type,
						  child_type_error_block,
						  tmp_name, var_name, var_parent, output_var->type, size_name,
						  tmp_name,
						  child_oom_error_block,
						  var_name, tmp_name,
						  var_name, size_name, output_var->name,
						  var_name, size_name)) {
				nih_free (code);
				return NULL;
			}
		}

		nih_list_add (&element_locals, &output_var->entry);
	}

	if (! nih_strcat_sprintf (&code, parent,
				  "\n"
				  "%s",
				  alloc_block ?: "")) {
		nih_free (code);
		return NULL;
	}

	if (! nih_strcat_sprintf (&block, parent,
				  "%s++;\n",
				  size_name)) {
		nih_free (code);
		return NULL;
	}

	vars_block = type_var_layout (NULL, &element_locals);
	if (! vars_block) {
		nih_free (code);
		return NULL;
	}

	NIH_LIST_FOREACH_SAFE (&element_structs, iter) {
		TypeStruct *structure = (TypeStruct *)iter;

		nih_ref (structure, code);
		nih_list_add (structs, &structure->entry);
	}

	/* Iterate over the incoming message */
	if (! nih_strcat_sprintf (&code, parent,
				  "while (dbus_message_iter_get_arg_type (&%s) != DBUS_TYPE_INVALID) {\n",
				  array_iter_name)) {
		nih_free (code);
		return NULL;
	}

	/* Lay all that out in an indented block inside the while loop.
	 * Make sure that we increase the size of the array as we go,
	 * which varies depending on whether we are using a fixed type or
	 * not.
	 */
	if (! indent (&vars_block, NULL, 1)) {
		nih_free (code);
		return NULL;
	}

	if (! indent (&element_block, NULL, 1)) {
		nih_free (code);
		return NULL;
	}

	if (! indent (&block, NULL, 1)) {
		nih_free (code);
		return NULL;
	}

	if (! nih_strcat_sprintf (&code, parent,
				  "%s"
				  "\n"
				  "%s"
				  "\n"
				  "%s",
				  vars_block,
				  element_block,
				  block)) {
		nih_free (code);
		return NULL;
	}

	/* Finish off the iteration and move onto the next message item */
	if (! nih_strcat_sprintf (&code, parent,
				  "}\n"
				  "\n"
				  "dbus_message_iter_next (&%s);\n",
				  iter_name)) {
		nih_free (code);
		return NULL;
	}

	if (dbus_type_is_fixed (element_type))
		nih_list_add (outputs, &size_var->entry);

	return code;
}


/**
 * demarshal_struct:
 * @parent: parent object for new string,
 * @iter: D-Bus signature iterator,
 * @parent_name: name of parent variable,
 * @name: name of variable,
 * @oom_error_code: code to execute on OOM Error,
 * @type_error_code: code to exectute on type error,
 * @outputs: list to append output variables to,
 * @locals: list to append local variables to,
 * @interface_symbol: symbol of interface for structure names,
 * @member_symbol: symbol of interface member for structure names,
 * @symbol: symbol of argument or variable for structure names,
 * @structs: list to append structure definitions to.
 *
 * Generates C code to demarshal a D-Bus structure type, and its members,
 * from the D-Bus iterator variable named @iter_name into an appropriately
 * typed variable pointer named @name.
 *
 * The type should be the current element of the signature iterator @iter.
 *
 * The generated code detects out-of-memory conditions but does not know
 * how to handle them, therefore you need to pass the appropriate handling
 * code in @oom_error_code.  This code will be inserted wherever an OOM
 * condition is detected.
 *
 * Likewise the code detects an invalid type in the iterator, but requires
 * that you pass the appropriate handling code in @type_error_code.
 *
 * The expected output variable types and names are given as TypeVar objects
 * appended to the @outputs list, each name is guaranteed to begin with @name
 * and the first member will always be @name itself.  Should the C code
 * require local variables, similar TypeVar objects will be appended to
 * the @locals list.
 *
 * If the variable requires a structure to be defined, the definition is
 * returned as a TypeStruct object appended to the @structs list.  The name
 * is generated from @prefix, @interface_symbol, @member_symbol and @symbol.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Demarshalling may require that memory is allocated, the parent object
 * is the variable named in @parent_name which may of course be "NULL".
 *
 * Returns: newly allocated string or NULL if insufficient memory.
 **/
static char *
demarshal_struct (const void *       parent,
		  DBusSignatureIter *iter,
		  const char *       parent_name,
		  const char *       iter_name,
		  const char *       name,
		  const char *       oom_error_code,
		  const char *       type_error_code,
		  NihList *          outputs,
		  NihList *          locals,
		  const char *       prefix,
		  const char *       interface_symbol,
		  const char *       member_symbol,
		  const char *       symbol,
		  NihList *          structs)
{
	nih_local char *  struct_iter_name = NULL;
	nih_local char *  oom_error_block = NULL;
	nih_local char *  child_oom_error_code = NULL;
	nih_local char *  child_oom_error_block = NULL;
	nih_local char *  type_error_block = NULL;
	nih_local char *  child_type_error_code = NULL;
	nih_local char *  child_type_error_block = NULL;
	int               dbus_type;
	const char *      dbus_const;
	DBusSignatureIter subiter;
	char *            code = NULL;
	TypeVar *         struct_iter_var;
	nih_local char *  c_type = NULL;
	nih_local char *  alloc_type = NULL;
	TypeStruct *      structure;
	size_t            count = 0;
	TypeVar *         var;

	nih_assert (iter != NULL);
	nih_assert (parent_name != NULL);
	nih_assert (iter_name != NULL);
	nih_assert (name != NULL);
	nih_assert (oom_error_code != NULL);
	nih_assert (type_error_code != NULL);
	nih_assert (outputs != NULL);
	nih_assert (locals != NULL);
	nih_assert (prefix != NULL);
	nih_assert (member_symbol != NULL);
	nih_assert (structs != NULL);

	struct_iter_name = nih_sprintf (NULL, "%s_iter", name);
	if (! struct_iter_name)
		return NULL;

	oom_error_block = nih_strdup (NULL, oom_error_code);
	if (! oom_error_block)
		return NULL;
	if (! indent (&oom_error_block, NULL, 1))
		return NULL;

	child_oom_error_code = nih_sprintf (NULL, ("nih_free (%s);\n"
						   "%s"),
					    name, oom_error_code);
	if (! child_oom_error_code)
		return NULL;

	child_oom_error_block = nih_strdup (NULL, child_oom_error_code);
	if (! child_oom_error_block)
		return NULL;
	if (! indent (&child_oom_error_block, NULL, 1))
		return NULL;

	type_error_block = nih_strdup (NULL, type_error_code);
	if (! type_error_block)
		return NULL;
	if (! indent (&type_error_block, NULL, 1))
		return NULL;

	child_type_error_code = nih_sprintf (NULL, ("nih_free (%s);\n"
						    "%s"),
					     name, type_error_code);
	if (! child_type_error_code)
		return NULL;

	child_type_error_block = nih_strdup (NULL, child_type_error_code);
	if (! child_type_error_block)
		return NULL;
	if (! indent (&child_type_error_block, NULL, 1))
		return NULL;

	/* Open the struct container, for that we need to know whether this
	 * is a struct or a dictionary entry even through we handled the two
	 * identically.  We'll obviously need a local variable for the
	 * recursed iterator.
	 */
	dbus_type = dbus_signature_iter_get_current_type (iter);
	dbus_const = type_const (dbus_type);

	dbus_signature_iter_recurse (iter, &subiter);

	if (! nih_strcat_sprintf (&code, parent,
				  "/* Demarshal a structure from the message */\n"
				  "if (dbus_message_iter_get_arg_type (&%s) != %s) {\n"
				  "%s"
				  "}\n"
				  "\n"
				  "dbus_message_iter_recurse (&%s, &%s);\n"
				  "\n",
				  iter_name, dbus_const,
				  type_error_block,
				  iter_name, struct_iter_name))
		return NULL;

	struct_iter_var = type_var_new (code, "DBusMessageIter",
					struct_iter_name);
	if (! struct_iter_var) {
		nih_free (code);
		return NULL;
	}

	nih_list_add (locals, &struct_iter_var->entry);

	/* FIXME there should be a way to override this to a different type
	 * name by annotation.
	 */
	alloc_type = symbol_typedef (NULL, prefix, interface_symbol, NULL,
				     member_symbol, symbol);
	if (! alloc_type) {
		nih_free (code);
		return NULL;
	}

	c_type = nih_strdup (NULL, alloc_type);
	if (! c_type) {
		nih_free (code);
		return NULL;
	}

	structure = type_struct_new (code, c_type);
	if (! structure) {
		nih_free (code);
		return NULL;
	}

	nih_list_add (structs, &structure->entry);

	if (! type_to_pointer (&c_type, NULL)) {
		nih_free (code);
		return NULL;
	}

	/* Allocate the new structure */
	if (! nih_strcat_sprintf (&code, parent,
				  "%s = nih_new (%s, %s);\n"
				  "if (! %s) {\n"
				  "%s"
				  "}\n"
				  "\n",
				  name, parent_name, alloc_type,
				  name,
				  oom_error_block)) {
		nih_free (code);
		return NULL;
	}

	/* Deal with each structure element individually, however we have
	 * to end up with just one set of locals and one block so we
	 * append directly onto our locals.
	 */
	do {
		nih_local char *item_member = NULL;
		nih_local char *item_name = NULL;
		nih_local char *item_symbol = NULL;
		NihList         item_outputs;
		NihList         item_locals;
		NihList         item_structs;
		nih_local char *item_code = NULL;

		/* FIXME there should be a way to override the item names
		 * via an annotation, which would also show up in the
		 * structure definition itself.
		 */
		item_member = nih_sprintf (NULL, "item%zu", count);
		if (! item_member) {
			nih_free (code);
			return NULL;
		}

		item_name = nih_sprintf (NULL, "%s_%s", name, item_member);
		if (! item_name) {
			nih_free (code);
			return NULL;
		}

		item_symbol = (symbol
			       ? nih_sprintf (NULL, "%s_%s", symbol, item_member)
			       : nih_strdup (NULL, item_member));
		if (! item_symbol) {
			nih_free (code);
			return NULL;
		}

		/* Get the code to do the demarshalling of this item */
		nih_list_init (&item_outputs);
		nih_list_init (&item_locals);
		nih_list_init (&item_structs);
		item_code = demarshal (NULL, &subiter,
				       name, struct_iter_name, item_name,
				       child_oom_error_code,
				       child_type_error_code,
				       &item_outputs, &item_locals,
				       prefix, interface_symbol,
				       member_symbol, item_symbol,
				       &item_structs);
		if (! item_code) {
			nih_free (code);
			return NULL;
		}

		/* Append the item locals onto our locals list, we have
		 * to reference these as we go.
		 */
		NIH_LIST_FOREACH_SAFE (&item_locals, iter) {
			TypeVar *item_local_var = (TypeVar *)iter;

			nih_list_add (locals, &item_local_var->entry);
			nih_ref (item_local_var, code);
		}

		/* Append item demarshalling code block */
		if (! nih_strcat_sprintf (&code, parent,
					  "%s"
					  "\n",
					  item_code)) {
			nih_free (code);
			return NULL;
		}

		/* Instead of mucking around with pointers and structure
		 * members, each of the marshalling code outputs is appended
		 * onto the local list and we copy from the local into
		 * our output variable.
		 */
		NIH_LIST_FOREACH_SAFE (&item_outputs, iter) {
			TypeVar *       output_var = (TypeVar *)iter;
			char *          suffix;
			nih_local char *member_name = NULL;
			TypeVar *       member_var;

			nih_assert (! strncmp (output_var->name, item_name,
					       strlen (item_name)));
			suffix = output_var->name + strlen (item_name);

			/* Create the structure member entry */
			member_name = nih_sprintf (NULL, "%s%s",
						   item_member, suffix);
			if (! member_name) {
				nih_free (code);
				return NULL;
			}

			member_var = type_var_new (structure,
						   output_var->type,
						   member_name);
			if (! member_var) {
				nih_free (code);
				return NULL;
			}

			nih_list_add (&structure->members, &member_var->entry);

			/* Add code to copy from local variable */
			if (! nih_strcat_sprintf (&code, parent,
						  "%s->%s = %s;\n",
						  name, member_name,
						  output_var->name)) {
				nih_free (code);
				return NULL;
			}

			/* Add to locals */
			nih_list_add (locals, &output_var->entry);
			nih_ref (output_var, code);
		}

		NIH_LIST_FOREACH_SAFE (&item_structs, iter) {
			TypeStruct *structure = (TypeStruct *)iter;

			nih_ref (structure, code);
			nih_list_add (structs, &structure->entry);
		}

		if (! nih_strcat (&code, parent, "\n")) {
			nih_free (code);
			return NULL;
		}

		nih_assert (++count > 0);
	} while (dbus_signature_iter_next (&subiter));

	/* Close the container again */
	if (! nih_strcat_sprintf (&code, parent,
				  "if (dbus_message_iter_get_arg_type (&%s) != DBUS_TYPE_INVALID) {\n"
				  "%s"
				  "}\n"
				  "\n"
				  "dbus_message_iter_next (&%s);\n",
				  struct_iter_name,
				  child_type_error_block,
				  iter_name)) {
		nih_free (code);
		return NULL;
	}

	/* Append our required output variable */
	var = type_var_new (code, c_type, name);
	if (! var) {
		nih_free (code);
		return NULL;
	}

	nih_list_add (outputs, &var->entry);

	return code;
}
