/* nih-dbus-tool
 *
 * test_demarshal.c - test suite for nih-dbus-tool/demarshal.c
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

#include <nih/test.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/list.h>

#include "type.h"
#include "demarshal.h"
#include "tests/demarshal_code.h"


void
test_demarshal (void)
{
	NihList           outputs;
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
	int16_t *         int16_array;
	size_t            int16_array_len;
	int16_t **        int16_array_array;
	size_t *          int16_array_array_len;
	char **           str_array;
	char ***          str_array_array;
	MyStructValue *   struct_value;
	MyStructArrayValueElement **struct_array;
	MyDictEntryArrayValueElement **dict_entry_array;
	int               unix_fd_value;

	TEST_FUNCTION ("demarshal");


	/* Check that the code to demarshal a D-Bus Byte into a uint8_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with byte");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_BYTE_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "byte", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a uint8_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_BYTE) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint8_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus Byte
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with byte (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			byte_value = 42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
							&byte_value);
		}

		byte_value = 0;

		ret = my_byte_demarshal (NULL, message, &byte_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (byte_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a byte is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for byte (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		byte_value = 0;

		ret = my_byte_demarshal (NULL, message, &byte_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (byte_value, FALSE);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Boolean into an int is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with boolean");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_BOOLEAN_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "boolean", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a int from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_BOOLEAN) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus
	 * Boolean in the message we pass and stores it in the int pointer,
	 * which should have the right value.
	 */
	TEST_FEATURE ("with boolean (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			boolean_value = TRUE;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN,
							&boolean_value);
		}

		boolean_value = FALSE;

		ret = my_boolean_demarshal (NULL, message, &boolean_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (boolean_value, TRUE);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a boolean is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for boolean (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		boolean_value = FALSE;

		ret = my_boolean_demarshal (NULL, message, &boolean_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (boolean_value, FALSE);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Int16 into a int16_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with int16");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_INT16_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "int16", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a int16_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT16) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int16_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus Int16
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with int16 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			int16_value = -42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT16,
							&int16_value);
		}

		int16_value = 0;

		ret = my_int16_demarshal (NULL, message, &int16_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int16_value, -42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int16 is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for int16 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		int16_value = 0;

		ret = my_int16_demarshal (NULL, message, &int16_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (int16_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus UInt16 into a uint16_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with uint16");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UINT16_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "uint16", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a uint16_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT16) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint16_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus UInt16
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with uint16 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			uint16_value = 42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT16,
							&uint16_value);
		}

		uint16_value = 0;

		ret = my_uint16_demarshal (NULL, message, &uint16_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint16_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a uint16 is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for uint16 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		uint16_value = 0;

		ret = my_uint16_demarshal (NULL, message, &uint16_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (uint16_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Int32 into a int32_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with int32");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_INT32_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "int32", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a int32_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus Int32
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with int32 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			int32_value = -42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,
							&int32_value);
		}

		int32_value = 0;

		ret = my_int32_demarshal (NULL, message, &int32_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int32_value, -42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int32 is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for int32 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		int32_value = 0;

		ret = my_int32_demarshal (NULL, message, &int32_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (int32_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus UInt32 into a uint32_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with uint32");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UINT32_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "uint32", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a uint32_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint32_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus UInt32
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with uint32 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			uint32_value = 42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
							&uint32_value);
		}

		uint32_value = 0;

		ret = my_uint32_demarshal (NULL, message, &uint32_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint32_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a uint32 is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for uint32 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		uint32_value = 0;

		ret = my_uint32_demarshal (NULL, message, &uint32_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (uint32_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Int64 into a int64_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with int64");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_INT64_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "int64", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a int64_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT64) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int64_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus Int64
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with int64 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			int64_value = -42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT64,
							&int64_value);
		}

		int64_value = 0;

		ret = my_int64_demarshal (NULL, message, &int64_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int64_value, -42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int64 is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for int64 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		int64_value = 0;

		ret = my_int64_demarshal (NULL, message, &int64_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (int64_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus UInt64 into a uint64_t is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with uint64");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UINT64_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "uint64", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a uint64_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT64) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "uint64_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus UInt64
	 * in the message we pass and stores it in the uint8_t pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with uint64 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			uint64_value = 42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT64,
							&uint64_value);
		}

		uint64_value = 0;

		ret = my_uint64_demarshal (NULL, message, &uint64_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (uint64_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a uint64 is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for uint64 (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		uint64_value = 0;

		ret = my_uint64_demarshal (NULL, message, &uint64_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (uint64_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Double into a double is
	 * correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with double");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_DOUBLE_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "double", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a double from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_DOUBLE) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "double");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus Double
	 * in the message we pass and stores it in the double pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with double (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 42;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		double_value = 0;

		ret = my_double_demarshal (NULL, message, &double_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (double_value, 42);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a double is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for double (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			byte_value = TRUE;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_BYTE,
							&byte_value);
		}

		double_value = 0;

		ret = my_double_demarshal (NULL, message, &double_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (double_value, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus String into a char *
	 * is correctly generated and returned as an allocated string.
	 * This code differs from the other basic types in that it returns
	 * an allocated copy of the string, so also requires a local variable
	 * to hold the constant D-Bus version so appends an entry to the
	 * list we pass.
	 */
	TEST_FEATURE ("with string");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_STRING_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "string", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a char * from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "value = nih_strdup (parent, value_dbus);\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_dbus");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus String
	 * in the message we pass and stores it in the char * pointer, which
	 * should have the right value.
	 */
	TEST_FEATURE ("with string (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING,
							&str_value);
		}

		str_value = NULL;

		ret = my_string_demarshal (NULL, message, &str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ_STR (str_value, "hello there");
		TEST_ALLOC_PARENT (str_value, NULL);

		nih_free (str_value);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a string is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for string (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		str_value = NULL;

		ret = my_string_demarshal (NULL, message, &str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_value, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Object Path into a char *
	 * is correctly generated and returned as an allocated string.
	 * This code differs from the other basic types in that it returns
	 * an allocated copy of the string, so also requires a local variable
	 * to hold the constant D-Bus version so appends an entry to the
	 * list we pass.
	 */
	TEST_FEATURE ("with object path");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_OBJECT_PATH_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "object_path", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a char * from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_OBJECT_PATH) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "value = nih_strdup (parent, value_dbus);\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_dbus");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus
	 * Object Path in the message we pass and stores it in the
	 * char * pointer, which should have the right value.
	 */
	TEST_FEATURE ("with object path (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			str_value = "/com/netsplit/Nih/Test";
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
							&str_value);
		}

		str_value = NULL;

		ret = my_object_path_demarshal (NULL, message, &str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ_STR (str_value, "/com/netsplit/Nih/Test");
		TEST_ALLOC_PARENT (str_value, NULL);

		nih_free (str_value);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when an object path is expected, but a different type
	 * is found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for object path (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		str_value = NULL;

		ret = my_object_path_demarshal (NULL, message, &str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_value, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Signature into a char *
	 * is correctly generated and returned as an allocated string.
	 * This code differs from the other basic types in that it returns
	 * an allocated copy of the string, so also requires a local variable
	 * to hold the constant D-Bus version so appends an entry to the
	 * list we pass.
	 */
	TEST_FEATURE ("with signature");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_SIGNATURE_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "signature", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a char * from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_SIGNATURE) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value_dbus);\n"
				   "\n"
				   "value = nih_strdup (parent, value_dbus);\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "const char *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_dbus");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus
	 * Signature in the message we pass and stores it in the char *
	 * pointer, which should have the right value.
	 */
	TEST_FEATURE ("with signature (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			str_value = "a(ii)";
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_SIGNATURE,
							&str_value);
		}

		str_value = NULL;

		ret = my_signature_demarshal (NULL, message, &str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ_STR (str_value, "a(ii)");
		TEST_ALLOC_PARENT (str_value, NULL);

		nih_free (str_value);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a signature is expected, but a different type is
	 * found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for signature (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		str_value = NULL;

		ret = my_signature_demarshal (NULL, message, &str_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_value, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Int16 Array into an
	 * array of int16_t and a length is correctly generated and returned
	 * as an allocated string.  This differs from others in that it
	 * actually returns two values, the array and the length of the
	 * array.  One local is required, the array iterator, and inside
	 * the generated code should be the locals and inputs to the nested
	 * marshalling code.
	 */
	TEST_FEATURE ("with int16 array");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_INT16_AS_STRING));

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "int16_array", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value_len = 0;\n"
				   "value = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tint16_t *value_tmp;\n"
				   "\tint16_t  value_element;\n"
				   "\n"
				   "\t/* Demarshal a int16_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INT16) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_iter, &value_element);\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "\tif (value_len + 1 > SIZE_MAX / sizeof (int16_t)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_tmp = nih_realloc (value, parent, sizeof (int16_t) * (value_len + 1));\n"
				   "\tif (! value_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue = value_tmp;\n"
				   "\tvalue[value_len] = value_element;\n"
				   "\n"
				   "\tvalue_len++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int16_t *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_len");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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


	/* Check that the generated code takes the values from a D-Bus
	 * Int16 Array in the message we pass and stores them in a newly
	 * allocated int16_t * array, returned along with its length.
	 */
	TEST_FEATURE ("with int16 array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_INT16_AS_STRING,
							  &subiter);

			int16_value = 4;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 8;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 15;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 16;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 23;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 42;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_INT16,
							&int16_value);

			dbus_message_iter_close_container (&iter, &subiter);

		}

		int16_array = NULL;
		int16_array_len = 0;

		ret = my_int16_array_demarshal (NULL, message,
						&int16_array,
						&int16_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_EQ (int16_array_len, 6);
		TEST_ALLOC_PARENT (int16_array, NULL);
		TEST_ALLOC_SIZE (int16_array, sizeof (int16_t) * 6);
		TEST_EQ (int16_array[0], 4);
		TEST_EQ (int16_array[1], 8);
		TEST_EQ (int16_array[2], 15);
		TEST_EQ (int16_array[3], 16);
		TEST_EQ (int16_array[4], 23);
		TEST_EQ (int16_array[5], 42);

		nih_free (int16_array);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int16 array is expected, but a different type is
	 * found at the top-level, the type error code is run and the function
	 * returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for int16 array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		int16_array = NULL;
		int16_array_len = 0;

		ret = my_int16_array_demarshal (NULL, message,
						&int16_array,
						&int16_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (int16_array, NULL);
		TEST_EQ (int16_array_len, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int16 array is expected, but a different type is
	 * found inside the array, the type error code is run and the function
	 * returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type inside int16 array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		int16_array = NULL;
		int16_array_len = 0;

		ret = my_int16_array_demarshal (NULL, message,
						&int16_array,
						&int16_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (int16_array, NULL);
		TEST_EQ (int16_array_len, 0);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Int16 Array Array into an
	 * array of int16_t arrays and a length array is correctly generated
	 * and returned as an allocated string.  This is even more complex
	 * than the int16_t array case since the second value is now an
	 * array of sizes, one for each of the int16 arrays in the first
	 * argument.
	 */
	TEST_FEATURE ("with int16 array array");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_INT16_AS_STRING));

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "int16_array_array", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value_size = 0;\n"
				   "value = NULL;\n"
				   "value_len = NULL;\n"
				   "\n"
				   "value = nih_alloc (parent, sizeof (int16_t *));\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value[value_size] = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tDBusMessageIter value_element_iter;\n"
				   "\tint16_t **      value_tmp;\n"
				   "\tint16_t *       value_element;\n"
				   "\tsize_t *        value_len_tmp;\n"
				   "\tsize_t          value_element_len;\n"
				   "\n"

				   "\t/* Demarshal an array from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_ARRAY) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&value_iter, &value_element_iter);\n"
				   "\n"
				   "\tvalue_element_len = 0;\n"
				   "\tvalue_element = NULL;\n"
				   "\n"
				   "\twhile (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tint16_t *value_element_tmp;\n"
				   "\t\tint16_t  value_element_element;\n"
				   "\n"

				   "\t\t/* Demarshal a int16_t from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_INT16) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn 1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&value_element_iter, &value_element_element);\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&value_element_iter);\n"
				   "\n"

				   "\t\tif (value_element_len + 1 > SIZE_MAX / sizeof (int16_t)) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn 1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element_tmp = nih_realloc (value_element, value, sizeof (int16_t) * (value_element_len + 1));\n"
				   "\t\tif (! value_element_tmp) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element = value_element_tmp;\n"
				   "\t\tvalue_element[value_element_len] = value_element_element;\n"
				   "\n"
				   "\t\tvalue_element_len++;\n"

				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_iter);\n"
				   "\n"

				   "\tif (value_size + 2 > SIZE_MAX / sizeof (int16_t *)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_tmp = nih_realloc (value, parent, sizeof (int16_t *) * (value_size + 2));\n"
				   "\tif (! value_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue = value_tmp;\n"
				   "\tvalue[value_size] = value_element;\n"
				   "\tvalue[value_size + 1] = NULL;\n"
				   "\n"
				   "\tif (value_size + 1 > SIZE_MAX / sizeof (size_t)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_len_tmp = nih_realloc (value_len, value, sizeof (size_t) * (value_size + 1));\n"
				   "\tif (! value_len_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_len = value_len_tmp;\n"
				   "\tvalue_len[value_size] = value_element_len;\n"
				   "\n"
				   "\tvalue_size++;\n"

				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int16_t **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "size_t *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_len");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);
		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from a D-Bus
	 * Int16 Array Array in the message we pass and stores them as
	 * allocated int16_t * arrays, inside their parent array along
	 * with allocated size_t arrays for their lengths.  The length
	 * array must be a child of the main array.
	 */
	TEST_FEATURE ("with int16 array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_TYPE_ARRAY_AS_STRING
							   DBUS_TYPE_INT16_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_INT16_AS_STRING,
							  &subsubiter);

			int16_value = 4;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 8;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 15;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 16;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 23;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 42;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_INT16_AS_STRING,
							  &subsubiter);

			int16_value = 999;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 911;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 112;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		int16_array_array = NULL;
		int16_array_array_len = NULL;

		ret = my_int16_array_array_demarshal (NULL, message,
						      &int16_array_array,
						      &int16_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (int16_array_array, NULL);
		TEST_ALLOC_SIZE (int16_array_array, sizeof (int16_t *) * 3);
		TEST_ALLOC_PARENT (int16_array_array_len, int16_array_array);
		TEST_ALLOC_SIZE (int16_array_array_len, sizeof (size_t) * 2);

		TEST_EQ (int16_array_array_len[0], 6);
		TEST_ALLOC_PARENT (int16_array_array[0], int16_array_array);
		TEST_ALLOC_SIZE (int16_array_array[0], sizeof (int16_t) * 6);
		TEST_EQ (int16_array_array[0][0], 4);
		TEST_EQ (int16_array_array[0][1], 8);
		TEST_EQ (int16_array_array[0][2], 15);
		TEST_EQ (int16_array_array[0][3], 16);
		TEST_EQ (int16_array_array[0][4], 23);
		TEST_EQ (int16_array_array[0][5], 42);

		TEST_EQ (int16_array_array_len[1], 3);
		TEST_ALLOC_PARENT (int16_array_array[1], int16_array_array);
		TEST_ALLOC_SIZE (int16_array_array[1], sizeof (int16_t) * 3);
		TEST_EQ (int16_array_array[1][0], 999);
		TEST_EQ (int16_array_array[1][1], 911);
		TEST_EQ (int16_array_array[1][2], 112);

		nih_free (int16_array_array);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int16 array array is expected, but a different
	 * type is found at the top-level, the type error code is run and
	 * the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for int16 array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		int16_array_array = NULL;
		int16_array_array_len = NULL;

		ret = my_int16_array_array_demarshal (NULL, message,
						      &int16_array_array,
						      &int16_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (int16_array_array, NULL);
		TEST_EQ_P (int16_array_array_len, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int16 array array is expected, but a different
	 * type is found inside the top array, the type error code is run and
	 * the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type inside int16 array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		int16_array_array = NULL;
		int16_array_array_len = NULL;

		ret = my_int16_array_array_demarshal (NULL, message,
						      &int16_array_array,
						      &int16_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (int16_array_array, NULL);
		TEST_EQ_P (int16_array_array_len, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a int16 array array is expected, but a different
	 * type is found inside the second array, the type error code is run
	 * and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type deep inside int16 array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_TYPE_ARRAY_AS_STRING
							   DBUS_TYPE_DOUBLE_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subsubiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		int16_array_array = NULL;
		int16_array_array_len = NULL;

		ret = my_int16_array_array_demarshal (NULL, message,
						      &int16_array_array,
						      &int16_array_array_len);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (int16_array_array, NULL);
		TEST_EQ_P (int16_array_array_len, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus String Array into an
	 * array of char * is correctly generated and returned as an
	 * allocated string.  Two locals are required, the array iterator
	 * and the length of the array.  Inside the generated code should
	 * be the locals and outputs from the nested marshalling code.
	 */
	TEST_FEATURE ("with string array");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING));

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "string_array", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value_size = 0;\n"
				   "value = NULL;\n"
				   "\n"
				   "value = nih_alloc (parent, sizeof (char *));\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value[value_size] = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tconst char *value_element_dbus;\n"
				   "\tchar **     value_tmp;\n"
				   "\tchar *      value_element;\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_iter, &value_element_dbus);\n"
				   "\n"
				   "\tvalue_element = nih_strdup (value, value_element_dbus);\n"
				   "\tif (! value_element) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "\tif (value_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_tmp = nih_realloc (value, parent, sizeof (char *) * (value_size + 2));\n"
				   "\tif (! value_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue = value_tmp;\n"
				   "\tvalue[value_size] = value_element;\n"
				   "\tvalue[value_size + 1] = NULL;\n"
				   "\n"
				   "\tvalue_size++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from a D-Bus
	 * String Array in the message we pass and stores them in a newly
	 * allocated char * array, with each a child of the array itself.
	 */
	TEST_FEATURE ("with string array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subiter);

			str_value = "this";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "is";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "a";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "test";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&iter, &subiter);

		}

		str_array = NULL;

		ret = my_string_array_demarshal (NULL, message,
						 &str_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (str_array, NULL);
		TEST_ALLOC_SIZE (str_array, sizeof (char *) * 5);
		TEST_EQ_STR (str_array[0], "this");
		TEST_ALLOC_PARENT (str_array[0], str_array);
		TEST_EQ_STR (str_array[1], "is");
		TEST_ALLOC_PARENT (str_array[1], str_array);
		TEST_EQ_STR (str_array[2], "a");
		TEST_ALLOC_PARENT (str_array[2], str_array);
		TEST_EQ_STR (str_array[3], "test");
		TEST_ALLOC_PARENT (str_array[3], str_array);
		TEST_EQ_P (str_array[4], NULL);

		nih_free (str_array);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a string array is expected, but a different type is
	 * found at the top-level, the type error code is run and the function
	 * returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for string array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		str_array = NULL;

		ret = my_string_array_demarshal (NULL, message,
						 &str_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a string array is expected, but a different type is
	 * found inside the array, the type error code is run and the function
	 * returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type inside string array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		str_array = NULL;

		ret = my_string_array_demarshal (NULL, message,
						 &str_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus String Array Array into
	 * an array of arrays of char * is correctly generated and returned
	 * as an allocated string.  Two locals are required, the top-level
	 * array iterator and the length of the top-level array.  Inside the
	 * generated code should be the locals and outputs from the nested
	 * marshalling code.
	 */
	TEST_FEATURE ("with string array array");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING));

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "string_array_array", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value_size = 0;\n"
				   "value = NULL;\n"
				   "\n"
				   "value = nih_alloc (parent, sizeof (char **));\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value[value_size] = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tDBusMessageIter value_element_iter;\n"
				   "\tsize_t          value_element_size;\n"
				   "\tchar ***        value_tmp;\n"
				   "\tchar **         value_element;\n"
				   "\n"
				   "\t/* Demarshal an array from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_ARRAY) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&value_iter, &value_element_iter);\n"
				   "\n"
				   "\tvalue_element_size = 0;\n"
				   "\tvalue_element = NULL;\n"
				   "\n"
				   "\tvalue_element = nih_alloc (value, sizeof (char *));\n"
				   "\tif (! value_element) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_element[value_element_size] = NULL;\n"
				   "\n"
				   "\twhile (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tconst char *value_element_element_dbus;\n"
				   "\t\tchar **     value_element_tmp;\n"
				   "\t\tchar *      value_element_element;\n"
				   "\n"
				   "\t\t/* Demarshal a char * from the message */\n"
				   "\t\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn 1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_get_basic (&value_element_iter, &value_element_element_dbus);\n"
				   "\n"
				   "\t\tvalue_element_element = nih_strdup (value_element, value_element_element_dbus);\n"
				   "\t\tif (! value_element_element) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tdbus_message_iter_next (&value_element_iter);\n"
				   "\n"
				   "\t\tif (value_element_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn 1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element_tmp = nih_realloc (value_element, value, sizeof (char *) * (value_element_size + 2));\n"
				   "\t\tif (! value_element_tmp) {\n"
				   "\t\t\tif (value_element)\n"
				   "\t\t\t\tnih_free (value_element);\n"
				   "\t\t\tif (value)\n"
				   "\t\t\t\tnih_free (value);\n"
				   "\t\t\treturn -1;\n"
				   "\t\t}\n"
				   "\n"
				   "\t\tvalue_element = value_element_tmp;\n"
				   "\t\tvalue_element[value_element_size] = value_element_element;\n"
				   "\t\tvalue_element[value_element_size + 1] = NULL;\n"
				   "\n"
				   "\t\tvalue_element_size++;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "\tif (value_size + 2 > SIZE_MAX / sizeof (char **)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_tmp = nih_realloc (value, parent, sizeof (char **) * (value_size + 2));\n"
				   "\tif (! value_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue = value_tmp;\n"
				   "\tvalue[value_size] = value_element;\n"
				   "\tvalue[value_size + 1] = NULL;\n"
				   "\n"
				   "\tvalue_size++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char ***");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the values from a D-Bus
	 * String Array Array in the message we pass and stores them in newly
	 * allocated char * arrays in a newly allocated array, with each
	 * a child of the parent array.
	 */
	TEST_FEATURE ("with string array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_TYPE_ARRAY_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subsubiter);

			str_value = "this";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "is";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "a";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "test";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subsubiter);

			str_value = "and";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "this";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "is";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		str_array_array = NULL;

		ret = my_string_array_array_demarshal (NULL, message,
						       &str_array_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (str_array_array, NULL);
		TEST_ALLOC_SIZE (str_array_array, sizeof (char **) * 3);

		TEST_ALLOC_SIZE (str_array_array[0], sizeof (char *) * 5);
		TEST_ALLOC_PARENT (str_array_array[0], str_array_array);
		TEST_EQ_STR (str_array_array[0][0], "this");
		TEST_ALLOC_PARENT (str_array_array[0][0], str_array_array[0]);
		TEST_EQ_STR (str_array_array[0][1], "is");
		TEST_ALLOC_PARENT (str_array_array[0][1], str_array_array[0]);
		TEST_EQ_STR (str_array_array[0][2], "a");
		TEST_ALLOC_PARENT (str_array_array[0][2], str_array_array[0]);
		TEST_EQ_STR (str_array_array[0][3], "test");
		TEST_ALLOC_PARENT (str_array_array[0][3], str_array_array[0]);
		TEST_EQ_P (str_array_array[0][4], NULL);

		TEST_ALLOC_SIZE (str_array_array[1], sizeof (char *) * 4);
		TEST_ALLOC_PARENT (str_array_array[1], str_array_array);
		TEST_EQ_STR (str_array_array[1][0], "and");
		TEST_ALLOC_PARENT (str_array_array[1][0], str_array_array[1]);
		TEST_EQ_STR (str_array_array[1][1], "this");
		TEST_ALLOC_PARENT (str_array_array[1][1], str_array_array[1]);
		TEST_EQ_STR (str_array_array[1][2], "is");
		TEST_ALLOC_PARENT (str_array_array[1][2], str_array_array[1]);
		TEST_EQ_P (str_array_array[1][3], NULL);

		TEST_EQ_P (str_array_array[2], NULL);

		nih_free (str_array_array);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when an array of string arrays is expected, but a
	 * different type is found at the top-level, the type error code
	 * is run and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for string array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		str_array_array = NULL;

		ret = my_string_array_array_demarshal (NULL, message,
						       &str_array_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_array_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when an array of string arrays is expected, but a
	 * different type is found inside the array, the type error code
	 * is run and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type inside string array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		str_array_array = NULL;

		ret = my_string_array_array_demarshal (NULL, message,
						       &str_array_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_array_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when an array of string arrays is expected, but a
	 * different type is found deep inside the array, the type error code
	 * is run and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type deep inside string array array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_TYPE_ARRAY_AS_STRING
							   DBUS_TYPE_DOUBLE_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subsubiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		str_array_array = NULL;

		ret = my_string_array_array_demarshal (NULL, message,
						       &str_array_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (str_array_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Structure members into
	 * a newly allocated structure pointer is correctly generated and
	 * returned as an allocated string.  We expect a large number of
	 * locals since this is all done at one level.
	 */
	TEST_FEATURE ("with structure");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
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

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "struct", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a structure from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value = nih_new (parent, MyStructValue);\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"

				   "/* Demarshal a char * from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRING) {\n"
				   "\tnih_free (value);\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&value_iter, &value_item0_dbus);\n"
				   "\n"
				   "value_item0 = nih_strdup (value, value_item0_dbus);\n"
				   "if (! value_item0) {\n"
				   "\tnih_free (value);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "value->item0 = value_item0;\n"
				   "\n"

				   "/* Demarshal a uint32_t from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_UINT32) {\n"
				   "\tnih_free (value);\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&value_iter, &value_item1);\n"
				   "\n"
				   "dbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "value->item1 = value_item1;\n"
				   "\n"

				   "/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_ARRAY) {\n"
				   "\tnih_free (value);\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&value_iter, &value_item2_iter);\n"
				   "\n"
				   "value_item2_size = 0;\n"
				   "value_item2 = NULL;\n"
				   "\n"
				   "value_item2 = nih_alloc (value, sizeof (char *));\n"
				   "if (! value_item2) {\n"
				   "\tnih_free (value);\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value_item2[value_item2_size] = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_item2_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tconst char *value_item2_element_dbus;\n"
				   "\tchar **     value_item2_tmp;\n"
				   "\tchar *      value_item2_element;\n"
				   "\n"
				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_item2_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\tif (value_item2)\n"
				   "\t\t\tnih_free (value_item2);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_item2_iter, &value_item2_element_dbus);\n"
				   "\n"
				   "\tvalue_item2_element = nih_strdup (value_item2, value_item2_element_dbus);\n"
				   "\tif (! value_item2_element) {\n"
				   "\t\tif (value_item2)\n"
				   "\t\t\tnih_free (value_item2);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_item2_iter);\n"
				   "\n"
				   "\tif (value_item2_size + 2 > SIZE_MAX / sizeof (char *)) {\n"
				   "\t\tif (value_item2)\n"
				   "\t\t\tnih_free (value_item2);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_item2_tmp = nih_realloc (value_item2, value, sizeof (char *) * (value_item2_size + 2));\n"
				   "\tif (! value_item2_tmp) {\n"
				   "\t\tif (value_item2)\n"
				   "\t\t\tnih_free (value_item2);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_item2 = value_item2_tmp;\n"
				   "\tvalue_item2[value_item2_size] = value_item2_element;\n"
				   "\tvalue_item2[value_item2_size + 1] = NULL;\n"
				   "\n"
				   "\tvalue_item2_size++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "value->item2 = value_item2;\n"
				   "\n"

				   "/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_ARRAY) {\n"
				   "\tnih_free (value);\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&value_iter, &value_item3_iter);\n"
				   "\n"
				   "value_item3_len = 0;\n"
				   "value_item3 = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_item3_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tint16_t *value_item3_tmp;\n"
				   "\tint16_t  value_item3_element;\n"
				   "\n"
				   "\t/* Demarshal a int16_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_item3_iter) != DBUS_TYPE_INT16) {\n"
				   "\t\tif (value_item3)\n"
				   "\t\t\tnih_free (value_item3);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_item3_iter, &value_item3_element);\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_item3_iter);\n"
				   "\n"
				   "\tif (value_item3_len + 1 > SIZE_MAX / sizeof (int16_t)) {\n"
				   "\t\tif (value_item3)\n"
				   "\t\t\tnih_free (value_item3);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_item3_tmp = nih_realloc (value_item3, value, sizeof (int16_t) * (value_item3_len + 1));\n"
				   "\tif (! value_item3_tmp) {\n"
				   "\t\tif (value_item3)\n"
				   "\t\t\tnih_free (value_item3);\n"
				   "\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_item3 = value_item3_tmp;\n"
				   "\tvalue_item3[value_item3_len] = value_item3_element;\n"
				   "\n"
				   "\tvalue_item3_len++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "value->item3 = value_item3;\n"
				   "value->item3_len = value_item3_len;\n"
				   "\n"

				   "if (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tnih_free (value);\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "MyStructValue *");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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
		TEST_EQ_STR (var->name, "value_item0_dbus");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char *");
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
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_item2_size");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_NOT_EMPTY (&locals);

		var = (TypeVar *)locals.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "char **");
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
		TEST_EQ_STR (var->type, "int16_t *");
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


	/* Check that the generated code takes each of the values from the
	 * D-Bus Struct in the message we pass and stores them in a newly
	 * allocated structure in the pointer we provide.
	 */
	TEST_FEATURE ("with structure (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
							  NULL, &subiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 1818118181;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_STRING_AS_STRING,
							  &subsubiter);

			str_value = "premium";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "economy";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			str_value = "only";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_INT16_AS_STRING,
							  &subsubiter);

			int16_value = 4;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 8;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 15;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 16;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 23;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			int16_value = 42;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_INT16,
							&int16_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		struct_value = NULL;

		ret = my_struct_demarshal (NULL, message, &struct_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (struct_value, NULL);
		TEST_ALLOC_SIZE (struct_value, sizeof (MyStructValue));

		TEST_EQ_STR (struct_value->item0, "hello there");
		TEST_ALLOC_PARENT (struct_value->item0, struct_value);

		TEST_EQ (struct_value->item1, 1818118181);

		TEST_ALLOC_SIZE (struct_value->item2, sizeof (char *) * 4);
		TEST_ALLOC_PARENT (struct_value->item2, struct_value);
		TEST_EQ_STR (struct_value->item2[0], "premium");
		TEST_ALLOC_PARENT (struct_value->item2[0], struct_value->item2);
		TEST_EQ_STR (struct_value->item2[1], "economy");
		TEST_ALLOC_PARENT (struct_value->item2[1], struct_value->item2);
		TEST_EQ_STR (struct_value->item2[2], "only");
		TEST_ALLOC_PARENT (struct_value->item2[2], struct_value->item2);
		TEST_EQ_P (struct_value->item2[3], NULL);

		TEST_EQ (struct_value->item3_len, 6);
		TEST_ALLOC_SIZE (struct_value->item3, sizeof (int16_t) * 6);
		TEST_ALLOC_PARENT (struct_value->item3, struct_value);
		TEST_EQ (struct_value->item3[0], 4);
		TEST_EQ (struct_value->item3[1], 8);
		TEST_EQ (struct_value->item3[2], 15);
		TEST_EQ (struct_value->item3[3], 16);
		TEST_EQ (struct_value->item3[4], 23);
		TEST_EQ (struct_value->item3[5], 42);

		nih_free (struct_value);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a structure is expected, but a different type
	 * is found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for structure (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		struct_value = NULL;

		ret = my_struct_demarshal (NULL, message,
					   &struct_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (struct_value, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a structure member is expected, but a different
	 * member type is found, the type error code is run and the function
	 * returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for structure member (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
							  NULL, &subiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 1818118181;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
							&uint32_value);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		struct_value = NULL;

		ret = my_struct_demarshal (NULL, message,
					   &struct_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (struct_value, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when an item inside a complex structure member is
	 * expected, but a different type is found, the type error code
	 * is run and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type inside structure member (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT,
							  NULL, &subiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 1818118181;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subsubiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		struct_value = NULL;

		ret = my_struct_demarshal (NULL, message,
					   &struct_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (struct_value, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus Structure Array into
	 * a newly allocated structure array pointer is correctly generated
	 * and returned as an allocated string.  All of the struct locals
	 * should be internalised and just the array iterator and length
	 * as locals.
	 */
	TEST_FEATURE ("with structure array");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_UINT32_AS_STRING
					   DBUS_STRUCT_END_CHAR_AS_STRING));

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "struct_array", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value_size = 0;\n"
				   "value = NULL;\n"
				   "\n"
				   "value = nih_alloc (parent, sizeof (MyStructArrayValueElement *));\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value[value_size] = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tDBusMessageIter             value_element_iter;\n"
				   "\tconst char *                value_element_item0_dbus;\n"
				   "\tchar *                      value_element_item0;\n"
				   "\tuint32_t                    value_element_item1;\n"
				   "\tMyStructArrayValueElement **value_tmp;\n"
				   "\tMyStructArrayValueElement * value_element;\n"
				   "\n"

				   "\t/* Demarshal a structure from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_STRUCT) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&value_iter, &value_element_iter);\n"
				   "\n"
				   "\tvalue_element = nih_new (value, MyStructArrayValueElement);\n"
				   "\tif (! value_element) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"

				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_element_iter, &value_element_item0_dbus);\n"
				   "\n"
				   "\tvalue_element_item0 = nih_strdup (value_element, value_element_item0_dbus);\n"
				   "\tif (! value_element_item0) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_element_iter);\n"
				   "\n"
				   "\tvalue_element->item0 = value_element_item0;\n"
				   "\n"

				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_element_iter, &value_element_item1);\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_element_iter);\n"
				   "\n"
				   "\tvalue_element->item1 = value_element_item1;\n"
				   "\n"

				   "\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "\tif (value_size + 2 > SIZE_MAX / sizeof (MyStructArrayValueElement *)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_tmp = nih_realloc (value, parent, sizeof (MyStructArrayValueElement *) * (value_size + 2));\n"
				   "\tif (! value_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue = value_tmp;\n"
				   "\tvalue[value_size] = value_element;\n"
				   "\tvalue[value_size + 1] = NULL;\n"
				   "\n"
				   "\tvalue_size++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "MyStructArrayValueElement **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_size");
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


	/* Check that the generated code takes each of the members of the
	 * D-Bus Struct Array in the message we pass and stores them in a
	 * newly allocated structure array in the pointer we provide.
	 */
	TEST_FEATURE ("with structure array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_UINT32_AS_STRING
							   DBUS_STRUCT_END_CHAR_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_STRUCT,
							  NULL, &subsubiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 1818118181;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_STRUCT,
							  NULL, &subsubiter);

			str_value = "goodbye world";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 12345;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		struct_array = NULL;

		ret = my_struct_array_demarshal (NULL, message, &struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (struct_array, NULL);
		TEST_ALLOC_SIZE (struct_array, sizeof (MyStructArrayValueElement *) * 3);

		TEST_ALLOC_PARENT (struct_array[0], struct_array);
		TEST_ALLOC_SIZE (struct_array[0], sizeof (MyStructArrayValueElement));
		TEST_EQ_STR (struct_array[0]->item0, "hello there");
		TEST_ALLOC_PARENT (struct_array[0]->item0, struct_array[0]);
		TEST_EQ (struct_array[0]->item1, 1818118181);

		TEST_ALLOC_PARENT (struct_array[1], struct_array);
		TEST_ALLOC_SIZE (struct_array[1], sizeof (MyStructArrayValueElement));
		TEST_EQ_STR (struct_array[1]->item0, "goodbye world");
		TEST_ALLOC_PARENT (struct_array[1]->item0, struct_array[1]);
		TEST_EQ (struct_array[1]->item1, 12345);

		TEST_EQ_P (struct_array[2], NULL);

		nih_free (struct_array);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a structure array is expected, but a different
	 * type is found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for structure array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		struct_array = NULL;

		ret = my_struct_array_demarshal (NULL, message,
						 &struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (struct_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a structure array is expected, but a different
	 * type is found in the array, the type error code is run and
	 * the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong array member type for structure array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		struct_array = NULL;

		ret = my_struct_array_demarshal (NULL, message,
						 &struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (struct_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a structure array member is expected, but a
	 * different member type is found, the type error code is run
	 * and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for structure member (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_STRUCT_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_DOUBLE_AS_STRING
							   DBUS_STRUCT_END_CHAR_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_STRUCT,
							  NULL, &subsubiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		struct_array = NULL;

		ret = my_struct_array_demarshal (NULL, message,
						 &struct_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (struct_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus DictEntry Array into
	 * a newly allocated dict entry array pointer is correctly generated
	 * and returned as an allocated string.  All of the struct locals
	 * should be internalised and just the array iterator and length
	 * as locals.
	 */
	TEST_FEATURE ("with dict entry array");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  (DBUS_TYPE_ARRAY_AS_STRING
					   DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					   DBUS_TYPE_STRING_AS_STRING
					   DBUS_TYPE_UINT32_AS_STRING
					   DBUS_DICT_ENTRY_END_CHAR_AS_STRING));

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "dict_entry_array", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal an array from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_recurse (&iter, &value_iter);\n"
				   "\n"
				   "value_size = 0;\n"
				   "value = NULL;\n"
				   "\n"
				   "value = nih_alloc (parent, sizeof (MyDictEntryArrayValueElement *));\n"
				   "if (! value) {\n"
				   "\treturn -1;\n"
				   "}\n"
				   "\n"
				   "value[value_size] = NULL;\n"
				   "\n"
				   "while (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_INVALID) {\n"
				   "\tDBusMessageIter                value_element_iter;\n"
				   "\tconst char *                   value_element_item0_dbus;\n"
				   "\tchar *                         value_element_item0;\n"
				   "\tuint32_t                       value_element_item1;\n"
				   "\tMyDictEntryArrayValueElement **value_tmp;\n"
				   "\tMyDictEntryArrayValueElement * value_element;\n"
				   "\n"

				   "\t/* Demarshal a structure from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_iter) != DBUS_TYPE_DICT_ENTRY) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_recurse (&value_iter, &value_element_iter);\n"
				   "\n"
				   "\tvalue_element = nih_new (value, MyDictEntryArrayValueElement);\n"
				   "\tif (! value_element) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"

				   "\t/* Demarshal a char * from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_STRING) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_element_iter, &value_element_item0_dbus);\n"
				   "\n"
				   "\tvalue_element_item0 = nih_strdup (value_element, value_element_item0_dbus);\n"
				   "\tif (! value_element_item0) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_element_iter);\n"
				   "\n"
				   "\tvalue_element->item0 = value_element_item0;\n"
				   "\n"

				   "\t/* Demarshal a uint32_t from the message */\n"
				   "\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_UINT32) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_get_basic (&value_element_iter, &value_element_item1);\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_element_iter);\n"
				   "\n"
				   "\tvalue_element->item1 = value_element_item1;\n"
				   "\n"

				   "\tif (dbus_message_iter_get_arg_type (&value_element_iter) != DBUS_TYPE_INVALID) {\n"
				   "\t\tnih_free (value_element);\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tdbus_message_iter_next (&value_iter);\n"
				   "\n"
				   "\tif (value_size + 2 > SIZE_MAX / sizeof (MyDictEntryArrayValueElement *)) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn 1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue_tmp = nih_realloc (value, parent, sizeof (MyDictEntryArrayValueElement *) * (value_size + 2));\n"
				   "\tif (! value_tmp) {\n"
				   "\t\tif (value)\n"
				   "\t\t\tnih_free (value);\n"
				   "\t\treturn -1;\n"
				   "\t}\n"
				   "\n"
				   "\tvalue = value_tmp;\n"
				   "\tvalue[value_size] = value_element;\n"
				   "\tvalue[value_size + 1] = NULL;\n"
				   "\n"
				   "\tvalue_size++;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "MyDictEntryArrayValueElement **");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

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
		TEST_EQ_STR (var->type, "size_t");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value_size");
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


	/* Check that the generated code takes each of the members of the
	 * D-Bus DictEntry Array in the message we pass and stores them in a
	 * newly allocated dict entry array in the pointer we provide.
	 */
	TEST_FEATURE ("with dict entry array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_UINT32_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &subsubiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 1818118181;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &subsubiter);

			str_value = "goodbye world";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			uint32_value = 12345;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_UINT32,
							&uint32_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		dict_entry_array = NULL;

		ret = my_dict_entry_array_demarshal (NULL, message,
						     &dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_ALLOC_PARENT (dict_entry_array, NULL);
		TEST_ALLOC_SIZE (dict_entry_array, sizeof (MyDictEntryArrayValueElement *) * 3);

		TEST_ALLOC_PARENT (dict_entry_array[0], dict_entry_array);
		TEST_ALLOC_SIZE (dict_entry_array[0], sizeof (MyDictEntryArrayValueElement));
		TEST_EQ_STR (dict_entry_array[0]->item0, "hello there");
		TEST_ALLOC_PARENT (dict_entry_array[0]->item0, dict_entry_array[0]);
		TEST_EQ (dict_entry_array[0]->item1, 1818118181);

		TEST_ALLOC_PARENT (dict_entry_array[1], dict_entry_array);
		TEST_ALLOC_SIZE (dict_entry_array[1], sizeof (MyDictEntryArrayValueElement));
		TEST_EQ_STR (dict_entry_array[1]->item0, "goodbye world");
		TEST_ALLOC_PARENT (dict_entry_array[1]->item0, dict_entry_array[1]);
		TEST_EQ (dict_entry_array[1]->item1, 12345);

		TEST_EQ_P (dict_entry_array[2], NULL);

		nih_free (dict_entry_array);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a dict entry array is expected, but a different
	 * type is found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for dict entry array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		dict_entry_array = NULL;

		ret = my_dict_entry_array_demarshal (NULL, message,
						     &dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (dict_entry_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a dict entry array is expected, but a different
	 * type is found in the array, the type error code is run and
	 * the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong array member type for dict entry array (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  DBUS_TYPE_DOUBLE_AS_STRING,
							  &subiter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		dict_entry_array = NULL;

		ret = my_dict_entry_array_demarshal (NULL, message,
						     &dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (dict_entry_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that when a dict entry array member is expected, but a
	 * different member type is found, the type error code is run
	 * and the function returns without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for dict entry member (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
							  (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
							   DBUS_TYPE_STRING_AS_STRING
							   DBUS_TYPE_DOUBLE_AS_STRING
							   DBUS_DICT_ENTRY_END_CHAR_AS_STRING),
							  &subiter);

			dbus_message_iter_open_container (&subiter, DBUS_TYPE_DICT_ENTRY,
							  NULL, &subsubiter);

			str_value = "hello there";
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_STRING,
							&str_value);

			double_value = 3.14;
			dbus_message_iter_append_basic (&subsubiter, DBUS_TYPE_DOUBLE,
							&double_value);

			dbus_message_iter_close_container (&subiter, &subsubiter);

			dbus_message_iter_close_container (&iter, &subiter);
		}

		dict_entry_array = NULL;

		ret = my_dict_entry_array_demarshal (NULL, message,
						     &dict_entry_array);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ_P (dict_entry_array, NULL);

		dbus_message_unref (message);

		dbus_shutdown ();
	}


	/* Check that the code to demarshal a D-Bus file descriptor into an
	 * int is correctly generated and returned as an allocated string.
	 */
	TEST_FEATURE ("with file descriptor");
	TEST_ALLOC_FAIL {
		nih_list_init (&outputs);
		nih_list_init (&locals);
		nih_list_init (&structs);

		dbus_signature_iter_init (&signature,
					  DBUS_TYPE_UNIX_FD_AS_STRING);

		str = demarshal (NULL, &signature,
				 "parent", "iter", "value",
				 "return -1;\n",
				 "return 1;\n",
				 &outputs, &locals,
				 "my", NULL, "unix_fd", "value",
				 &structs);

		if (test_alloc_failed) {
			TEST_EQ_P (str, NULL);
			TEST_LIST_EMPTY (&outputs);
			TEST_LIST_EMPTY (&locals);
			TEST_LIST_EMPTY (&structs);
			continue;
		}

		TEST_EQ_STR (str, ("/* Demarshal a int from the message */\n"
				   "if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UNIX_FD) {\n"
				   "\treturn 1;\n"
				   "}\n"
				   "\n"
				   "dbus_message_iter_get_basic (&iter, &value);\n"
				   "\n"
				   "dbus_message_iter_next (&iter);\n"));

		TEST_LIST_NOT_EMPTY (&outputs);

		var = (TypeVar *)outputs.next;
		TEST_ALLOC_SIZE (var, sizeof (TypeVar));
		TEST_ALLOC_PARENT (var, str);
		TEST_EQ_STR (var->type, "int");
		TEST_ALLOC_PARENT (var->type, var);
		TEST_EQ_STR (var->name, "value");
		TEST_ALLOC_PARENT (var->name, var);
		nih_free (var);

		TEST_LIST_EMPTY (&outputs);

		TEST_LIST_EMPTY (&locals);

		TEST_LIST_EMPTY (&structs);

		nih_free (str);
	}


	/* Check that the generated code takes the value from the D-Bus
	 * file descriptor in the message we pass and stores it in the int
	 * pointer, which should have the right value.
	 */
	TEST_FEATURE ("with file descriptor (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			unix_fd_value = 1;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_UNIX_FD,
							&unix_fd_value);
		}

		unix_fd_value = -1;

		ret = my_unix_fd_demarshal (NULL, message, &unix_fd_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_EQ (ret, 0);
		TEST_GT (unix_fd_value, 2); // duplicated by dbus

		dbus_message_unref (message);
		close (unix_fd_value);

		dbus_shutdown ();
	}


	/* Check that when a file descriptor is expected, but a different
	 * type is found, the type error code is run and the function returns
	 * without modifying the pointer.
	 */
	TEST_FEATURE ("with wrong type for file descriptor (generated code)");
	TEST_ALLOC_FAIL {
		TEST_ALLOC_SAFE {
			message = dbus_message_new (DBUS_MESSAGE_TYPE_METHOD_CALL);

			dbus_message_iter_init_append (message, &iter);

			double_value = 3.14;
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_DOUBLE,
							&double_value);
		}

		unix_fd_value = -1;

		ret = my_unix_fd_demarshal (NULL, message, &unix_fd_value);

		if (test_alloc_failed) {
			TEST_LT (ret, 0);

			dbus_message_unref (message);
			dbus_shutdown ();
			continue;
		}

		TEST_GT (ret, 0);
		TEST_EQ (unix_fd_value, -1);

		dbus_message_unref (message);

		dbus_shutdown ();
	}
}


int
main (int   argc,
      char *argv[])
{
	test_demarshal ();

	return 0;
}
