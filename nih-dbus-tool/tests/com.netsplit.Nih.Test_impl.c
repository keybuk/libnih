/* libnih
 *
 * com.netsplit.Nih.Test_impl.c - implementation of test object interfaces
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/main.h>
#include <nih/logging.h>
#include <nih/error.h>

#include <nih-dbus/dbus_error.h>
#include <nih-dbus/dbus_message.h>

#include "tests/com.netsplit.Nih.Test_object.h"
#include "tests/com.netsplit.Nih.Test_impl.h"


int
my_test_ordinary_method (void *          data,
			 NihDBusMessage *message,
			 const char *    input,
			 char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.OrdinaryMethod.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_strdup (message, input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_nameless_method (void *          data,
			 NihDBusMessage *message,
			 const char *    arg1,
			 char **         arg2)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (arg1, NULL);
	TEST_NE_P (arg2, NULL);

	if (! strlen (arg1)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.NamelessMethod.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (arg1, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*arg2 = nih_strdup (message, arg1);
	if (! *arg2)
		nih_return_no_memory_error (-1);

	return 0;
}

int             async_method_main_loop;
char *          async_method_input;
NihDBusMessage *async_method_message;

static void
my_test_async_method_send_reply (void *           data,
				 NihMainLoopFunc *loop)
{
	NIH_ZERO (my_test_async_method_reply (async_method_message,
					      async_method_input));

	nih_free (async_method_message);
	nih_free (async_method_input);

	async_method_input = NULL;
	async_method_message = NULL;

	nih_free (loop);
}

int
my_test_async_method (void *          data,
		      NihDBusMessage *message,
		      const char *    input)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.AsyncMethod.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (async_method_input)
		nih_free (async_method_input);

	async_method_input = nih_strdup (NULL, input);
	if (! async_method_input)
		nih_return_no_memory_error (-1);

	async_method_message = message;
	nih_ref (async_method_message, async_method_input);

	if (async_method_main_loop)
		NIH_MUST (nih_main_loop_add_func (NULL, my_test_async_method_send_reply,
						  NULL));

	return 0;
}


int
my_test_byte_to_str (void *          data,
		     NihDBusMessage *message,
		     uint8_t         input,
		     char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.ByteToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%hhu", (unsigned char)input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_byte (void *          data,
		     NihDBusMessage *message,
		     const char *    input,
		     uint8_t *       output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToByte.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (uint8_t)strtoumax (input, NULL, 10);

	return 0;
}


int
my_test_boolean_to_str (void *          data,
			NihDBusMessage *message,
			int             input,
			char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	/* Yes, this is just wrong, but D-Bus sanitises booleans for us over
	 * the wire so we can only receive TRUE or FALSE.
	 */
	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.BooleanToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	}

	*output = nih_strdup (message, input ? "True" : "False");
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_boolean (void *          data,
			NihDBusMessage *message,
			const char *    input,
			int *           output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToBoolean.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (! strcmp (input, "False")) {
		*output = FALSE;
	} else {
		*output = TRUE;
	}

	return 0;
}


int
my_test_int16_to_str (void *          data,
		      NihDBusMessage *message,
		      int16_t         input,
		      char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int16ToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%hd", (short)input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_int16 (void *          data,
		      NihDBusMessage *message,
		      const char *    input,
		      int16_t *       output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToInt16.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (int16_t)strtoimax (input, NULL, 10);

	return 0;
}


int
my_test_uint16_to_str (void *          data,
		       NihDBusMessage *message,
		       uint16_t        input,
		       char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt16ToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%hu", (unsigned short)input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_uint16 (void *          data,
		       NihDBusMessage *message,
		       const char *    input,
		       uint16_t *      output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToUInt16.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (uint16_t)strtoumax (input, NULL, 10);

	return 0;
}


int
my_test_int32_to_str (void *          data,
		      NihDBusMessage *message,
		      int32_t         input,
		      char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32ToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = NIH_MUST (nih_sprintf (message, "%d", (int)input));
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_int32 (void *          data,
		      NihDBusMessage *message,
		      const char *    input,
		      int32_t *       output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToInt32.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (int32_t)strtoimax (input, NULL, 10);

	return 0;
}


int
my_test_uint32_to_str (void *          data,
		       NihDBusMessage *message,
		       uint32_t        input,
		       char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt32ToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%u", (unsigned int)input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_uint32 (void *          data,
		       NihDBusMessage *message,
		       const char *    input,
		       uint32_t *      output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToUInt32.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (uint32_t)strtoumax (input, NULL, 10);

	return 0;
}


int
my_test_int64_to_str (void *          data,
		      NihDBusMessage *message,
		      int64_t         input,
		      char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int64ToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%lld", (long long)input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_int64 (void *          data,
		      NihDBusMessage *message,
		      const char *    input,
		      int64_t *       output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToInt64.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (int64_t)strtoimax (input, NULL, 10);

	return 0;
}


int
my_test_uint64_to_str (void *          data,
		       NihDBusMessage *message,
		       uint64_t        input,
		       char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt64ToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%llu", (unsigned long long)input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_uint64 (void *          data,
		       NihDBusMessage *message,
		       const char *    input,
		       uint64_t *      output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToUInt64.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = (uint64_t)strtoumax (input, NULL, 10);

	return 0;
}


int
my_test_double_to_str (void *          data,
		       NihDBusMessage *message,
		       double          input,
		       char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	if (! input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.DoubleToStr.ZeroInput",
				      "The input argument was zero");
		return -1;
	} else if (input == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%f", input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_double (void *          data,
		       NihDBusMessage *message,
		       const char *    input,
		       double *        output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToDouble.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = strtod (input, NULL);

	return 0;
}


int
my_test_object_path_to_str (void *          data,
			    NihDBusMessage *message,
			    const char *    input,
			    char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strcmp (input, "/")) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.ObjectPathToStr.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "/invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_strdup (message, input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_object_path (void *          data,
			    NihDBusMessage *message,
			    const char *    input,
			    char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToObjectPath.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_strdup (message, input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}


int
my_test_signature_to_str (void *          data,
			  NihDBusMessage *message,
			  const char *    input,
			  char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.SignatureToStr.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "inva(x)id")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_strdup (message, input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_signature (void *          data,
			  NihDBusMessage *message,
			  const char *    input,
			  char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToSignature.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_strdup (message, input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}


int
my_test_struct_to_str (void *                        data,
		       NihDBusMessage *              message,
		       const MyTestStructToStrInput *input,
		       char **                       output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_ALLOC_PARENT (input, message);
	TEST_NE_P (input->item0, NULL);
	TEST_ALLOC_PARENT (input->item0, input);

	TEST_NE_P (output, NULL);

	if (! strlen (input->item0)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StructToStr.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input->item0, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_sprintf (message, "%s %u", input->item0,
			       (unsigned int)input->item1);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_struct (void *                    data,
		       NihDBusMessage *          message,
		       const char *              input,
		       MyTestStrToStructOutput **output)
{
	char         item0_value[512];
	unsigned int item1_value;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_ALLOC_PARENT (input, message);

	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToStruct.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	sscanf (input, "%s %d", item0_value, &item1_value);

	*output = nih_new (message, MyTestStrToStructOutput);
	if (! *output)
		nih_return_no_memory_error (-1);

	(*output)->item0 = nih_strdup (*output, item0_value);
	if (! (*output)->item0) {
		nih_error_raise_no_memory ();
		nih_free (*output);
		*output = NULL;
		return -1;
	}

	(*output)->item1 = item1_value;

	return 0;
}


int
my_test_int32_array_to_str (void *          data,
			    NihDBusMessage *message,
			    int32_t *       input,
			    size_t          input_len,
			    char **         output)
{
	char *str;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (input_len)
		TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! input_len) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32ArrayToStr.EmptyInput",
				      "The input array was empty");
		return -1;
	} else if (input_len == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	str = NULL;

	for (size_t i = 0; i < input_len; i++) {
		if (! nih_strcat_sprintf (&str, message, i ? " %d" : "%d",
					  (int)input[i])) {
			nih_error_raise_no_memory ();
			if (str)
				nih_free (str);
			return -1;
		}
	}

	*output = str;

	return 0;
}

int
my_test_str_to_int32_array (void *          data,
			    NihDBusMessage *message,
			    const char *    input,
			    int32_t **      output,
			    size_t *        output_len)
{
	nih_local char **parts = NULL;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToInt32Array.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = NULL;
	*output_len = 0;

	parts = nih_str_split (NULL, input, " ", FALSE);
	if (! parts)
		nih_return_no_memory_error (-1);

	for (char **part = parts; part && *part; part++) {
		int32_t *tmp;

		tmp = nih_realloc (*output, message,
				   sizeof (int32_t) * (*output_len + 1));
		if (! tmp) {
			nih_error_raise_no_memory ();
			if (*output) {
				nih_free (*output);
				*output = NULL;
			}
			return -1;
		}

		*output = tmp;
		(*output)[(*output_len)++] = (int32_t)strtoimax (*part, NULL, 10);
	}

	return 0;
}


int
my_test_str_array_to_str (void *          data,
			  NihDBusMessage *message,
			  char * const *  input,
			  char **         output)
{
	char *str;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! *input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrArrayToStr.EmptyInput",
				      "The input array was empty");
		return -1;
	} else if (input[0] && input[1] && input[2] && input[3]
		   && (! input[4])) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	str = NULL;

	for (char * const *s = input; s && *s; s++) {
		if (! nih_strcat_sprintf (&str, message, str ? " %s" : "%s",
					  *s)) {
			nih_error_raise_no_memory ();
			if (str)
				nih_free (str);
			return -1;
		}
	}

	*output = str;

	return 0;
}

int
my_test_str_to_str_array (void *          data,
			  NihDBusMessage *message,
			  const char *    input,
			  char ***        output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToStrArray.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = nih_str_split (message, input, " ", FALSE);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}


int
my_test_int32_array_array_to_str (void *          data,
				  NihDBusMessage *message,
				  int32_t **      input,
				  size_t *        input_len,
				  char **         output)
{
	char *  str;
	size_t *array_len;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! *input) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32ArrayArrayToStr.EmptyInput",
				      "The input array was empty");
		return -1;
	} else if (input[0] && (! input[1])) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	str = NULL;

	array_len = input_len;
	for (int32_t **array = input; array && *array; array++) {
		for (size_t i = 0; i < *array_len; i++) {
			if (! nih_strcat_sprintf (&str, message,
						  i ? " %d" : (str ? "\n%d" : "%d"),
						  (int)(*array)[i])) {
				nih_error_raise_no_memory ();
				if (str)
					nih_free (str);
				return -1;
			}
		}
	}

	*output = str;

	return 0;
}

int
my_test_str_to_int32_array_array (void *          data,
				  NihDBusMessage *message,
				  const char *    input,
				  int32_t ***     output,
				  size_t **       output_len)
{
	nih_local char **lines = NULL;
	size_t           output_size = 0;
	int32_t **       tmp;
	size_t *         tmp_len;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToInt32ArrayArray.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = NULL;
	*output_len = NULL;
	output_size = 0;

	lines = nih_str_split (NULL, input, "\n", FALSE);
	if (! lines)
		nih_return_no_memory_error (-1);

	for (char **line = lines; line && *line; line++) {
		nih_local char **parts = NULL;
		size_t           array_size;

		tmp = nih_realloc (*output, message,
				   sizeof (int32_t *) * (output_size + 1));
		if (! tmp)
			goto error;

		*output = tmp;
		(*output)[output_size] = NULL;

		tmp_len = nih_realloc (*output_len, message,
				       sizeof (size_t) * (output_size + 1));
		if (! tmp_len)
			goto error;

		*output_len = tmp_len;

		parts = nih_str_split (NULL, *line, " ", FALSE);
		if (! parts)
			goto error;

		array_size = 0;
		for (char **part = parts; part && *part; part++) {
			int32_t *tmp;

			tmp = nih_realloc ((*output)[output_size], message,
					   sizeof (int32_t) * (array_size + 1));
			if (! tmp)
				goto error;

			(*output)[output_size] = tmp;
			(*output)[output_size][array_size++] \
				= (int32_t)strtoimax (*part, NULL, 10);
		}

		(*output_len)[output_size++] = array_size;
	}

	tmp = nih_realloc (*output, message,
			   sizeof (int32_t *) * (output_size + 1));
	if (! tmp)
		goto error;

	*output = tmp;
	(*output)[output_size] = NULL;

	tmp_len = nih_realloc (*output_len, message,
			       sizeof (size_t) * (output_size + 1));
	if (! tmp_len)
		goto error;

	*output_len = tmp_len;
	(*output_len)[output_size] = 0;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (*output) {
		nih_free (*output);
		*output = NULL;
	}
	if (*output_len) {
		nih_free (*output_len);
		*output_len = NULL;
	}
	return -1;
}


int
my_test_struct_array_to_str (void *            data,
			     NihDBusMessage   *message,
			     MyTestStructArrayToStrInputElement * const *input,
			     char **           output)
{
	char *str;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_ALLOC_PARENT (input, message);

	TEST_NE_P (output, NULL);

	if (! input[0]) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StructArrayToStr.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! input[1]) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	str = NULL;

	for (MyTestStructArrayToStrInputElement * const *element = input;
	     element && *element; element++) {
		if (! nih_strcat_sprintf (&str, message,
					  str ? "\n%s %u" : "%s %u",
					  (*element)->item0,
					  (unsigned int)(*element)->item1)) {
			nih_error_raise_no_memory ();
			if (str)
				nih_free (str);
			return -1;
		}
	}

	*output = str;

	return 0;
}

int
my_test_str_to_struct_array (void *          data,
			     NihDBusMessage *message,
			     const char *    input,
			     MyTestStrToStructArrayOutputElement ***output)
{
	MyTestStrToStructArrayOutputElement **tmp;
	MyTestStrToStructArrayOutputElement *element;
	nih_local char **lines = NULL;
	size_t           output_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_ALLOC_PARENT (input, message);

	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToStructArray.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = NULL;
	output_size = 0;

	lines = nih_str_split (NULL, input, "\n", FALSE);
	if (! lines)
		nih_return_no_memory_error (-1);

	for (char **line = lines; line && *line; line++) {
		char     item0_value[512];
		uint32_t item1_value;

		tmp = nih_realloc (*output, message,
				   sizeof (MyTestStrToStructArrayOutputElement *) * (output_size + 1));
		if (! tmp)
			goto error;

		*output = tmp;

		sscanf (*line, "%s %d", item0_value, &item1_value);

		element = nih_new (*output, MyTestStrToStructArrayOutputElement);
		if (! element)
			goto error;

		element->item0 = nih_strdup (element, item0_value);
		if (! element->item0)
			goto error;

		element->item1 = item1_value;

		(*output)[output_size++] = element;
	}

	tmp = nih_realloc (*output, message,
			   sizeof (MyTestStrToStructArrayOutputElement *) * (output_size + 1));
	if (! tmp)
		goto error;

	*output = tmp;
	(*output)[output_size++] = NULL;;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (*output) {
		nih_free (*output);
		*output = NULL;
	}
	return -1;
}


int
my_test_dict_entry_array_to_str (void *            data,
				 NihDBusMessage   *message,
				 MyTestDictEntryArrayToStrInputElement * const *input,
				 char **           output)
{
	char *str;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_ALLOC_PARENT (input, message);

	TEST_NE_P (output, NULL);

	if (! input[0]) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.DictEntryArrayToStr.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! input[1]) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	str = NULL;

	for (MyTestDictEntryArrayToStrInputElement * const *element = input;
	     element && *element; element++) {
		if (! nih_strcat_sprintf (&str, message,
					  str ? "\n%s %u" : "%s %u",
					  (*element)->item0,
					  (unsigned int)(*element)->item1)) {
			nih_error_raise_no_memory ();
			if (str)
				nih_free (str);
			return -1;
		}
	}

	*output = str;

	return 0;
}

int
my_test_str_to_dict_entry_array (void *          data,
				 NihDBusMessage *message,
				 const char *    input,
				 MyTestStrToDictEntryArrayOutputElement ***output)
{
	MyTestStrToDictEntryArrayOutputElement **tmp;
	MyTestStrToDictEntryArrayOutputElement *element;
	nih_local char **lines = NULL;
	size_t           output_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_ALLOC_PARENT (input, message);

	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToDictEntryArray.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = NULL;
	output_size = 0;

	lines = nih_str_split (NULL, input, "\n", FALSE);
	if (! lines)
		nih_return_no_memory_error (-1);

	for (char **line = lines; line && *line; line++) {
		char     item0_value[512];
		uint32_t item1_value;

		tmp = nih_realloc (*output, message,
				   sizeof (MyTestStrToDictEntryArrayOutputElement *) * (output_size + 1));
		if (! tmp)
			goto error;

		*output = tmp;

		sscanf (*line, "%s %d", item0_value, &item1_value);

		element = nih_new (*output, MyTestStrToDictEntryArrayOutputElement);
		if (! element)
			goto error;

		element->item0 = nih_strdup (element, item0_value);
		if (! element->item0)
			goto error;

		element->item1 = item1_value;

		(*output)[output_size++] = element;
	}

	tmp = nih_realloc (*output, message,
			   sizeof (MyTestStrToDictEntryArrayOutputElement *) * (output_size + 1));
	if (! tmp)
		goto error;

	*output = tmp;
	(*output)[output_size++] = NULL;;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (*output) {
		nih_free (*output);
		*output = NULL;
	}
	return -1;
}


int
my_test_unix_fd_to_str (void *          data,
			NihDBusMessage *message,
			int             input,
			char **         output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (output, NULL);

	/* We don't care, we just don't want to leak */
	close (input);

	*output = nih_sprintf (message, "%d", input);
	if (! *output)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_str_to_unix_fd (void *          data,
			NihDBusMessage *message,
			const char *    input,
			int *           output)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strlen (input)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrToUnixFd.EmptyInput",
				      "The input argument was empty");
		return -1;
	} else if (! strcmp (input, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*output = atoi (input);

	return 0;
}




uint8_t byte_property;

int
my_test_get_byte (void *          data,
		  NihDBusMessage *message,
		  uint8_t *       value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! byte_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Byte.Zero",
				      "The property value was zero");
		return -1;
	} else if (byte_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = byte_property;

	return 0;
}

int
my_test_set_byte (void *          data,
		  NihDBusMessage *message,
		  uint8_t         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Byte.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	byte_property = value;

	return 0;
}


int boolean_property;

int
my_test_get_boolean (void *          data,
		     NihDBusMessage *message,
		     int *           value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	/* Yes, this is just wrong, but D-Bus sanitises booleans for us over
	 * the wire so we can only receive TRUE or FALSE.
	 */
	if (! boolean_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Boolean.Zero",
				      "The property value was zero");
		return -1;
	}

	*value = boolean_property;

	return 0;
}

int
my_test_set_boolean (void *          data,
		     NihDBusMessage *message,
		     int             value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	/* Yes, this is just wrong, but D-Bus sanitises booleans for us over
	 * the wire so we can only receive TRUE or FALSE.
	 */
	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Boolean.Zero",
				      "The property value was zero");
		return -1;
	}

	boolean_property = value;

	return 0;
}


int16_t int16_property;

int
my_test_get_int16 (void *          data,
		   NihDBusMessage *message,
		   int16_t *       value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! int16_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int16.Zero",
				      "The property value was zero");
		return -1;
	} else if (int16_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = int16_property;

	return 0;
}

int
my_test_set_int16 (void *          data,
		   NihDBusMessage *message,
		   int16_t         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int16.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	int16_property = value;

	return 0;
}


uint16_t uint16_property;

int
my_test_get_uint16 (void *          data,
		    NihDBusMessage *message,
		    uint16_t *      value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! uint16_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt16.Zero",
				      "The property value was zero");
		return -1;
	} else if (uint16_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = uint16_property;

	return 0;
}

int
my_test_set_uint16 (void *          data,
		    NihDBusMessage *message,
		    uint16_t        value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt16.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	uint16_property = value;

	return 0;
}


int32_t int32_property;

int
my_test_get_int32 (void *          data,
		   NihDBusMessage *message,
		   int32_t *       value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! int32_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32.Zero",
				      "The property value was zero");
		return -1;
	} else if (int32_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = int32_property;

	return 0;
}

int
my_test_set_int32 (void *          data,
		   NihDBusMessage *message,
		   int32_t         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	int32_property = value;

	return 0;
}


uint32_t uint32_property;

int
my_test_get_uint32 (void *          data,
		    NihDBusMessage *message,
		    uint32_t *      value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! uint32_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt32.Zero",
				      "The property value was zero");
		return -1;
	} else if (uint32_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = uint32_property;

	return 0;
}

int
my_test_set_uint32 (void *          data,
		    NihDBusMessage *message,
		    uint32_t        value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt32.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	uint32_property = value;

	return 0;
}


int64_t int64_property;

int
my_test_get_int64 (void *          data,
		   NihDBusMessage *message,
		   int64_t *       value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! int64_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int64.Zero",
				      "The property value was zero");
		return -1;
	} else if (int64_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = int64_property;

	return 0;
}

int
my_test_set_int64 (void *          data,
		   NihDBusMessage *message,
		   int64_t         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int64.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	int64_property = value;

	return 0;
}


uint64_t uint64_property;

int
my_test_get_uint64 (void *          data,
		    NihDBusMessage *message,
		    uint64_t *      value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! uint64_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt64.Zero",
				      "The property value was zero");
		return -1;
	} else if (uint64_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = uint64_property;

	return 0;
}

int
my_test_set_uint64 (void *          data,
		    NihDBusMessage *message,
		    uint64_t        value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UInt64.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	uint64_property = value;

	return 0;
}


double double_property;

int
my_test_get_dubble (void *          data,
		    NihDBusMessage *message,
		    double *        value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! double_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Double.Zero",
				      "The property value was zero");
		return -1;
	} else if (double_property == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = double_property;

	return 0;
}

int
my_test_set_dubble (void *          data,
		    NihDBusMessage *message,
		    double          value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (! value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Double.Zero",
				      "The property value was zero");
		return -1;
	} else if (value == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	double_property = value;

	return 0;
}


char *str_property;

int
my_test_get_string (void *          data,
		    NihDBusMessage *message,
		    char **         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strlen (str_property)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.String.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (str_property, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = nih_strdup (message, str_property);
	if (! *value)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_set_string (void *          data,
		    NihDBusMessage *message,
		    const char *    value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strlen (value)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.String.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (value, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (str_property)
		nih_free (str_property);

	str_property = nih_strdup (NULL, value);
	if (! str_property)
		nih_return_no_memory_error (-1);

	return 0;
}


char *object_path_property;

int
my_test_get_object_path (void *          data,
			 NihDBusMessage *message,
			 char **         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strcmp (object_path_property, "/")) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.ObjectPath.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (object_path_property, "/invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = nih_strdup (message, object_path_property);
	if (! *value)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_set_object_path (void *          data,
			 NihDBusMessage *message,
			 const char *    value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strcmp (value, "/")) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.ObjectPath.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (value, "/invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (object_path_property)
		nih_free (object_path_property);

	object_path_property = nih_strdup (NULL, value);
	if (! object_path_property)
		nih_return_no_memory_error (-1);

	return 0;
}


char *signature_property;

int
my_test_get_signature (void *          data,
		       NihDBusMessage *message,
		       char **         value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strlen (signature_property)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Signature.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (signature_property, "inva(x)id")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = nih_strdup (message, signature_property);
	if (! *value)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_set_signature (void *          data,
		       NihDBusMessage *message,
		       const char *    value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strlen (value)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Signature.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (value, "inva(x)id")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (signature_property)
		nih_free (signature_property);

	signature_property = nih_strdup (NULL, value);
	if (! signature_property)
		nih_return_no_memory_error (-1);

	return 0;
}


MyStruct *struct_property;

int
my_test_get_structure (void *            data,
		       NihDBusMessage *  message,
		       MyTestStructure **value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strlen (struct_property->item0)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Structure.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (struct_property->item0, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = nih_new (message, MyTestStructure);
	if (! *value)
		nih_return_no_memory_error (-1);

	(*value)->item0 = nih_strdup (*value, struct_property->item0);
	if (! (*value)->item0) {
		nih_error_raise_no_memory ();
		nih_free (*value);
		return -1;
	}

	(*value)->item1 = struct_property->item1;

	return 0;
}

int
my_test_set_structure (void *                 data,
		       NihDBusMessage *       message,
		       const MyTestStructure *value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! strlen (value->item0)) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Structure.Empty",
				      "The property value was empty");
		return -1;
	} else if (! strcmp (value->item0, "invalid")) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (struct_property)
		nih_free (struct_property);

	struct_property = nih_new (NULL, MyStruct);
	if (! struct_property)
		nih_return_no_memory_error (-1);

	struct_property->item0 = nih_strdup (struct_property, value->item0);
	if (! struct_property->item0) {
		nih_error_raise_no_memory ();
		nih_free (struct_property);
		struct_property = NULL;
		return -1;
	}

	struct_property->item1 = value->item1;

	return 0;
}


int32_t *int32_array_property;
size_t   int32_array_property_len;

int
my_test_get_int32_array (void *          data,
			 NihDBusMessage *message,
			 int32_t **      value,
			 size_t *        value_len)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_NE_P (value_len, NULL);

	if (! int32_array_property_len) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32Array.Empty",
				      "The property value was empty");
		return -1;
	} else if (int32_array_property_len == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = nih_alloc (message,
			    sizeof (int32_t) * int32_array_property_len);
	if (! *value)
		nih_return_no_memory_error (-1);

	memcpy (*value, int32_array_property,
		sizeof (int32_t) * int32_array_property_len);
	*value_len = int32_array_property_len;

	return 0;
}

int
my_test_set_int32_array (void *          data,
			 NihDBusMessage *message,
			 const int32_t * value,
			 size_t          value_len)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	if (value_len)
		TEST_NE_P (value, NULL);

	if (! value_len) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32Array.Empty",
				      "The property value was empty");
		return -1;
	} else if (value_len == 4) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (int32_array_property)
		nih_free (int32_array_property);

	int32_array_property = nih_alloc (NULL, sizeof (int32_t) * value_len);
	if (! int32_array_property)
		nih_return_no_memory_error (-1);

	memcpy (int32_array_property, value, sizeof (int32_t) * value_len);
	int32_array_property_len = value_len;

	return 0;
}


char **str_array_property;

int
my_test_get_str_array (void *          data,
		       NihDBusMessage *message,
		       char ***        value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! *str_array_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrArray.Empty",
				      "The property array was empty");
		return -1;
	} else if (str_array_property[0] && str_array_property[1]
		   && str_array_property[2] && str_array_property[3]
		   && (! str_array_property[4])) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = nih_str_array_copy (message, NULL, str_array_property);
	if (! *value)
		nih_return_no_memory_error (-1);

	return 0;
}

int
my_test_set_str_array (void *          data,
		       NihDBusMessage *message,
		       char * const *  value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! *value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StrArray.Empty",
				      "The property array was empty");
		return -1;
	} else if (value[0] && value[1] && value[2] && value[3]
		   && (! value[4])) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (str_array_property)
		nih_free (str_array_property);

	str_array_property = nih_str_array_copy (NULL, NULL, value);
	if (! str_array_property)
		nih_return_no_memory_error (-1);

	return 0;
}


int32_t **int32_array_array_property;
size_t *  int32_array_array_property_len;

int
my_test_get_int32_array_array (void *          data,
			       NihDBusMessage *message,
			       int32_t ***     value,
			       size_t **       value_len)
{
	size_t array_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);
	TEST_NE_P (value_len, NULL);

	if (! *int32_array_array_property) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32ArrayArray.Empty",
				      "The property array was empty");
		return -1;
	} else if (int32_array_array_property[0]
		   && (! int32_array_array_property[1])) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	array_size = 0;
	for (int32_t **array = int32_array_array_property; array && *array; array++)
		array_size++;

	*value = nih_alloc (message, sizeof (int32_t *) * (array_size + 1));
	if (! *value)
		nih_return_no_memory_error (-1);

	*value_len = nih_alloc (message, sizeof (size_t) * array_size);
	if (! *value_len) {
		nih_free (*value);
		*value = NULL;
		nih_return_no_memory_error (-1);
	}

	array_size = 0;
	for (int32_t **array = int32_array_array_property; array && *array; array++) {
		(*value)[array_size] = nih_alloc (*value,
						  sizeof (int32_t) * int32_array_array_property_len[array_size]);
		if (! (*value)[array_size]) {
			nih_free (*value);
			*value = NULL;

			nih_free (*value_len);
			*value_len = NULL;

			nih_return_no_memory_error (-1);
		}

		memcpy ((*value)[array_size], int32_array_array_property[array_size],
			sizeof (int32_t) * int32_array_array_property_len[array_size]);
		(*value_len)[array_size] = int32_array_array_property_len[array_size];

		array_size++;
	}

	(*value)[array_size] = NULL;

	return 0;
}

int
my_test_set_int32_array_array (void *           data,
			       NihDBusMessage * message,
			       int32_t * const *value,
			       size_t *         value_len)
{
	size_t value_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! *value) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.Int32ArrayArray.Empty",
				      "The property array was empty");
		return -1;
	} else if (value[0] && (! value[1])) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (int32_array_array_property)
		nih_free (int32_array_array_property);
	if (int32_array_array_property_len)
		nih_free (int32_array_array_property_len);

	int32_array_array_property = NULL;
	int32_array_array_property_len = NULL;

	value_size = 0;
	for (int32_t * const *array = value; array && *array; array++)
		value_size++;

	int32_array_array_property = nih_alloc (NULL, sizeof (int32_t *) * (value_size + 1));
	if (! int32_array_array_property)
		nih_return_no_memory_error (-1);

	int32_array_array_property_len = nih_alloc (NULL, sizeof (size_t) * value_size);
	if (! int32_array_array_property_len) {
		nih_free (int32_array_array_property);
		int32_array_array_property = NULL;
		nih_return_no_memory_error (-1);
	}

	value_size = 0;
	for (int32_t * const *array = value; array && *array; array++) {
		int32_array_array_property[value_size] = nih_alloc (int32_array_array_property,
								    sizeof (int32_t) * value_len[value_size]);
		if (! int32_array_array_property[value_size]) {
			nih_free (int32_array_array_property);
			int32_array_array_property = NULL;

			nih_free (int32_array_array_property_len);
			int32_array_array_property_len = NULL;

			nih_return_no_memory_error (-1);
		}

		memcpy (int32_array_array_property[value_size], value[value_size],
			sizeof (int32_t) * value_len[value_size]);
		int32_array_array_property_len[value_size] = value_len[value_size];

		value_size++;
	}

	int32_array_array_property[value_size] = NULL;

	return 0;
}


MyStruct **struct_array_property;

int
my_test_get_struct_array (void *                      data,
			  NihDBusMessage *            message,
			  MyTestStructArrayElement ***value)
{
	MyTestStructArrayElement **tmp;
	size_t                     value_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! struct_array_property[0]) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StructArray.Empty",
				      "The property value was empty");
		return -1;
	} else if (! struct_array_property[1]) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = NULL;
	value_size = 0;

	for (MyStruct **element = struct_array_property; element && *element;
	     element++) {
		tmp = nih_realloc (*value, message,
				   sizeof (MyTestStructArrayElement *) * (value_size + 1));
		if (! tmp)
			goto error;

		*value = tmp;

		(*value)[value_size] = nih_new (*value, MyTestStructArrayElement);
		if (! (*value)[value_size])
			goto error;

		(*value)[value_size]->item0 = nih_strdup ((*value)[value_size],
							  (*element)->item0);
		if (! (*value)[value_size]->item0)
			goto error;

		(*value)[value_size]->item1 = (*element)->item1;

		value_size++;
	}

	tmp = nih_realloc (*value, message,
			   sizeof (MyTestStructArrayElement *) * (value_size + 1));
	if (! tmp)
		goto error;

	*value = tmp;
	(*value)[value_size] = NULL;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (*value) {
		nih_free (*value);
		*value = NULL;
	}
	return -1;
}

int
my_test_set_struct_array (void *          data,
			  NihDBusMessage *message,
			  MyTestStructArrayElement * const *value)
{
	MyStruct **tmp;
	size_t     value_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! value[0]) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.StructArray.Empty",
				      "The property value was empty");
		return -1;
	} else if (! value[1]) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (struct_array_property)
		nih_free (struct_array_property);

	struct_array_property = NULL;
	value_size = 0;

	for (MyTestStructArrayElement * const *element = value;
	     element && *element; element++) {
		tmp = nih_realloc (struct_array_property, NULL,
				   sizeof (MyStruct *) * (value_size + 1));
		if (! tmp)
			goto error;

		struct_array_property = tmp;

		struct_array_property[value_size] = nih_new (struct_array_property,
							     MyStruct);
		if (! struct_array_property[value_size])
			goto error;

		struct_array_property[value_size]->item0 = nih_strdup (struct_array_property[value_size],
								       (*element)->item0);
		if (! struct_array_property[value_size]->item0)
			goto error;

		struct_array_property[value_size]->item1 = (*element)->item1;

		value_size++;
	}

	tmp = nih_realloc (struct_array_property, NULL,
			   sizeof (MyStruct *) * (value_size + 1));
	if (! tmp)
		goto error;

	struct_array_property = tmp;
	struct_array_property[value_size] = NULL;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (struct_array_property) {
		nih_free (struct_array_property);
		struct_array_property = NULL;
	}
	return -1;
}


MyStruct **dict_entry_array_property;

int
my_test_get_dict_entry_array (void *                      data,
			      NihDBusMessage *            message,
			      MyTestDictEntryArrayElement ***value)
{
	MyTestDictEntryArrayElement **tmp;
	size_t                     value_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! dict_entry_array_property[0]) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.DictEntryArray.Empty",
				      "The property value was empty");
		return -1;
	} else if (! dict_entry_array_property[1]) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	*value = NULL;
	value_size = 0;

	for (MyStruct **element = dict_entry_array_property; element && *element;
	     element++) {
		tmp = nih_realloc (*value, message,
				   sizeof (MyTestDictEntryArrayElement *) * (value_size + 1));
		if (! tmp)
			goto error;

		*value = tmp;

		(*value)[value_size] = nih_new (*value, MyTestDictEntryArrayElement);
		if (! (*value)[value_size])
			goto error;

		(*value)[value_size]->item0 = nih_strdup ((*value)[value_size],
							  (*element)->item0);
		if (! (*value)[value_size]->item0)
			goto error;

		(*value)[value_size]->item1 = (*element)->item1;

		value_size++;
	}

	tmp = nih_realloc (*value, message,
			   sizeof (MyTestDictEntryArrayElement *) * (value_size + 1));
	if (! tmp)
		goto error;

	*value = tmp;
	(*value)[value_size] = NULL;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (*value) {
		nih_free (*value);
		*value = NULL;
	}
	return -1;
}

int
my_test_set_dict_entry_array (void *          data,
			      NihDBusMessage *message,
			      MyTestDictEntryArrayElement * const *value)
{
	MyStruct **tmp;
	size_t     value_size;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (! value[0]) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.DictEntryArray.Empty",
				      "The property value was empty");
		return -1;
	} else if (! value[1]) {
		nih_error_raise (EINVAL, "Invalid argument");
		return -1;
	}

	if (dict_entry_array_property)
		nih_free (dict_entry_array_property);

	dict_entry_array_property = NULL;
	value_size = 0;

	for (MyTestDictEntryArrayElement * const *element = value;
	     element && *element; element++) {
		tmp = nih_realloc (dict_entry_array_property, NULL,
				   sizeof (MyStruct *) * (value_size + 1));
		if (! tmp)
			goto error;

		dict_entry_array_property = tmp;

		dict_entry_array_property[value_size] = nih_new (dict_entry_array_property,
								 MyStruct);
		if (! dict_entry_array_property[value_size])
			goto error;

		dict_entry_array_property[value_size]->item0 = nih_strdup (dict_entry_array_property[value_size],
									   (*element)->item0);
		if (! dict_entry_array_property[value_size]->item0)
			goto error;

		dict_entry_array_property[value_size]->item1 = (*element)->item1;

		value_size++;
	}

	tmp = nih_realloc (dict_entry_array_property, NULL,
 			   sizeof (MyStruct *) * (value_size + 1));
	if (! tmp)
		goto error;

	dict_entry_array_property = tmp;
	dict_entry_array_property[value_size] = NULL;

	return 0;
error:
	nih_error_raise_no_memory ();
	if (dict_entry_array_property) {
		nih_free (dict_entry_array_property);
		dict_entry_array_property = NULL;
	}
	return -1;
}


int unix_fd_property;

int
my_test_get_unix_fd (void *          data,
		     NihDBusMessage *message,
		     int *           value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (value, NULL);

	if (unix_fd_property < 0) {
		nih_dbus_error_raise ("com.netsplit.Nih.Test.UnixFd.Invalid",
				      "The property value was invalid");
		return -1;
	}

	*value = unix_fd_property;

	return 0;
}

int
my_test_set_unix_fd (void *          data,
		     NihDBusMessage *message,
		     int             value)
{
	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->connection, NULL);
	TEST_NE_P (message->message, NULL);

	unix_fd_property = value;

	return 0;
}
