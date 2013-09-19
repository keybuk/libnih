/* nih-dbus-tool
 *
 * test_marshal.c - test suite for nih-dbus-tool/marshal.c
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

#include <nih/test.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>

#include "type.h"
#include "marshal.h"
#include "tests/marshal_code.h"


void
test_marshal (void)
{
	NihList           inputs;
	NihList           locals;
	NihList           structs;
	TypeVar *         var;
	TypeStruct *      structure;
	DBusSignatureIter signature;
	DBusMessage *     message = NULL;
	DBusMessageIter   iter;
	DBusMessageIter   subiter;
	DBusMessageIter   subsubiter;
	char *            str;
	int               ret;
	uint8_t           byte_value;
	int               boolean_value;
	int16_t           int16_value;
	uint16_t          uint16_value;
	int32_t           int32_value;
	uint32_t          uint32_value;
	int64_t           int64_value;
	uint64_t          uint64_value;
	double            double_value;
	char *            str_value;
	int16_t *         int16_array = NULL;
	size_t            int16_array_len;
	int16_t **        int16_array_array = NULL;
	size_t *          int16_array_array_len = NULL;
	char **           str_array = NULL;
	char ***          str_array_array = NULL;
	MyStructValue *   struct_value = NULL;
	MyStructArrayValueElement **struct_array = NULL;
	MyDictEntryArrayValueElement **dict_entry_array = NULL;
	int               unix_fd_value;

	TEST_FUNCTION ("marshal");


	/* Check that the code to marshal a uint8_t into a D-Bus Byte is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with byte");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_BYTE_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "byte", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a uint8_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint8_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the uint8_t and
	 * appends it as a D-Bus Byte to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with byte (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		byte_value = 42;

		ret = my_byte_marshal (message, byte_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_BYTE_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_BYTE);

		byte_value = 0;
		dbus_message_iter_get_basic (&iter, &byte_value);
		TEST_EQ (byte_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an int into a D-Bus Boolean is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with boolean");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_BOOLEAN_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "boolean", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a int onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the int and
	 * appends it as a D-Bus Boolean to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with boolean (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		boolean_value = TRUE;

		ret = my_boolean_marshal (message, boolean_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_BOOLEAN_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_BOOLEAN);

		boolean_value = FALSE;
		dbus_message_iter_get_basic (&iter, &boolean_value);
		TEST_EQ (boolean_value, TRUE);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an int16_t into a D-Bus Int16 is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with int16");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_INT16_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "int16", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a int16_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int16_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the int16_t and
	 * appends it as a D-Bus Int16 to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with int16 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		int16_value = -42;

		ret = my_int16_marshal (message, int16_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_INT16_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&iter, &int16_value);
		TEST_EQ (int16_value, -42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an uint16_t into a D-Bus UInt16 is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with uint16");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UINT16_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "uint16", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a uint16_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint16_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the uint16_t and
	 * appends it as a D-Bus UInt16 to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with uint16 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		uint16_value = 42;

		ret = my_uint16_marshal (message, uint16_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_UINT16_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_UINT16);

		uint16_value = 0;
		dbus_message_iter_get_basic (&iter, &uint16_value);
		TEST_EQ (uint16_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an int32_t into a D-Bus Int32 is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with int32");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_INT32_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "int32", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a int32_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the int32_t and
	 * appends it as a D-Bus Int32 to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with int32 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		int32_value = -42;

		ret = my_int32_marshal (message, int32_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_INT32_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT32);

		int32_value = 0;
		dbus_message_iter_get_basic (&iter, &int32_value);
		TEST_EQ (int32_value, -42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an uint32_t into a D-Bus UInt32 is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with uint32");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UINT32_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "uint32", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a uint32_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the uint32_t and
	 * appends it as a D-Bus UInt32 to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with uint32 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		uint32_value = 42;

		ret = my_uint32_marshal (message, uint32_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_UINT32_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_UINT32);

		uint32_value = 0;
		dbus_message_iter_get_basic (&iter, &uint32_value);
		TEST_EQ (uint32_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an int64_t into a D-Bus Int64 is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with int64");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_INT64_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "int64", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a int64_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int64_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the int64_t and
	 * appends it as a D-Bus Int64 to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with int64 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		int64_value = -42;

		ret = my_int64_marshal (message, int64_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_INT64_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INT64);

		int64_value = 0;
		dbus_message_iter_get_basic (&iter, &int64_value);
		TEST_EQ (int64_value, -42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an uint64_t into a D-Bus UInt64 is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with uint64");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UINT64_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "uint64", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a uint64_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint64_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the uint64_t and
	 * appends it as a D-Bus UInt64 to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with uint64 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		uint64_value = 42;

		ret = my_uint64_marshal (message, uint64_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_UINT64_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_UINT64);

		uint64_value = 0;
		dbus_message_iter_get_basic (&iter, &uint64_value);
		TEST_EQ (uint64_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an double into a D-Bus Double is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with double");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_DOUBLE_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "double", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a double onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "double");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the double and
	 * appends it as a D-Bus Double to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with double (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		double_value = 3.14;

		ret = my_double_marshal (message, double_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_DOUBLE_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_DOUBLE);

		double_value = 0;
		dbus_message_iter_get_basic (&iter, &double_value);
		TEST_EQ (double_value, 3.14);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal a char * into a D-Bus String is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with string");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_STRING_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "string", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a char * onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the char * and
	 * appends it as a D-Bus String to the message we pass.  We check the
	 * message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with string (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		str_value = "hello there";

		ret = my_string_marshal (message, str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_STRING_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "hello there");

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal a char * into a D-Bus Object Path is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with object path");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_OBJECT_PATH_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "object_path", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a char * onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the char * and
	 * appends it as a D-Bus Object Path to the message we pass.  We check
	 * the message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with object path (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		str_value = "/com/netsplit/Nih/Test";

		ret = my_object_path_marshal (message, str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_OBJECT_PATH_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_OBJECT_PATH);

		str_value = NULL;
		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal a char * into a D-Bus Signature is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with signature");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_SIGNATURE_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "signature", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a char * onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the char * and
	 * appends it as a D-Bus Signature to the message we pass.  We check
	 * the message signature is correct, then iterate the message to check
	 * the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with signature (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		str_value = "a(ii)";

		ret = my_signature_marshal (message, str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_SIGNATURE_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_SIGNATURE);

		str_value = NULL;
		dbus_message_iter_get_basic (&iter, &str_value);
		TEST_EQ_STR (str_value, "a(ii)");

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to marshal an array of int16_t into a D-Bus
	 * Int16 Array is correctly generated and returned as an allocated
	 * string.  This will a local variable to track the iteration.
	 */
	TEST_FEATURE ("with int16 array");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_INT16_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "int16_array", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"n\", &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "for (size_t value_i = 0; value_i < value_len; value_i++) {\n"
				   "\tint16_t value_element;\n"
				   "\n"
				   "\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t/* Marshal a int16_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_INT16, &value_element)) {\n"
				   "\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int16_t *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_len");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from the array of
	 * int16_t and appends them as a D-Bus Int16 Array to the message
	 * we pass.  We check the message signature is correct, then iterate
	 * the message to check the types are correct, and extract the values
	 * to check that they are correct too.
	 */
	TEST_FEATURE ("with int16 array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			int16_array = nih_alloc (NULL, sizeof (int16_t) * 6);
		}

		int16_array_len = 0;
		int16_array[int16_array_len++] = 4;
		int16_array[int16_array_len++] = 8;
		int16_array[int16_array_len++] = 15;
		int16_array[int16_array_len++] = 16;
		int16_array[int16_array_len++] = 23;
		int16_array[int16_array_len++] = 42;

		ret = my_int16_array_marshal (message, int16_array,
					      int16_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (int16_array);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT16_AS_STRING));

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, 4);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, 8);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, 15);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, 16);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, 23);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subiter, &int16_value);
		TEST_EQ (int16_value, 42);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (int16_array);
	}


	/* Check that the code to marshal an array of arrays of int16_t
	 * into a D-Bus Int16 Array Array is correctly generated and
	 * returned as an allocated string.  This will require a local
	 * variable to track the iteration, and iterates over both the
	 * int16_t array and a size_t length array.
	 */
	TEST_FEATURE ("with int16 array array");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_INT16_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "int16_array_array", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"an\", &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (value) {\n"
				   "\tfor (size_t value_i = 0; value[value_i]; value_i++) {\n"
				   "\t\tDBusMessageIter value_element_iter;\n"
				   "\t\tconst int16_t * value_element;\n"
				   "\t\tsize_t          value_element_len;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\t\tvalue_element_len = value_len[value_i];\n"
				   "\n"

				   "\t\t/* Marshal an array onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&value_iter, DBUS_TYPE_ARRAY, \"n\", &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tfor (size_t value_element_i = 0; value_element_i < value_element_len; value_element_i++) {\n"
				   "\t\t\tint16_t value_element_element;\n"
				   "\n"
				   "\t\t\tvalue_element_element = value_element[value_element_i];\n"
				   "\n"
				   "\t\t\t/* Marshal a int16_t onto the message */\n"
				   "\t\t\tif (! dbus_message_iter_append_basic (&value_element_iter, DBUS_TYPE_INT16, &value_element_element)) {\n"
				   "\t\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_element_iter);\n"
				   "\t\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\t\treturn -1;\n"
				   "\t\t\t}\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&value_iter, &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"

				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int16_t **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "size_t *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_len");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from the array of
	 * int16_t arrays and appends them as a D-Bus Int16 Array Array to
	 * the message we pass.  We check the message signature is correct,
	 * then iterate the message to check the types are correct, and
	 * extract the values to check that they are correct too.
	 */
	TEST_FEATURE ("with int16 array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			int16_array_array = nih_alloc (NULL, sizeof (int16_t *) * 3);
			int16_array_array_len = nih_alloc (int16_array_array, sizeof (size_t) * 2);

			int16_array_array[0] = nih_alloc (int16_array_array, sizeof (int16_t) * 6);
			int16_array_array[1] = nih_alloc (int16_array_array, sizeof (int16_t) * 3);
			int16_array_array[2] = NULL;
		}

		int16_array_array_len[0] = 6;
		int16_array_array[0][0] = 4;
		int16_array_array[0][1] = 8;
		int16_array_array[0][2] = 15;
		int16_array_array[0][3] = 16;
		int16_array_array[0][4] = 23;
		int16_array_array[0][5] = 42;

		int16_array_array_len[1] = 3;
		int16_array_array[1][0] = 999;
		int16_array_array[1][1] = 911;
		int16_array_array[1][2] = 112;

		ret = my_int16_array_array_marshal (message, int16_array_array,
						    int16_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (int16_array_array);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_INT16_AS_STRING));

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 4);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 8);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 15);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 16);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 23);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 42);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 999);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 911);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 112);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (int16_array_array);
	}


	/* Check that the code to marshal an array of char * into a D-Bus
	 * String Array is correctly generated and returned as an allocated
	 * string.   Alocal variable to track the iteration is required.
	 */
	TEST_FEATURE ("with string array");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "string_array", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"s\", &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (value) {\n"
				   "\tfor (size_t value_i = 0; value[value_i]; value_i++) {\n"
				   "\t\tconst char *value_element;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from the array of
	 * char * up to the NULL and appends them as a D-Bus String Array to
	 * the message we pass.  The NULL pointer itself should not be
	 * appended.  We check the message signature is correct, then iterate
	 * the message to check the types are correct, and extract the values
	 * to check that they are correct too.
	 */
	TEST_FEATURE ("with string array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			str_array = nih_alloc (NULL, sizeof (char *) * 4);
		}

		str_array[0] = "hello";
		str_array[1] = "cruel";
		str_array[2] = "world";
		str_array[3] = NULL;

		ret = my_string_array_marshal (message, str_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (str_array);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING));

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "hello");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "cruel");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "world");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (str_array);
	}


	/* Check that the code to marshal an array of arrays of char * into
	 * a D-Bus String Array Array is correctly generated and returned
	 * as an allocated string.  We have a local variable to track the
	 * iteration, but there should be no other leak from inside.
	 */
	TEST_FEATURE ("with string array array");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "string_array_array", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"as\", &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (value) {\n"
				   "\tfor (size_t value_i = 0; value[value_i]; value_i++) {\n"
				   "\t\tDBusMessageIter value_element_iter;\n"
				   "\t\tchar * const *  value_element;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t\t/* Marshal an array onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&value_iter, DBUS_TYPE_ARRAY, \"s\", &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (value_element) {\n"
				   "\t\t\tfor (size_t value_element_i = 0; value_element[value_element_i]; value_element_i++) {\n"
				   "\t\t\t\tconst char *value_element_element;\n"
				   "\n"
				   "\t\t\t\tvalue_element_element = value_element[value_element_i];\n"
				   "\n"
				   "\t\t\t\t/* Marshal a char * onto the message */\n"
				   "\t\t\t\tif (! dbus_message_iter_append_basic (&value_element_iter, DBUS_TYPE_STRING, &value_element_element)) {\n"
				   "\t\t\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_element_iter);\n"
				   "\t\t\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\t\t\treturn -1;\n"
				   "\t\t\t\t}\n"
				   "\t\t\t}\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&value_iter, &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char ***");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from the array of
	 * arrays of char * up to each of the NULLs and appends them as
	 * D-Bus String Arrays to a D-Bus String Array Array for the message
	 * we pass.  The NULL pointers themselves should not be appended.
	 * We check the message signature is correct, then iterate
	 * the message to check the types are correct, and extract the values
	 * to check that they are correct too.
	 */
	TEST_FEATURE ("with string array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			str_array_array = nih_alloc (NULL, sizeof (char **) * 3);
			str_array_array[0] = nih_alloc (str_array_array, sizeof (char *) * 4);
			str_array_array[1] = nih_alloc (str_array_array, sizeof (char *) * 3);
			str_array_array[2] = NULL;
		}

		str_array_array[0][0] = "hello";
		str_array_array[0][1] = "cruel";
		str_array_array[0][2] = "world";
		str_array_array[0][3] = NULL;

		str_array_array[1][0] = "frodo";
		str_array_array[1][1] = "baggins";
		str_array_array[1][2] = NULL;

		ret = my_string_array_array_marshal (message, str_array_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (str_array_array);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     (DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_ARRAY_AS_STRING
			      DBUS_TYPE_STRING_AS_STRING));

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "hello");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "cruel");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "world");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "frodo");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "baggins");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (str_array_array);
	}


 	/* Check that the code to marshal a structure into a D-Bus Struct is
	 * correctly generated and returned as an allocated string, containing
	 * the marshalling code for each of the structure's members.
	 */
	TEST_FEATURE ("with structure");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_UINT32_AS_STRING
					   DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_INT16_AS_STRING
					   DBUS_STRUCT_END_CHAR_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "struct", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a structure onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value_item0 = value->item0;\n"
				   "\n"
				   "/* Marshal a char * onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_STRING, &value_item0)) {\n"
				   "\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value_item1 = value->item1;\n"
				   "\n"
				   "/* Marshal a uint32_t onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&value_iter, DBUS_TYPE_UINT32, &value_item1)) {\n"
				   "\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value_item2 = value->item2;\n"
				   "\n"
				   "/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&value_iter, DBUS_TYPE_ARRAY, \"s\", &value_item2_iter)) {\n"
				   "\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (value_item2) {\n"
				   "\tfor (size_t value_item2_i = 0; value_item2[value_item2_i]; value_item2_i++) {\n"
				   "\t\tconst char *value_item2_element;\n"
				   "\n"
				   "\t\tvalue_item2_element = value_item2[value_item2_i];\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_item2_iter, DBUS_TYPE_STRING, &value_item2_element)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_item2_iter);\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&value_iter, &value_item2_iter)) {\n"
				   "\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value_item3 = value->item3;\n"
				   "value_item3_len = value->item3_len;\n"
				   "\n"
				   "/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&value_iter, DBUS_TYPE_ARRAY, \"n\", &value_item3_iter)) {\n"
				   "\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "for (size_t value_item3_i = 0; value_item3_i < value_item3_len; value_item3_i++) {\n"
				   "\tint16_t value_item3_element;\n"
				   "\n"
				   "\tvalue_item3_element = value_item3[value_item3_i];\n"
				   "\n"
				   "\t/* Marshal a int16_t onto the message */\n"
				   "\tif (! dbus_message_iter_append_basic (&value_item3_iter, DBUS_TYPE_INT16, &value_item3_element)) {\n"
				   "\t\tdbus_message_iter_abandon_container (&value_iter, &value_item3_iter);\n"
				   "\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&value_iter, &value_item3_iter)) {\n"
				   "\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "MyStructValue *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item2_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char * const *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item2");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item3_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const int16_t *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item3");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item3_len");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyStructValue");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item2");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "int16_t *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item3");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item3_len");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes each of the values from a
	 * structure and appends them into a D-Bus Structure to the message
	 * we pass.  We check the message signature is correct, then iterate
	 * the message to check the types are correct, and extract the values
	 * to check that they* are correct too.
	 */
	TEST_FEATURE ("with structure (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			struct_value = nih_new (NULL, MyStructValue);
			struct_value->item0 = "hello there";
			struct_value->item1 = 1818118181;
			struct_value->item2 = nih_alloc (struct_value, sizeof (char *) * 3);
			struct_value->item2[0] = "welcome";
			struct_value->item2[1] = "aboard";
			struct_value->item2[2] = NULL;
			struct_value->item3 = nih_alloc (struct_value, sizeof (int16_t) * 6);
			struct_value->item3[0] = 4;
			struct_value->item3[1] = 8;
			struct_value->item3[2] = 15;
			struct_value->item3[3] = 16;
			struct_value->item3[4] = 23;
			struct_value->item3[5] = 42;
			struct_value->item3_len = 6;
		}

		ret = my_struct_marshal (message, struct_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (struct_value);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			     DBUS_TYPE_STRING_AS_STRING
			     DBUS_TYPE_UINT32_AS_STRING
			     DBUS_TYPE_ARRAY_AS_STRING
			     DBUS_TYPE_STRING_AS_STRING
			     DBUS_TYPE_ARRAY_AS_STRING
			     DBUS_TYPE_INT16_AS_STRING
			     DBUS_STRUCT_END_CHAR_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subiter, &str_value);
		TEST_EQ_STR (str_value, "hello there");

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_UINT32);

		uint32_value = 0;
		dbus_message_iter_get_basic (&subiter, &uint32_value);
		TEST_EQ (uint32_value, 1818118181);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "welcome");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "aboard");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 4);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 8);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 15);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 16);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 23);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INT16);

		int16_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &int16_value);
		TEST_EQ (int16_value, 42);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (struct_value);
	}


 	/* Check that the code to marshal an array of structures into a
	 * D-Bus Struct Array is correctly generated and returned as an
	 * allocated string, containing the marshalling code for each of
	 * the structures.
	 */
	TEST_FEATURE ("with structure array");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_UINT32_AS_STRING
					   DBUS_STRUCT_END_CHAR_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "struct_array", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"(su)\", &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (value) {\n"
				   "\tfor (size_t value_i = 0; value[value_i]; value_i++) {\n"
				   "\t\tDBusMessageIter                  value_element_iter;\n"
				   "\t\tconst char *                     value_element_item0;\n"
				   "\t\tuint32_t                         value_element_item1;\n"
				   "\t\tconst MyStructArrayValueElement *value_element;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t\t/* Marshal a structure onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&value_iter, DBUS_TYPE_STRUCT, NULL, &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element_item0 = value_element->item0;\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_element_iter, DBUS_TYPE_STRING, &value_element_item0)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_element_iter);\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element_item1 = value_element->item1;\n"
				   "\n"
				   "\t\t/* Marshal a uint32_t onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_element_iter, DBUS_TYPE_UINT32, &value_element_item1)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_element_iter);\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&value_iter, &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "MyStructArrayValueElement **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyStructArrayValueElement");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes each of the values from an
	 * array of structures and appends them into a D-Bus Structure Array
	 * to the message we pass.  We check the message signature is correct,
	 * then iterate the message to check the types are correct, and
	 * extract the values to check that they* are correct too.
	 */
	TEST_FEATURE ("with structure array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			struct_array = nih_alloc (NULL, sizeof (MyStructArrayValueElement *) * 3);

			struct_array[0] = nih_new (struct_array, MyStructArrayValueElement);
			struct_array[0]->item0 = "hello there";
			struct_array[0]->item1 = 1818118181;

			struct_array[1] = nih_new (struct_array, MyStructArrayValueElement);
			struct_array[1]->item0 = "goodbye world";
			struct_array[1]->item1 = 12345;

			struct_array[2] = NULL;
		}

		ret = my_struct_array_marshal (message, struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (struct_array);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_ARRAY_AS_STRING
			     DBUS_STRUCT_BEGIN_CHAR_AS_STRING
			     DBUS_TYPE_STRING_AS_STRING
			     DBUS_TYPE_UINT32_AS_STRING
			     DBUS_STRUCT_END_CHAR_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "hello there");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_UINT32);

		uint32_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &uint32_value);
		TEST_EQ (uint32_value, 1818118181);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_STRUCT);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "goodbye world");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_UINT32);

		uint32_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &uint32_value);
		TEST_EQ (uint32_value, 12345);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (struct_array);
	}


 	/* Check that the code to marshal an array of key/value structures
	 * into a D-Bus DictEntry Array is correctly generated and returned
	 * as an allocated string, containing the marshalling code for each of
	 * the structures.
	 */
	TEST_FEATURE ("with dict entry array");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_UINT32_AS_STRING
					   DBUS_DICT_ENTRY_END_CHAR_AS_STRING));

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "dict_entry_array", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal an array onto the message */\n"
				   "if (! dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, \"{su}\", &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "if (value) {\n"
				   "\tfor (size_t value_i = 0; value[value_i]; value_i++) {\n"
				   "\t\tDBusMessageIter                     value_element_iter;\n"
				   "\t\tconst char *                        value_element_item0;\n"
				   "\t\tuint32_t                            value_element_item1;\n"
				   "\t\tconst MyDictEntryArrayValueElement *value_element;\n"
				   "\n"
				   "\t\tvalue_element = value[value_i];\n"
				   "\n"
				   "\t\t/* Marshal a structure onto the message */\n"
				   "\t\tif (! dbus_message_iter_open_container (&value_iter, DBUS_TYPE_DICT_ENTRY, NULL, &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element_item0 = value_element->item0;\n"
				   "\n"
				   "\t\t/* Marshal a char * onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_element_iter, DBUS_TYPE_STRING, &value_element_item0)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_element_iter);\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element_item1 = value_element->item1;\n"
				   "\n"
				   "\t\t/* Marshal a uint32_t onto the message */\n"
				   "\t\tif (! dbus_message_iter_append_basic (&value_element_iter, DBUS_TYPE_UINT32, &value_element_item1)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&value_iter, &value_element_iter);\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tif (! dbus_message_iter_close_container (&value_iter, &value_element_iter)) {\n"
				   "\t\t\tdbus_message_iter_abandon_container (&iter, &value_iter);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\t}\n"
				   "}\n"
				   "\n"
				   "if (! dbus_message_iter_close_container (&iter, &value_iter)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "MyDictEntryArrayValueElement **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "DBusMessageIter");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_iter");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);


		TEST_LIST_NOT_EMPTY (&structs);

		structure = (TypeStruct *)structs.next;
		TEST_ALLOC_SIZE (structure, sizeof (TypeStruct));
		TEST_ALLOC_PARENT (structure, str);
		TEST_EQ_STR (structure->name, "MyDictEntryArrayValueElement");
		TEST_ALLOC_PARENT (structure->name, structure);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item0");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&structure->members);

		var = (TypeVar *)structure->members.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, structure);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "item1");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&structure->members);
		nih_free (structure);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes each of the values from an
	 * array of dict entries and appends them into a D-Bus DictEntry Array
	 * to the message we pass.  We check the message signature is correct,
	 * then iterate the message to check the types are correct, and
	 * extract the values to check that they* are correct too.
	 */
	TEST_FEATURE ("with dict entry array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dict_entry_array = nih_alloc (NULL, sizeof (MyDictEntryArrayValueElement *) * 3);

			dict_entry_array[0] = nih_new (dict_entry_array, MyDictEntryArrayValueElement);
			dict_entry_array[0]->item0 = "hello there";
			dict_entry_array[0]->item1 = 1818118181;

			dict_entry_array[1] = nih_new (dict_entry_array, MyDictEntryArrayValueElement);
			dict_entry_array[1]->item0 = "goodbye world";
			dict_entry_array[1]->item1 = 12345;

			dict_entry_array[2] = NULL;
		}

		ret = my_dict_entry_array_marshal (message, dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();

			nih_free (dict_entry_array);
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_ARRAY_AS_STRING
			     DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			     DBUS_TYPE_STRING_AS_STRING
			     DBUS_TYPE_UINT32_AS_STRING
			     DBUS_DICT_ENTRY_END_CHAR_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_ARRAY);

		dbus_message_iter_recurse (&iter, &subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "hello there");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_UINT32);

		uint32_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &uint32_value);
		TEST_EQ (uint32_value, 1818118181);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_DICT_ENTRY);

		dbus_message_iter_recurse (&subiter, &subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_STRING);

		str_value = NULL;
		dbus_message_iter_get_basic (&subsubiter, &str_value);
		TEST_EQ_STR (str_value, "goodbye world");

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_UINT32);

		uint32_value = 0;
		dbus_message_iter_get_basic (&subsubiter, &uint32_value);
		TEST_EQ (uint32_value, 12345);

		dbus_message_iter_next (&subsubiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subsubiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&subiter);

		TEST_EQ (dbus_message_iter_get_arg_type (&subiter),
			 DBUS_TYPE_INVALID);

		dbus_message_iter_next (&iter);

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_INVALID);

		dbus_message_unref (message);

		dbus_shutdown ();

		nih_free (dict_entry_array);
	}


	/* Check that the code to marshal an int into a D-Bus file descriptor
	 * is correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with file descriptor");
	TEST_ALLOC_FAIL {
		nih_list_init (&inputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UNIX_FD_AS_STRING);

		str = marshal (NULL, &signature,
			       "iter", "value",
			       "return -1;\n",
			       &inputs, &locals,
			       "my", NULL, "unix_fd", "value",
			       &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&inputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Marshal a int onto the message */\n"
				   "if (! dbus_message_iter_append_basic (&iter, DBUS_TYPE_UNIX_FD, &value)) {\n"
				   "\treturn -1;\n"
				   "}\n"));

		TEST_LIST_NOT_EMPTY (&inputs);

		var = (TypeVar *)inputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&inputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the int and
	 * appends it as a D-Bus file descriptor to the message we pass.  We
	 * check the message signature is correct, then iterate the message to
	 * check the types are correct, and extract the values to check that they
	 * are correct too.
	 */
	TEST_FEATURE ("with file descriptor (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);
		}

		unix_fd_value = 1;

		ret = my_unix_fd_marshal (message, unix_fd_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);

		TEST_EQ_STR (dbus_message_get_signature (message),
			     DBUS_TYPE_UNIX_FD_AS_STRING);

		assert (dbus_message_iter_init (message, &iter));

		TEST_EQ (dbus_message_iter_get_arg_type (&iter),
			 DBUS_TYPE_UNIX_FD);

		unix_fd_value = -1;
		dbus_message_iter_get_basic (&iter, &unix_fd_value);
		TEST_GT (unix_fd_value, 2); // file descriptor is duplicated

		dbus_message_unref (message);
		close (unix_fd_value);

		dbus_shutdown ();
	}
}


int
main (int   argc,
      char *argv[])
{
	test_marshal ();

	return 0;
}
