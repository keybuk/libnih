/* nih-dbus-tool
 *
 * type.c - type handling
 *
 * Copyright © 2010 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2010 Canonical Ltd.
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

#include "symbol.h"
#include "indent.h"
#include "type.h"


/**
 * type_const:
 * @dbus_type: D-Bus type constant.
 *
 * Converts an integer D-Bus type constant into the string name of the
 * constant, used when generating code.
 *
 * Returns: constant string.
 **/
const char *
type_const (int dbus_type)
{
	switch (dbus_type) {
	case DBUS_TYPE_BYTE:
		return "DBUS_TYPE_BYTE";
	case DBUS_TYPE_BOOLEAN:
		return "DBUS_TYPE_BOOLEAN";
	case DBUS_TYPE_INT16:
		return "DBUS_TYPE_INT16";
	case DBUS_TYPE_UINT16:
		return "DBUS_TYPE_UINT16";
	case DBUS_TYPE_INT32:
		return "DBUS_TYPE_INT32";
	case DBUS_TYPE_UINT32:
		return "DBUS_TYPE_UINT32";
	case DBUS_TYPE_INT64:
		return "DBUS_TYPE_INT64";
	case DBUS_TYPE_UINT64:
		return "DBUS_TYPE_UINT64";
	case DBUS_TYPE_DOUBLE:
		return "DBUS_TYPE_DOUBLE";
	case DBUS_TYPE_STRING:
		return "DBUS_TYPE_STRING";
	case DBUS_TYPE_OBJECT_PATH:
		return "DBUS_TYPE_OBJECT_PATH";
	case DBUS_TYPE_SIGNATURE:
		return "DBUS_TYPE_SIGNATURE";
	case DBUS_TYPE_ARRAY:
		return "DBUS_TYPE_ARRAY";
	case DBUS_TYPE_STRUCT:
		return "DBUS_TYPE_STRUCT";
	case DBUS_TYPE_DICT_ENTRY:
		return "DBUS_TYPE_DICT_ENTRY";
	case DBUS_TYPE_UNIX_FD:
		return "DBUS_TYPE_UNIX_FD";
	default:
		nih_assert_not_reached ();
	}
}


/**
 * type_of:
 * @parent: parent object for new string,
 * @iter: D-Bus signature iterator.
 *
 * Converts the D-Bus basic type at the current element of the iterator
 * @iter into an appropriate C type to hold it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation failed.
 **/
char *
type_of (const void *       parent,
	 DBusSignatureIter *iter)
{
	int dbus_type;

	nih_assert (iter != NULL);

	dbus_type = dbus_signature_iter_get_current_type (iter);

	switch (dbus_type) {
	case DBUS_TYPE_BYTE:
		return nih_strdup (parent, "uint8_t");
	case DBUS_TYPE_BOOLEAN:
		return nih_strdup (parent, "int");
	case DBUS_TYPE_INT16:
		return nih_strdup (parent, "int16_t");
	case DBUS_TYPE_UINT16:
		return nih_strdup (parent, "uint16_t");
	case DBUS_TYPE_INT32:
		return nih_strdup (parent, "int32_t");
	case DBUS_TYPE_UINT32:
		return nih_strdup (parent, "uint32_t");
	case DBUS_TYPE_INT64:
		return nih_strdup (parent, "int64_t");
	case DBUS_TYPE_UINT64:
		return nih_strdup (parent, "uint64_t");
	case DBUS_TYPE_DOUBLE:
		return nih_strdup (parent, "double");
	case DBUS_TYPE_STRING:
		return nih_strdup (parent, "char *");
	case DBUS_TYPE_OBJECT_PATH:
		return nih_strdup (parent, "char *");
	case DBUS_TYPE_SIGNATURE:
		return nih_strdup (parent, "char *");
	case DBUS_TYPE_UNIX_FD:
		return nih_strdup (parent, "int");
	default:
		nih_assert_not_reached ();
	}
}


/**
 * type_var_new:
 * @parent: parent object for new structure,
 * @type: C type,
 * @name: variable name.
 *
 * Allocates and returns a new TypeVar structure with the C type @type
 * and variable name @name, the strucure is not placed into any linked
 * list but will be removed from its containing list when freed.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned structure.  When all parents
 * of the returned structure are freed, the returned structure will also be
 * freed.
 *
 * Returns: the new TypeVar structure or NULL if insufficient memory.
 **/
TypeVar *
type_var_new (const void *parent,
	      const char *type,
	      const char *name)
{
	TypeVar *var;

	nih_assert (type != NULL);
	nih_assert (name != NULL);

	var = nih_new (parent, TypeVar);
	if (! var)
		return NULL;

	nih_list_init (&var->entry);

	var->type = nih_strdup (var, type);
	if (! var->type) {
		nih_free (var);
		return NULL;
	}

	var->name = nih_strdup (var, name);
	if (! var->name) {
		nih_free (var);
		return NULL;
	}

	var->array = FALSE;

	nih_alloc_set_destructor (var, nih_list_destroy);

	return var;
}

/**
 * type_var_to_string:
 * @parent: parent object for new string,
 * @var: variable to convert.
 *
 * Returns a string for the given variable @var, consisting of the type
 * and variable name separated by a space if appropriate.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated string or NULL if insufficient memory.
 **/
char *
type_var_to_string (const void *parent,
		    TypeVar *   var)
{
	char *str;

	nih_assert (var != NULL);

	if (strchr (var->type, '*')) {
		str = nih_sprintf (parent, "%s%s", var->type, var->name);
	} else {
		str = nih_sprintf (parent, "%s %s", var->type, var->name);
	}

	if (! str)
		return NULL;

	if (var->array) {
		if (! nih_strcat (&str, parent, "[]")) {
			nih_free (str);
			return NULL;
		}
	}

	return str;
}

/**
 * type_var_layout:
 * @parent: parent object for new string,
 * @vars: list of variables to convert.
 *
 * Returns a string for the list of variables @vars, each of which should
 * be a TypeVar structure.  Each variable is declared on a new line,
 * with the names lined up to the longest type length.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated string or NULL if insufficient memory.
 **/
char *
type_var_layout (const void *parent,
		 NihList *   vars)
{
	size_t max;
	size_t len;
	char * str;

	nih_assert (vars != NULL);

	/* Work out how much space to have for the types */
	max = 0;
	NIH_LIST_FOREACH (vars, iter) {
		TypeVar *var = (TypeVar *)iter;
		size_t   this_len;

		this_len = strlen (var->type);
		if (! strchr (var->type, '*'))
			this_len++;

		if (this_len > max)
			max = this_len;
	}

	/* Allocate a string with each of the variables on each line. */
	len = 0;
	str = nih_strdup (parent, "");
	if (! str)
		return NULL;

	NIH_LIST_FOREACH (vars, iter) {
		TypeVar *var = (TypeVar *)iter;
		char *   new_str;

		new_str = nih_realloc (str, parent,
				       (len + max + strlen (var->name)
					+ (var->array ? 2 : 0) + 3));
		if (! new_str) {
			nih_free (str);
			return NULL;
		}

		str = new_str;

		memset (str + len, ' ', max);
		memcpy (str + len, var->type, strlen (var->type));
		len += max;

		memcpy (str + len, var->name, strlen (var->name));
		len += strlen (var->name);

		if (var->array) {
			memcpy (str + len, "[]", 2);
			len += 2;
		}

		memcpy (str + len, ";\n", 2);
		len += 2;

		str[len] = '\0';
	}

	return str;
}


/**
 * type_func_new:
 * @parent: parent object for new structure,
 * @type: C return type,
 * @name: function name.
 *
 * Allocates and returns a new TypeFunc structure with the
 * C return type @type and function name @name, the structure is not placed
 * into any linked list but will be removed from its containing list when
 * freed.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned structure.  When all parents
 * of the returned structure are freed, the returned structure will also be
 * freed.
 *
 * Returns: the new TypeFunc structure or NULL if insufficient memory.
 **/
TypeFunc *
type_func_new (const void *parent,
	       const char *type,
	       const char *name)
{
	TypeFunc *func;

	nih_assert (type != NULL);
	nih_assert (name != NULL);

	func = nih_new (parent, TypeFunc);
	if (! func)
		return NULL;

	nih_list_init (&func->entry);

	func->type = nih_strdup (func, type);
	if (! func->type) {
		nih_free (func);
		return NULL;
	}

	func->name = nih_strdup (func, name);
	if (! func->name) {
		nih_free (func);
		return NULL;
	}

	nih_list_init (&func->args);
	nih_list_init (&func->attribs);

	nih_alloc_set_destructor (func, nih_list_destroy);

	return func;
}

/**
 * type_func_to_string:
 * @parent: parent object for new string,
 * @func: function to convert.
 *
 * Returns a string for the given function @func for use as the function
 * declaration header.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated string or NULL if insufficient memory.
 **/
char *
type_func_to_string (const void *parent,
		     TypeFunc *  func)
{
	char * str;
	size_t max;
	size_t len;

	nih_assert (func != NULL);

	/* Type goes on a separate line to the name */
	str = nih_sprintf (parent, "%s\n%s (", func->type, func->name);
	if (! str)
		return NULL;

	/* If no arguments, should be just void */
	if (NIH_LIST_EMPTY (&func->args)) {
		if (! nih_strcat (&str, parent, "void)\n")) {
			nih_free (str);
			return NULL;
		}

		return str;
	}

	/* Figure out the longest type name */
	max = 0;
	NIH_LIST_FOREACH (&func->args, iter) {
		TypeVar *arg = (TypeVar *)iter;
		size_t   this_len;

		this_len = strlen (arg->type);
		if (! strchr (arg->type, '*'))
			this_len++;

		if (this_len > max)
			max = this_len;
	}

	/* Append each argument with all the types and names lined up */
	len = strlen (str);
	NIH_LIST_FOREACH (&func->args, iter) {
		TypeVar *arg = (TypeVar *)iter;
		char *   new_str;

		/* Each additional argument goes onto a new line, indented
		 * by the length of the function name plus the usual
		 * punctuation
		 */
		if (iter != func->args.next) {
			new_str = nih_realloc (str, parent,
					       len + strlen (func->name) + 5);
			if (! new_str) {
				nih_free (str);
				return NULL;
			}

			str = new_str;

			memcpy (str + len, ",\n", 2);
			len += 2;

			memset (str + len, ' ', strlen (func->name) + 2);
			len += strlen (func->name) + 2;

			str[len] = '\0';
		}

		/* Append the argument so that the names line up */
		new_str = nih_realloc (str, parent,
				       len + max + strlen (arg->name) + 1);
		if (! new_str) {
			nih_free (str);
			return NULL;
		}

		str = new_str;

		memset (str + len, ' ', max);
		memcpy (str + len, arg->type, strlen (arg->type));
		len += max;

		memcpy (str + len, arg->name, strlen (arg->name));
		len += strlen (arg->name);

		str[len] = '\0';
	}

	if (! nih_strcat (&str, parent, ")\n")) {
		nih_free (str);
		return NULL;
	}

	return str;
}

/**
 * type_func_to_typedef:
 * @parent: parent object for new string,
 * @func: function to convert.
 *
 * Returns a string for the given function typedef @func for use as the
 * typedef declaration.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated string or NULL if insufficient memory.
 **/
char *
type_func_to_typedef (const void *parent,
		      TypeFunc *  func)
{
	char *str;

	nih_assert (func != NULL);

	str = nih_sprintf (parent, "%s %s (", func->type, func->name);
	if (! str)
		return NULL;

	/* If no arguments, should be just void */
	if (NIH_LIST_EMPTY (&func->args)) {
		if (! nih_strcat (&str, parent, "void")) {
			nih_free (str);
			return NULL;
		}
	}

	/* Append the arguments */
	NIH_LIST_FOREACH (&func->args, iter) {
		TypeVar *       arg = (TypeVar *)iter;
		nih_local char *arg_str = NULL;

		if (iter != func->args.next) {
			if (! nih_strcat (&str, parent, ", ")) {
				nih_free (str);
				return NULL;
			}
		}

		arg_str = type_var_to_string (NULL, arg);
		if (! arg_str) {
			nih_free (str);
			return NULL;
		}

		if (! nih_strcat (&str, parent, arg_str)) {
			nih_free (str);
			return NULL;
		}
	}

	if (! nih_strcat (&str, parent, ");\n")) {
		nih_free (str);
		return NULL;
	}

	return str;
}

/**
 * type_func_layout:
 * @parent: parent object for new string,
 * @funcs: list of functions to convert.
 *
 * Returns a string for the list of functions @funcs, each of which should
 * be a TypeFunc structure.  Each function is declared on a new line,
 * with the names lined up to the longest type length and the arguments
 * list lined up to the longest name length.  Attributes follow on the next
 * line.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated string or NULL if insufficient memory.
 **/
char *
type_func_layout (const void *parent,
		  NihList *   funcs)
{
	size_t type_max;
	size_t name_max;
	char * str;

	nih_assert (funcs != NULL);

	/* Work out how much space to have for the types and the names */
	type_max = 0;
	name_max = 0;
	NIH_LIST_FOREACH (funcs, iter) {
		TypeFunc *func = (TypeFunc *)iter;
		size_t    this_type_len;
		size_t    this_name_len;

		this_type_len = strlen (func->type);
		if (! strchr (func->type, '*'))
			this_type_len++;

		this_name_len = strlen (func->name);

		if (this_type_len > type_max)
			type_max = this_type_len;
		if (this_name_len > name_max)
			name_max = this_name_len;
	}

	/* Allocate a string with each of the functions on each line. */
	str = nih_strdup (parent, "");
	if (! str)
		return NULL;

	NIH_LIST_FOREACH (funcs, iter) {
		TypeFunc *func = (TypeFunc *)iter;
		char *    new_str;
		size_t    len;

		len = strlen (str);

		new_str = nih_realloc (str, parent,
				       len + type_max + name_max + 3);
		if (! new_str) {
			nih_free (str);
			return NULL;
		}

		str = new_str;

		memset (str + len, ' ', type_max);
		memcpy (str + len, func->type, strlen (func->type));
		len += type_max;

		memset (str + len, ' ', name_max + 1);
		memcpy (str + len, func->name, strlen (func->name));
		len += name_max + 1;

		str[len] = '(';
		len++;

		str[len] = '\0';

		/* If no arguments, should be just void */
		if (NIH_LIST_EMPTY (&func->args)) {
			if (! nih_strcat (&str, parent, "void")) {
				nih_free (str);
				return NULL;
			}
		}

		/* Append the arguments */
		NIH_LIST_FOREACH (&func->args, iter) {
			TypeVar *       arg = (TypeVar *)iter;
			nih_local char *arg_str = NULL;

			if (iter != func->args.next) {
				if (! nih_strcat (&str, parent, ", ")) {
					nih_free (str);
					return NULL;
				}
			}

			arg_str = type_var_to_string (NULL, arg);
			if (! arg_str) {
				nih_free (str);
				return NULL;
			}

			if (! nih_strcat (&str, parent, arg_str)) {
				nih_free (str);
				return NULL;
			}
		}

		/* If there are no attributes, that's it */
		if (NIH_LIST_EMPTY (&func->attribs)) {
			if (! nih_strcat (&str, parent, ");\n")) {
				nih_free (str);
				return NULL;
			}

			continue;
		}

		/* Append the attributes indented on the next line */
		if (! nih_strcat (&str, parent, ")\n\t__attribute__ ((")) {
			nih_free (str);
			return NULL;
		}

		NIH_LIST_FOREACH (&func->attribs, iter) {
			NihListEntry *attrib = (NihListEntry *)iter;

			if (iter != func->attribs.next) {
				if (! nih_strcat (&str, parent, ", ")) {
					nih_free (str);
					return NULL;
				}
			}

			if (! nih_strcat (&str, parent, attrib->str)) {
				nih_free (str);
				return NULL;
			}
		}

		if (! nih_strcat (&str, parent, "));\n")) {
			nih_free (str);
			return NULL;
		}
	}

	return str;
}


/**
 * type_struct_new:
 * @parent: parent object for new structure,
 * @name: structure name.
 *
 * Allocates and returns a new TypeStruct structure with the structure
 * name @name, the structure is not placed into any linked list but will
 * be removed from its containing list when freed.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned structure.  When all parents
 * of the returned structure are freed, the returned structure will also be
 * freed.
 *
 * Returns: the new TypeStruct structure or NULL if insufficient memory.
 **/
TypeStruct *
type_struct_new (const void *parent,
		 const char *name)
{
	TypeStruct *structure;

	nih_assert (name != NULL);

	structure = nih_new (parent, TypeStruct);
	if (! structure)
		return NULL;

	nih_list_init (&structure->entry);

	structure->name = nih_strdup (structure, name);
	if (! structure->name) {
		nih_free (structure);
		return NULL;
	}

	nih_list_init (&structure->members);

	nih_alloc_set_destructor (structure, nih_list_destroy);

	return structure;
}

/**
 * type_struct_to_string:
 * @parent: parent object for new string,
 * @structure: structure to convert.
 *
 * Returns a string for the given @structure declaring it both as a C
 * structure and as a shorter typedef.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are freed, the returned string will also be
 * freed.
 *
 * Returns: the newly allocated string or NULL if insufficient memory.
 **/
char *
type_struct_to_string (const void *parent,
		       TypeStruct *structure)
{
	nih_local char *symbol = NULL;
	nih_local char *block = NULL;
	char *          str;

	nih_assert (structure != NULL);

	/* Generate a lowercase structure symbol */
	symbol = symbol_from_name (NULL, structure->name);
	if (! symbol)
		return NULL;

	/* Line up the members and indent */
	block = type_var_layout (NULL, &structure->members);
	if (! block)
		return NULL;

	if (! NIH_LIST_EMPTY (&structure->members))
		if (! indent (&block, NULL, 1))
			return NULL;

	/* Output */
	str = nih_sprintf (parent,
			   "typedef struct %s {\n"
			   "%s"
			   "} %s;\n",
			   symbol,
			   block,
			   structure->name);

	return str;
}


/**
 * type_to_const:
 * @type: C type,
 * @parent: parent object for original string.
 *
 * Converts the C type @type into a constant form if it's a pointer type.
 *
 * Only the first level of pointer is made constant, this is consistent
 * with C99 only allowing one level of type-compatibility in this regard.
 * i.e.:
 *   char * becomes const char *
 *   char ** becomes char * const *
 *
 * This has no effect if the first level of pointer is already constant,
 * if other levels are constant, the pointer will become more constant.
 *
 * @type is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @type for style reasons.
 *
 * Returns: modified @type or NULL if insufficient memory.
 **/
char *
type_to_const (char **     type,
	       const void *parent)
{
	char * ptr;
	char * ret;
	size_t len;

	nih_assert (type != NULL);
	nih_assert (*type != NULL);

	len = strlen (*type);
	nih_assert (len > 0);

	/* If this is a pointer, the final character will be the operator;
	 * if not, we can return now.
	 */
	ptr = *type + len - 1;
	if (*ptr != '*')
		return *type;

	/* Check whether this is the sole pointer operator, if not we need
	 * to insert "const" before this one - otherwise we simply prepend
	 * the declaration with "const".  In both cases, check it's not
	 * already there.  (This still allows us to const-up a pointer).
	 */
	if (strchr (*type, '*') != ptr) {
		if ((len > 8)
		    && (! strncmp (*type + len - 8, " const *", 8)))
			return *type;

		ret = nih_realloc (*type, parent, len + 8);
		if (! ret)
			return NULL;

		*type = ret;

		memcpy (*type + len - 1, " const *", 9);
	} else if (strncmp (*type, "const ", 6)) {
		ret = nih_realloc (*type, parent, len + 7);
		if (! ret)
			return NULL;

		*type = ret;
		memmove (*type + 6, *type, len + 1);
		memcpy (*type, "const ", 6);
	}

	return *type;
}

/**
 * type_to_pointer:
 * @type: C type,
 * @parent: parent object for original string.
 *
 * Covnerts the C type @type into a type containing a pointer to the
 * original type.  If the type in @type is already a pointer, a further
 * level of indirection is added.
 *
 * This has a special behaviour in the case of constant pointers, the
 * constantness is moved from the previous top level to the new top
 * level.
 * i.e.:
 *   const char * becomes char * const *
 *   char * const * becomes char ** const *
 *
 * This is to allow for arrays to pointerify their elements whilst
 * preserving the "I don't modify this" flag use of const.  We can't
 * just add another const becuse C99 only allows one level of
 * type-compatibility.
 *
 * @type is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @type for style reasons.
 *
 * Returns: modified @type or NULL if insufficient memory.
 **/
char *
type_to_pointer (char **     type,
		 const void *parent)
{
	char *  ptr;
	char *  ret;
	size_t len;

	nih_assert (type != NULL);
	nih_assert (*type != NULL);

	len = strlen (*type);
	nih_assert (len > 0);

	ptr = *type + len - 1;

	/* If the string is a single-level constant pointer, we have to
	 * shuffle it all around to make it a two-level pointer with the
	 * new first-level constant.
	 *
	 * If the string is a nth-level constant pointer, we just insert
	 * a pointer in before the const part.
	 *
	 * If the string is any kind of pointer, we append another pointer.
	 *
	 * Otherwise we add a pointer and a space to keep everything
	 * separate.
	 */
	if ((strchr (*type, '*') == ptr)
	    && (! strncmp (*type, "const ", 6))) {
		ret = nih_realloc (*type, parent, len + 3);
		if (! ret)
			return NULL;

		*type = ret;

		memmove (*type, *type + 6, len - 6);
		memcpy (*type + len - 6, " const *", 9);

	} else if ((len > 8)
		   && (! strncmp (*type + len - 8, " const *", 8))) {
		ret = nih_realloc (*type, parent, len + 2);
		if (! ret)
			return NULL;

		*type = ret;

		memcpy (*type + len - 8, "* const *", 10);
	} else if (*ptr == '*') {
		ret = nih_realloc (*type, parent, len + 2);
		if (! ret)
			return NULL;

		*type = ret;

		memcpy (*type + len, "*", 2);
	} else {
		ret = nih_realloc (*type, parent, len + 3);
		if (! ret)
			return NULL;

		*type = ret;

		memcpy (*type + len, " *", 3);
	}

	return *type;
}

/**
 * type_to_static:
 * @type: C type,
 * @parent: parent object for original string.
 *
 * Converts the C type @type into a static form.
 *
 * This has no effect if the type is already static.
 *
 * @type is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @type for style reasons.
 *
 * Returns: modified @type or NULL if insufficient memory.
 **/
char *
type_to_static (char **     type,
		const void *parent)
{
	char * ret;
	size_t len;

	nih_assert (type != NULL);
	nih_assert (*type != NULL);

	len = strlen (*type);
	nih_assert (len > 0);

	if (strncmp (*type, "static ", 7)) {
		ret = nih_realloc (*type, parent, len + 8);
		if (! ret)
			return NULL;

		*type = ret;

		memmove (*type + 7, *type, len + 1);
		memcpy (*type, "static ", 7);
	}

	return *type;
}

/**
 * type_to_extern:
 * @type: C type,
 * @parent: parent object for original string.
 *
 * Converts the C type @type into an extern form.
 *
 * This has no effect if the type is already extern.
 *
 * @type is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @type for style reasons.
 *
 * Returns: modified @type or NULL if insufficient memory.
 **/
char *
type_to_extern (char **     type,
		const void *parent)
{
	char * ret;
	size_t len;

	nih_assert (type != NULL);
	nih_assert (*type != NULL);

	len = strlen (*type);
	nih_assert (len > 0);

	if (strncmp (*type, "extern ", 7)) {
		ret = nih_realloc (*type, parent, len + 8);
		if (! ret)
			return NULL;

		*type = ret;

		memmove (*type + 7, *type, len + 1);
		memcpy (*type, "extern ", 7);
	}

	return *type;
}


/**
 * type_strcat_assert:
 * @block: code block to append to,
 * @parent: parent object for @block,
 * @var: variable to append assert block for,
 * @prev: prev variable in list or NULL,
 * @next: next variable in list or NULL.
 *
 * If @var is a pointer variable, appends a line of code to @block that
 * asserts that the variable is not NULL.  If @var is not a pointer variable,
 * this function has no effect.
 *
 * This function handles the case of @var being an array and @next
 * being its size member, in which case @var may be NULL if its size member
 * is zero.
 *
 * This function also handles the case of @var being an array of size
 * members and @prev being the array, in which case @var may be NULL if the
 * first member of @prev is NULL.
 *
 * @block is modified directly, the returned string is simply a pointer to
 * it, thus @parent is actually ignored though it usual to pass the parent
 * of @block for style reasons.
 *
 * Note that @block may be returned unmodified, therefore it is not
 * permitted to pass NULL for @block as this would be indistinguishable from
 * insufficient memory.
 *
 * Returns: modified @block or NULL if insufficient memory.
 **/
char *
type_strcat_assert (char **     block,
		    const void *parent,
		    TypeVar *   var,
		    TypeVar *   prev,
		    TypeVar *   next)
{
	nih_assert (block != NULL);
	nih_assert (var != NULL);

	if (! strchr (var->type, '*'))
		return *block;

	if (next && (! strcmp (next->type, "size_t"))) {
		if (! nih_strcat_sprintf (block, parent,
					  "nih_assert ((%s == 0) || (%s != NULL));\n",
					  next->name, var->name))
			return NULL;

	} else if (prev && strstr (var->type, "size_t")) {
		if (! nih_strcat_sprintf (block, parent,
					  "nih_assert ((*%s == NULL) || (%s != NULL));\n",
					  prev->name, var->name))
			return NULL;

	} else {
		if (! nih_strcat_sprintf (block, parent,
					  "nih_assert (%s != NULL);\n",
					  var->name))
			return NULL;
	}

	return *block;
}

