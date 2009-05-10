/* nih-dbus-tool
 *
 * type.c - type handling
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


#include <dbus/dbus.h>

#include <string.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/list.h>
#include <nih/string.h>
#include <nih/logging.h>

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
	default:
		nih_assert_not_reached ();
	}
}


/**
 * type_of:
 * @parent: parent object for new string,
 * @iter: D-Bus signature iterator.
 *
 * Converts the D-Bus type at the current element of the iterator @iter into
 * an appropriate C type to hold it.
 *
 * If @parent is not NULL, it should be a pointer to another object which
 * will be used as a parent for the returned string.  When all parents
 * of the returned string are entry, the returned string will also be
 * freed.
 *
 * Returns: newly allocated string or NULL if allocation failed.
 **/
char *
type_of (const void *       parent,
	 DBusSignatureIter *iter)
{
	int               dbus_type;
	DBusSignatureIter subiter;
	char *            c_type;
	char *            signature;

	nih_assert (iter != NULL);

	dbus_type = dbus_signature_iter_get_current_type (iter);

	switch (dbus_type) {
	case DBUS_TYPE_BYTE:
		return nih_strdup (parent, "uint8_t");
		break;
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
	case DBUS_TYPE_ARRAY:
		dbus_signature_iter_recurse (iter, &subiter);

		c_type = type_of (parent, &subiter);
		if (! c_type)
			return NULL;

		if (! type_to_pointer (&c_type, parent)) {
			nih_free (c_type);
			return NULL;
		}

		return c_type;
	case DBUS_TYPE_STRUCT:
	case DBUS_TYPE_DICT_ENTRY:
		signature = dbus_signature_iter_get_signature (iter);
		if (! signature)
			return NULL;

		nih_assert (signature[0] == DBUS_STRUCT_BEGIN_CHAR);
		nih_assert (signature[strlen (signature) - 1] == DBUS_STRUCT_END_CHAR);

		signature[strlen (signature) - 1] = '\0';

		c_type = nih_sprintf (parent, "struct dbus_struct_%s *",
				      signature + 1);

		dbus_free (signature);

		return c_type;
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
 * of the returned structure are entry, the returned structure will also be
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

	nih_alloc_set_destructor (var, nih_list_destroy);

	return var;
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
 * of the returned structure are entry, the returned structure will also be
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
