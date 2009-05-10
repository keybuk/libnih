/* nih-dbus-tool
 *
 * test_test.c - test suite for nih-dbus-tool/type.c
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

#include <nih/test.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>

#include "type.h"


void
test_const (void)
{
	TEST_FUNCTION ("type_const");

	/* Check that the correct value is returned for a Byte. */
	TEST_FEATURE ("with byte");
	TEST_EQ_STR (type_const (DBUS_TYPE_BYTE), "DBUS_TYPE_BYTE");

	/* Check that the correct value is returned for a Boolean. */
	TEST_FEATURE ("with boolean");
	TEST_EQ_STR (type_const (DBUS_TYPE_BOOLEAN), "DBUS_TYPE_BOOLEAN");

	/* Check that the correct value is returned for an Int16. */
	TEST_FEATURE ("with int16");
	TEST_EQ_STR (type_const (DBUS_TYPE_INT16), "DBUS_TYPE_INT16");

	/* Check that the correct value is returned for a UInt16. */
	TEST_FEATURE ("with uint16");
	TEST_EQ_STR (type_const (DBUS_TYPE_UINT16), "DBUS_TYPE_UINT16");

	/* Check that the correct value is returned for an Int32. */
	TEST_FEATURE ("with int32");
	TEST_EQ_STR (type_const (DBUS_TYPE_INT32), "DBUS_TYPE_INT32");

	/* Check that the correct value is returned for a UInt32. */
	TEST_FEATURE ("with uint32");
	TEST_EQ_STR (type_const (DBUS_TYPE_UINT32), "DBUS_TYPE_UINT32");

	/* Check that the correct value is returned for an Int64. */
	TEST_FEATURE ("with int64");
	TEST_EQ_STR (type_const (DBUS_TYPE_INT64), "DBUS_TYPE_INT64");

	/* Check that the correct value is returned for a UInt64. */
	TEST_FEATURE ("with uint64");
	TEST_EQ_STR (type_const (DBUS_TYPE_UINT64), "DBUS_TYPE_UINT64");

	/* Check that the correct value is returned for a Double. */
	TEST_FEATURE ("with double");
	TEST_EQ_STR (type_const (DBUS_TYPE_DOUBLE), "DBUS_TYPE_DOUBLE");

	/* Check that the correct value is returned for a String. */
	TEST_FEATURE ("with stirng");
	TEST_EQ_STR (type_const (DBUS_TYPE_STRING), "DBUS_TYPE_STRING");

	/* Check that the correct value is returned for an Object Path. */
	TEST_FEATURE ("with object path");
	TEST_EQ_STR (type_const (DBUS_TYPE_OBJECT_PATH),
		     "DBUS_TYPE_OBJECT_PATH");

	/* Check that the correct value is returned for a Signature. */
	TEST_FEATURE ("with signature");
	TEST_EQ_STR (type_const (DBUS_TYPE_SIGNATURE), "DBUS_TYPE_SIGNATURE");

	/* Check that the correct value is returned for an Array. */
	TEST_FEATURE ("with array");
	TEST_EQ_STR (type_const (DBUS_TYPE_ARRAY), "DBUS_TYPE_ARRAY");

	/* Check that the correct value is returned for a Struct. */
	TEST_FEATURE ("with struct");
	TEST_EQ_STR (type_const (DBUS_TYPE_STRUCT), "DBUS_TYPE_STRUCT");

	/* Check that the correct value is returned for a Dict Entry. */
	TEST_FEATURE ("with dict entry");
	TEST_EQ_STR (type_const (DBUS_TYPE_DICT_ENTRY),
		     "DBUS_TYPE_DICT_ENTRY");
}


void
test_of (void)
{
	DBusSignatureIter iter;
	char *            str;

	TEST_FUNCTION ("type_of");


	/* Check that the expected C type is returned for a D-Bus Byte
	 * type.
	 */
	TEST_FEATURE ("with byte");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_BYTE_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint8_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Boolean
	 * type.
	 */
	TEST_FEATURE ("with boolean");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_BOOLEAN_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int16
	 * type.
	 */
	TEST_FEATURE ("with int16");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_INT16_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int16_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus UInt16
	 * type.
	 */
	TEST_FEATURE ("with uint16");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_UINT16_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint16_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int32
	 * type.
	 */
	TEST_FEATURE ("with int32");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_INT32_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int32_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus UInt32
	 * type.
	 */
	TEST_FEATURE ("with uint32");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_UINT32_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint32_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int64
	 * type.
	 */
	TEST_FEATURE ("with int64");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_INT64_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int64_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus UInt64
	 * type.
	 */
	TEST_FEATURE ("with uint64");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_UINT64_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "uint64_t");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Double
	 * type.
	 */
	TEST_FEATURE ("with double");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_DOUBLE_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "double");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus String
	 * type.
	 */
	TEST_FEATURE ("with string");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_STRING_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Object Path
	 * type.
	 */
	TEST_FEATURE ("with object path");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_OBJECT_PATH_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Signature
	 * type.
	 */
	TEST_FEATURE ("with signature");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, DBUS_TYPE_SIGNATURE_AS_STRING);

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int16 Array
	 * type.
	 */
	TEST_FEATURE ("with int16 array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_INT16_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int16_t *");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Int16
	 * Array Array type.
	 */
	TEST_FEATURE ("with int16 array array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_INT16_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "int16_t **");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus String
	 * Array type.
	 */
	TEST_FEATURE ("with string array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char **");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus String
	 * Array Array type.
	 */
	TEST_FEATURE ("with string array array");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "char ***");

		nih_free (str);
	}


	/* Check that the expected C type is returned for a D-Bus Structure,
	 * which is a struct definition including the type signature.
	 */
	TEST_FEATURE ("with structure");
	TEST_ALLOC_FAIL {
		dbus_signature_iter_init (&iter, (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING
						  DBUS_TYPE_UINT32_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_STRING_AS_STRING
						  DBUS_TYPE_ARRAY_AS_STRING
						  DBUS_TYPE_INT16_AS_STRING
						  DBUS_STRUCT_END_CHAR_AS_STRING));

		str = type_of (NULL, &iter);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_ALLOC_ORPHAN (str);
		TEST_EQ_STR (str, "struct dbus_struct_suasan *");

		nih_free (str);
	}
}


void
test_var_new (void)
{
	TypeVar *var;

	TEST_FUNCTION ("type_var_new");


	/* Check to make sure that a TypeVar structure is allocated
	 * correctly and returned.
	 */
	TEST_ALLOC_FAIL {
		var = type_var_new (NULL, "char *", "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (var, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_LIST_EMPTY (&var->entry);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "foo");
		TEST_ALLOC_PARENT (var->name, var);

		nih_free (var);
	}
}

void
test_var_to_string (void)
{
	TypeVar *var = NULL;
	char *   str;

	TEST_FUNCTION ("type_var_to_string");


	/* Check to make sure that a non-pointer variable is returned with
	 * the type and name separated by a space.
	 */
	TEST_FEATURE ("with non-pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			var = type_var_new (NULL, "int", "foo");
		}

		str = type_var_to_string (NULL, var);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var);
			continue;
		}

		TEST_EQ_STR (str, "int foo");

		nih_free (str);
		nih_free (var);
	}


	/* Check to make sure that a pointer variable is returned with
	 * the type and name separated by no spaces.
	 */
	TEST_FEATURE ("with pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			var = type_var_new (NULL, "char *", "foo");
		}

		str = type_var_to_string (NULL, var);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var);
			continue;
		}

		TEST_EQ_STR (str, "char *foo");

		nih_free (str);
		nih_free (var);
	}
}

void
test_var_layout (void)
{
	NihList  vars;
	TypeVar *var1 = NULL;
	TypeVar *var2 = NULL;
	TypeVar *var3 = NULL;
	char *   str;

	TEST_FUNCTION ("test_var_layout");


	/* Check that we can lay out a couple of different non-pointer
	 * variables with them lined up with the longest type name
	 * separated by a space and the other names lined up.
	 */
	TEST_FEATURE ("with set of non-pointers");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		TEST_ALLOC_SAFE {
			var1 = type_var_new (NULL, "int", "foo");
			nih_list_add (&vars, &var1->entry);

			var2 = type_var_new (NULL, "struct bar", "bar");
			nih_list_add (&vars, &var2->entry);

			var3 = type_var_new (NULL, "uint32_t", "baz");
			nih_list_add (&vars, &var3->entry);
		}

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var1);
			nih_free (var2);
			nih_free (var3);
			continue;
		}

		TEST_EQ_STR (str, ("int        foo;\n"
				   "struct bar bar;\n"
				   "uint32_t   baz;\n"));

		nih_free (var1);
		nih_free (var2);
		nih_free (var3);

		nih_free (str);
	}


	/* Check that we can lay out a couple of different pointer
	 * variables with them lined up with the longest type name
	 * followed by the name with the others lined up under it.
	 */
	TEST_FEATURE ("with set of pointers");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		TEST_ALLOC_SAFE {
			var1 = type_var_new (NULL, "int *", "foo");
			nih_list_add (&vars, &var1->entry);

			var2 = type_var_new (NULL, "struct bar *", "bar");
			nih_list_add (&vars, &var2->entry);

			var3 = type_var_new (NULL, "uint32_t *", "baz");
			nih_list_add (&vars, &var3->entry);
		}

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var1);
			nih_free (var2);
			nih_free (var3);
			continue;
		}

		TEST_EQ_STR (str, ("int *       foo;\n"
				   "struct bar *bar;\n"
				   "uint32_t *  baz;\n"));

		nih_free (var1);
		nih_free (var2);
		nih_free (var3);

		nih_free (str);
	}


	/* Check that we can lay out a mix of pointer and non-pointer
	 * variables with them lined up with the longest type name
	 * followed by the name with the others lined up under it.
	 */
	TEST_FEATURE ("with mixed set");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		TEST_ALLOC_SAFE {
			var1 = type_var_new (NULL, "int *", "foo");
			nih_list_add (&vars, &var1->entry);

			var2 = type_var_new (NULL, "struct bar", "bar");
			nih_list_add (&vars, &var2->entry);

			var3 = type_var_new (NULL, "uint32_t *", "baz");
			nih_list_add (&vars, &var3->entry);
		}

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);

			nih_free (var1);
			nih_free (var2);
			nih_free (var3);
			continue;
		}

		TEST_EQ_STR (str, ("int *      foo;\n"
				   "struct bar bar;\n"
				   "uint32_t * baz;\n"));

		nih_free (var1);
		nih_free (var2);
		nih_free (var3);

		nih_free (str);
	}


	/* Check that we can accept an empty set, and have the empty
	 * string returned.
	 */
	TEST_FEATURE ("with empty list");
	TEST_ALLOC_FAIL {
		nih_list_init (&vars);

		str = type_var_layout (NULL, &vars);

 		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			continue;
		}

		TEST_EQ_STR (str, "");

		nih_free (str);
	}
}


void
test_func_new (void)
{
	TypeFunc *func;

	TEST_FUNCTION ("type_func_new");


	/* Check to make sure that a TypeFunc structure is allocated
	 * correctly and returned.
	 */
	TEST_ALLOC_FAIL {
		func = type_func_new (NULL, "char *", "foo");

		if (test_alloc_failed) {
			TEST_EQ_P (func, NULL);
			continue;
		}

		TEST_ALLOC_SIZE (func, sizeof (TypeFunc));
		TEST_LIST_EMPTY (&func->entry);
		TEST_EQ_STR (func->type, "char *");
		TEST_ALLOC_PARENT (func->type, func);
		TEST_EQ_STR (func->name, "foo");
		TEST_ALLOC_PARENT (func->name, func);
		TEST_LIST_EMPTY (&func->args);
		TEST_LIST_EMPTY (&func->attribs);

		nih_free (func);
	}
}


void
test_to_const (void)
{
	char *str = NULL;
	char *ret;

	TEST_FUNCTION ("type_to_const");


	/* Check to make sure that a non-pointer declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with non-pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "int");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "int");

		nih_free (str);
	}


	/* Check that a simple first-level pointer has the const initialiser
	 * prepended onto the front, before the type name.
	 */
	TEST_FEATURE ("with pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const char *");

		nih_free (str);
	}


	/* Check that a two-level pointer has the const initialiser
	 * placed before the final pointer operator.
	 */
	TEST_FEATURE ("with pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char **");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char * const *");

		nih_free (str);
	}


	/* Check that a three-level pointer has the const initialiser
	 * placed before the final pointer operator.
	 */
	TEST_FEATURE ("with pointer pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char ***");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char ***");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char ** const *");

		nih_free (str);
	}


	/* Check that an already-const pointer declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const struct foo *");

		nih_free (str);
	}


	/* Check that an already-const pointer pointer declaration is returned
	 * unmodified.
	 */
	TEST_FEATURE ("with const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const *");

		nih_free (str);
	}


	/* Check that a pointer to a const pointer is modified to be a
	 * const pointer to a const pointer.
	 */
	TEST_FEATURE ("with pointer to const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo **");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const struct foo * const *");

		nih_free (str);
	}


	/* Check that an already-const pointer pointer pointer declaration
	 * is returned unmodified.
	 */
	TEST_FEATURE ("with const pointer pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo ** const *");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo ** const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo ** const *");

		nih_free (str);
	}


	/* Check that a pointer to a const pointer pointer is modified
	 * to be a constant pointer to a const pointer pointer.
	 */
	TEST_FEATURE ("with pointer to const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const **");
		}

		ret = type_to_const (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const * const *");

		nih_free (str);
	}
}

void
test_to_pointer (void)
{
	char *str = NULL;
	char *ret;

	TEST_FUNCTION ("type_to_pointer");


	/* Check to make sure that a non-pointer declaration is returned
	 * as a pointer.
	 */
	TEST_FEATURE ("with non-pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "int");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "int");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "int *");

		nih_free (str);
	}


	/* Check that a simple first-level pointer has a further pointer
	 * level added.
	 */
	TEST_FEATURE ("with pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char **");

		nih_free (str);

	}


	/* Check that a two-level pointer has yet another further pointer
	 * level added.
	 */
	TEST_FEATURE ("with pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "char **");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "char **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "char ***");

		nih_free (str);
	}


	/* Check that a constant pointer has a further level of pointerness
	 * added, and the const moved to the new first level.
	 */
	TEST_FEATURE ("with const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const *");

		nih_free (str);

	}


	/* Check that a constant pointer pointer has a further level of
	 * pointerness added and the const moved to the new first level.
	 */
	TEST_FEATURE ("with const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo ** const *");

		nih_free (str);

	}


	/* Check that a constant pointer pointer pointer has a further level
	 * of pointerness added and the const moved to the new first level.
	 */
	TEST_FEATURE ("with const pointer pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo ** const *");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo ** const *");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo *** const *");

		nih_free (str);

	}


	/* Check that a pointer to a constant pointer only has a further
	 * level of pointerness added, and the const pointer is not
	 * moved to the new first level.
	 */
	TEST_FEATURE ("with pointer to const pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "const struct foo **");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "const struct foo **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "const struct foo ***");

		nih_free (str);

	}


	/* Check that a pointer to a constant pointer pointer only has a
	 * further level of pointerness added, and the const pointer is not
	 * moved to the new first level.
	 */
	TEST_FEATURE ("with pointer to const pointer pointer");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			str = nih_strdup (NULL, "struct foo * const **");
		}

		ret = type_to_pointer (&str, NULL);

		if (test_alloc_failed) {
			TEST_EQ_P (ret, NULL);

			TEST_EQ_STR (str, "struct foo * const **");

			nih_free (str);
			continue;
		}

		TEST_EQ_P (ret, str);
		TEST_EQ_STR (str, "struct foo * const ***");

		nih_free (str);

	}
}


int
main (int   argc,
      char *argv[])
{
	test_const ();

	test_of ();

	test_var_new ();
	test_var_to_string ();
	test_var_layout ();

	test_func_new ();

	test_to_const ();
	test_to_pointer ();

	return 0;
}
