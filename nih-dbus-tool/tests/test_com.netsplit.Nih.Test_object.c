/* libnih
 *
 * test_com.netsplit.Nih.Test_object.c - test suite for auto-generated
 * object bindings.
 *
 * Copyright © 2009 Scott James Remnant <scott@netsplit.com>.
 * Copyright © 2009 Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <nih/test.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>

#include "com.netsplit.Nih.Test_impl.h"


void
test_method_marshal (void)
{
	DBusConnection  *conn;
	DBusMessage     *message, *reply;
	DBusError        error;
	const char      *input, *output;
	dbus_unichar_t   byte_arg;
	dbus_bool_t      boolean_arg;
	dbus_int16_t     int16_arg;
	dbus_uint16_t    uint16_arg;
	dbus_int32_t     flags, int32_arg;
	dbus_uint32_t    uint32_arg;
	dbus_int64_t     int64_arg;
	dbus_uint64_t    uint64_arg;
	double           double_arg;
	dbus_int32_t    *int32_array;
	const char     **str_array;
	size_t           array_len;

	TEST_GROUP ("method marshalling");
	dbus_error_init (&error);


	/* Check that we can make a D-Bus method call, passing in the
	 * expected arguments and receiving an expected reply.
	 */
	TEST_FEATURE ("with valid argument");
	conn = my_setup ();

	input = "test data";
	flags = 0;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "test data");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises a D-Bus error and
	 * returns non-zero, the error is returned as a real D-Bus error
	 * with the same name and message.
	 */
	TEST_FEATURE ("with returned D-Bus error");
	conn = my_setup ();

	input = "test data";
	flags = 1;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, "com.netsplit.Nih.IllegalValue");

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises ENOMEM and returns
	 * non-zero, the D-Bus need more memory condition is returned which
	 * will make D-Bus repeat the method handler (at which point it
	 * will work).
	 */
	TEST_FEATURE ("with out of memory error");
	conn = my_setup ();

	input = "test data";
	flags = 2;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "test data");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the D-Bus method handler raises a different error
	 * and returns non-zero, the generic D-Bus Failed error is returned.
	 */
	TEST_FEATURE ("with unknown error");
	conn = my_setup ();

	input = "test data";
	flags = 3;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_FAILED);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with the wrong argument
	 * type, we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with wrong argument type");
	conn = my_setup ();

	input = "test data";
	output = "not test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_STRING, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call with too many arguments,
	 * we also get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with too many arguments");
	conn = my_setup ();

	input = "test data";
	flags = 0;
	output = "not test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_STRING, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the method call without enough arguments,
	 * we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with missing arguments");
	conn = my_setup ();

	input = "test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");

	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we make the method call without any arguments,
	 * we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with no arguments");
	conn = my_setup ();

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we say that we're not expecting a reply, none will
	 * be generated as allowed by the D-Bus spec.
	 */
	TEST_FEATURE ("with drive-by call");
	conn = my_setup ();

	input = "test data";
	flags = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	dbus_message_set_no_reply (message, TRUE);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   500, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_NO_REPLY);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that we can make an asynchronous D-Bus method call,
	 * passing in the expected arguments and receiving an expected
	 * reply even though it's generated by a timer callback.
	 */
	TEST_FEATURE ("with valid argument to async call");
	conn = my_setup ();

	input = "test data";
	flags = 0;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "test data");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the asynchronous D-Bus method handler raises a
	 * D-Bus error and returns non-zero, the error is returned as a
	 * real D-Bus error with the same name and message.
	 */
	TEST_FEATURE ("with returned D-Bus error from async call");
	conn = my_setup ();

	input = "test data";
	flags = 1;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, "com.netsplit.Nih.IllegalValue");

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the async D-Bus method handler raises ENOMEM and
	 * returns non-zero, the D-Bus need more memory condition is
	 * returned which will make D-Bus repeat the method handler (at
	 * which point it will work).
	 */
	TEST_FEATURE ("with out of memory error from async call");
	conn = my_setup ();

	input = "test data";
	flags = 2;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "test data");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if the async D-Bus method handler raises a different
	 * error and returns non-zero, the generic D-Bus Failed error is
	 * returned.
	 */
	TEST_FEATURE ("with unknown error from async call");
	conn = my_setup ();

	input = "test data";
	flags = 3;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_FAILED);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the async method call with the wrong
	 * argument type, we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with wrong argument type from async call");
	conn = my_setup ();

	input = "test data";
	output = "not test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_STRING, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the async method call with too many
	 * arguments, we also get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with too many arguments from async call");
	conn = my_setup ();

	input = "test data";
	flags = 0;
	output = "not test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_STRING, &output,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_error_free (&error);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that if we make the async method call without enough
	 * arguments, we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with missing arguments from async call");
	conn = my_setup ();

	input = "test data";

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");

	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we make the async method call without any
	 * arguments, we get the D-Bus invalid arguments error back.
	 */
	TEST_FEATURE ("with no arguments from async call");
	conn = my_setup ();

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_INVALID_ARGS);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that if we say that we're not expecting a reply, none will
	 * be generated as allowed by the D-Bus spec; even though the
	 * timer will be fired.
	 */
	TEST_FEATURE ("with drive-by async call");
	conn = my_setup ();

	input = "test data";
	flags = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"TestAsyncMethod");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INT32, &flags,
				  DBUS_TYPE_INVALID);

	dbus_message_set_no_reply (message, TRUE);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   2500, &error);

	TEST_EQ_P (reply, NULL);
	TEST_EQ_STR (error.name, DBUS_ERROR_NO_REPLY);

	dbus_message_unref (message);

	dbus_error_free (&error);

	my_teardown (conn);


	/* Check that an input argument of Byte type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Byte input argument");
	conn = my_setup ();

	byte_arg = 65;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"ByteToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_BYTE, &byte_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "65");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Byte type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Byte output argument");
	conn = my_setup ();

	input = "65";
	byte_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToByte");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_BYTE, &byte_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (byte_arg, 65);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Boolean type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Boolean input argument");
	conn = my_setup ();

	boolean_arg = 1;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"BooleanToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_BOOLEAN, &boolean_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "True");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Boolean type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Boolean output argument");
	conn = my_setup ();

	input = "False";
	boolean_arg = TRUE;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToBoolean");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_BOOLEAN, &boolean_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (boolean_arg, FALSE);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Int16 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int16 input argument");
	conn = my_setup ();

	int16_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int16ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT16, &int16_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Int16 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int16 output argument");
	conn = my_setup ();

	input = "1701";
	int16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt16");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT16, &int16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int16_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of UInt16 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt16 input argument");
	conn = my_setup ();

	uint16_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"UInt16ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_UINT16, &uint16_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of UInt16 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt16 output argument");
	conn = my_setup ();

	input = "1701";
	uint16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToUInt16");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_UINT16, &uint16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint16_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Int32 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int32 input argument");
	conn = my_setup ();

	int32_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int32ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &int32_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Int32 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int32 output argument");
	conn = my_setup ();

	input = "1701";
	int32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt32");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT32, &int32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int32_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of UInt32 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt32 input argument");
	conn = my_setup ();

	uint32_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"UInt32ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_UINT32, &uint32_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of UInt32 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt32 output argument");
	conn = my_setup ();

	input = "1701";
	uint32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToUInt32");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_UINT32, &uint32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint32_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


 	/* Check that an input argument of Int64 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Int64 input argument");
	conn = my_setup ();

	int64_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int64ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_INT64, &int64_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Int64 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Int64 output argument");
	conn = my_setup ();

	input = "1701";
	int64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt64");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_INT64, &int64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int64_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of UInt64 type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with UInt64 input argument");
	conn = my_setup ();

	uint64_arg = 1701;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"UInt64ToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_UINT64, &uint64_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "1701");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of UInt64 type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with UInt64 output argument");
	conn = my_setup ();

	input = "1701";
	uint64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToUInt64");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_UINT64, &uint64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint64_arg, 1701);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of Double type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Double input argument");
	conn = my_setup ();

	double_arg = 3.141592;
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"DoubleToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_DOUBLE, &double_arg,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "3.141592");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Double type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Double output argument");
	conn = my_setup ();

	input = "3.141";
	double_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToDouble");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_DOUBLE, &double_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (double_arg, 3.141);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of ObjectPatch type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with ObjectPath input argument");
	conn = my_setup ();

	input = "/com/netsplit/Nih";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"ObjectPathToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_OBJECT_PATH, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "/com/netsplit/Nih");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of ObjectPath type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with ObjectPath output argument");
	conn = my_setup ();

	input = "/com/netsplit/Nih";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToObjectPath");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_OBJECT_PATH, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "/com/netsplit/Nih");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of Signature type is marshalled
	 * correctly.
	 */
	TEST_FEATURE ("with Signature input argument");
	conn = my_setup ();

	input = "a{sv}";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"SignatureToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_SIGNATURE, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "a{sv}");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an output argument of Signature type is dispatched
	 * correctly.
	 */
	TEST_FEATURE ("with Signature output argument");
	conn = my_setup ();

	input = "a{sv}";
	output = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToSignature");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_SIGNATURE, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "a{sv}");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of Array type with Int32 members
	 * is marshalled correctly.
	 */
	TEST_FEATURE ("with Int32 Array input argument");
	conn = my_setup ();

	int32_array = nih_alloc (NULL, sizeof (int32_t) * 6);
	int32_array[0] = 4;
	int32_array[1] = 8;
	int32_array[2] = 15;
	int32_array[3] = 16;
	int32_array[4] = 23;
	int32_array[5] = 42;
	array_len = 6;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"Int32ArrayToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_ARRAY, DBUS_TYPE_INT32,
				  &int32_array, array_len,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "4 8 15 16 23 42");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	nih_free (int32_array);

	my_teardown (conn);


	/* Check that an output argument of Array type with Int32 elements
	 * is dispatched correctly.
	 */
	TEST_FEATURE ("with Int32 Array output argument");
	conn = my_setup ();

	input = "4 8 15 16 23 42";
	int32_array = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToInt32Array");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_ARRAY, DBUS_TYPE_INT32,
					  &int32_array, &array_len,
					  DBUS_TYPE_INVALID));

	TEST_NE_P (int32_array, NULL);
	TEST_EQ (array_len, 6);
	TEST_EQ (int32_array[0], 4);
	TEST_EQ (int32_array[1], 8);
	TEST_EQ (int32_array[2], 15);
	TEST_EQ (int32_array[3], 16);
	TEST_EQ (int32_array[4], 23);
	TEST_EQ (int32_array[5], 42);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that an input argument of Array type with String members
	 * is marshalled correctly.
	 */
	TEST_FEATURE ("with String Array input argument");
	conn = my_setup ();

	str_array = nih_alloc (NULL, sizeof (char *) * 4);
	str_array[0] = "this";
	str_array[1] = "is";
	str_array[2] = "a";
	str_array[3] = "test";
	array_len = 4;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrArrayToStr");
	dbus_message_append_args (message,
				  DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
				  &str_array, array_len,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_STRING, &output,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (output, "this is a test");

	dbus_message_unref (reply);
	dbus_message_unref (message);

	nih_free (str_array);

	my_teardown (conn);


	/* Check that an output argument of Array type with String elements
	 * is dispatched correctly.
	 */
	TEST_FEATURE ("with String Array output argument");
	conn = my_setup ();

	input = "this is a test";
	str_array = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Test",
						"StrToStrArray");
	dbus_message_append_args (message,
				  DBUS_TYPE_STRING, &input,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);
	TEST_TRUE (dbus_message_get_args (reply, &error,
					  DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
					  &str_array, &array_len,
					  DBUS_TYPE_INVALID));

	TEST_NE_P (str_array, NULL);
	TEST_EQ (array_len, 4);
	TEST_EQ_STR (str_array[0], "this");
	TEST_EQ_STR (str_array[1], "is");
	TEST_EQ_STR (str_array[2], "a");
	TEST_EQ_STR (str_array[3], "test");

	dbus_free_string_array ((char **)str_array);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	my_teardown (conn);
}


void
test_signal_dispatch (void)
{
	DBusConnection  *conn;
	DBusMessage     *message, *reply;
	DBusError        error;
	dbus_int32_t     signum, flags;
	const char      *str;
	dbus_unichar_t   byte_arg;
	dbus_bool_t      boolean_arg;
	dbus_int16_t     int16_arg;
	dbus_uint16_t    uint16_arg;
	dbus_int32_t     int32_arg;
	dbus_uint32_t    uint32_arg;
	dbus_int64_t     int64_arg;
	dbus_uint64_t    uint64_arg;
	double           double_arg;
	dbus_int32_t    *int32_array;
	const char     **str_array;
	size_t           array_len;

	TEST_GROUP ("signal dispatching");
	dbus_error_init (&error);

	/* Check that an ordinary signal can be emitted by the server with
	 * a set of arguments, and that we can catch it with them as we
	 * expected.  No particular error conditions to check for, since the
	 * only one is out of memory.
	 */
	TEST_FEATURE ("with ordinary signal");
	conn = my_setup ();

	signum = 0;
	str = NULL;
	flags = -1;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "TestSignal"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INT32, &flags,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "hello there");
	TEST_EQ (flags, 0);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Byte argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Byte argument");
	conn = my_setup ();

	signum = 1;
	byte_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitByte"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_BYTE, &byte_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (byte_arg, 65);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Boolean argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Boolean argument");
	conn = my_setup ();

	signum = 2;
	boolean_arg = FALSE;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitBoolean"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_BOOLEAN, &boolean_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (boolean_arg, TRUE);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Int16 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Int16 argument");
	conn = my_setup ();

	signum = 3;
	int16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt16"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_INT16, &int16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int16_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a UInt16 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with UInt16 argument");
	conn = my_setup ();

	signum = 4;
	uint16_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitUInt16"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_UINT16, &uint16_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint16_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Int32 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Int32 argument");
	conn = my_setup ();

	signum = 5;
	int32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt32"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_INT32, &int32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int32_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a UInt32 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with UInt32 argument");
	conn = my_setup ();

	signum = 6;
	uint32_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitUInt32"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_UINT32, &uint32_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint32_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Int64 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Int64 argument");
	conn = my_setup ();

	signum = 7;
	int64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt64"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_INT64, &int64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (int64_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a UInt64 argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with UInt64 argument");
	conn = my_setup ();

	signum = 8;
	uint64_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitUInt64"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_UINT64, &uint64_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (uint64_arg, 1701);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Double argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with Double argument");
	conn = my_setup ();

	signum = 9;
	double_arg = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitDouble"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_DOUBLE, &double_arg,
					  DBUS_TYPE_INVALID));

	TEST_EQ (double_arg, 3.141);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a String argument can be emitted and that
	 * we can catch it as expected.
	 */
	TEST_FEATURE ("with String argument");
	conn = my_setup ();

	signum = 10;
	str = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitString"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_STRING, &str,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "test data");

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a ObjectPath argument can be emitted and
	 * that we can catch it as expected.
	 */
	TEST_FEATURE ("with ObjectPath argument");
	conn = my_setup ();

	signum = 11;
	str = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitObjectPath"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_OBJECT_PATH, &str,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "/com/netsplit/Nih");

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Signature argument can be emitted and
	 * that we can catch it as expected.
	 */
	TEST_FEATURE ("with Signature argument");
	conn = my_setup ();

	signum = 12;
	str = NULL;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitSignature"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_SIGNATURE, &str,
					  DBUS_TYPE_INVALID));

	TEST_EQ_STR (str, "a{sv}");

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Array argument and Int32 elements
	 * can be emitted and that we can catch it as expected.
	 */
	TEST_FEATURE ("with Int32 Array argument");
	conn = my_setup ();

	signum = 13;
	int32_array = NULL;
	array_len = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitInt32Array"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_ARRAY, DBUS_TYPE_INT32,
					  &int32_array, &array_len,
					  DBUS_TYPE_INVALID));

	TEST_NE_P (int32_array, NULL);
	TEST_EQ (int32_array[0], 4);
	TEST_EQ (int32_array[1], 8);
	TEST_EQ (int32_array[2], 15);
	TEST_EQ (int32_array[3], 16);
	TEST_EQ (int32_array[4], 23);
	TEST_EQ (int32_array[5], 42);
	TEST_EQ (array_len, 6);

	dbus_message_unref (message);

	my_teardown (conn);


	/* Check that a signal with a Array argument and String elements
	 * can be emitted and that we can catch it as expected.
	 */
	TEST_FEATURE ("with String Array argument");
	conn = my_setup ();

	signum = 14;
	str_array = NULL;
	array_len = 0;

	message = dbus_message_new_method_call (NULL, "/com/netsplit/Nih",
						"com.netsplit.Nih.Glue",
						"EmitSignal");

	dbus_message_append_args (message,
				  DBUS_TYPE_INT32, &signum,
				  DBUS_TYPE_INVALID);

	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, &error);

	TEST_NE_P (reply, NULL);

	dbus_message_unref (reply);
	dbus_message_unref (message);

	message = dbus_connection_pop_message (conn);

	TEST_NE_P (message, NULL);
	TEST_TRUE (dbus_message_is_signal (message, "com.netsplit.Nih.Test",
					   "EmitStrArray"));
	TEST_TRUE (dbus_message_get_args (message, &error,
					  DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
					  &str_array, &array_len,
					  DBUS_TYPE_INVALID));

	TEST_NE_P (str_array, NULL);
	TEST_EQ_STR (str_array[0], "this");
	TEST_EQ_STR (str_array[1], "is");
	TEST_EQ_STR (str_array[2], "a");
	TEST_EQ_STR (str_array[3], "test");
	TEST_EQ (array_len, 4);

	dbus_free_string_array ((char **)str_array);

	dbus_message_unref (message);

	my_teardown (conn);
}


int
main (int   argc,
      char *argv[])
{
	test_method_marshal ();
	test_signal_dispatch ();

	return 0;
}
