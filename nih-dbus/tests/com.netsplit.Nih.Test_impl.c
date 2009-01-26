/* libnih
 *
 * com.netsplit.Nih.Test_impl.c - implementation of test object interfaces
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

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <nih/macros.h>
#include <nih/alloc.h>
#include <nih/string.h>
#include <nih/timer.h>
#include <nih/signal.h>
#include <nih/main.h>
#include <nih/error.h>
#include <nih/errors.h>

#include <nih-dbus/dbus.h>

#include "com.netsplit.Nih.Test_object.h"


static const NihDBusInterface *my_interfaces[] = {
	&com_netsplit_Nih_Test,
	&com_netsplit_Nih_Glue,
	NULL
};


int
my_test_method (void            *data,
		NihDBusMessage  *message,
		const char      *input,
		int32_t          flags,
		const char     **output)
{
	static int32_t last_flags = -1;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if ((flags == 1) && (last_flags != 1)) {
		last_flags = flags;

		nih_dbus_error_raise ("com.netsplit.Nih.IllegalValue",
				      "The value given was not legal");

		return -1;
	}

	if ((flags == 2) && (last_flags != 2)) {
		last_flags = flags;

		errno = ENOMEM;
		nih_error_raise_system ();
		return -1;
	}

	if ((flags == 3) && (last_flags != 3)) {
		last_flags = flags;

		errno = EBADF;
		nih_error_raise_system ();
		return -1;
	}

	last_flags = flags;
	*output = nih_strdup (message, input);

	return 0;
}


typedef struct async_method {
	NihDBusMessage *message;
	char           *input;
	int32_t         flags;
} AsyncMethod;

static void
async_method_reply (AsyncMethod *method,
		    NihTimer    *timer)
{
	int ret;

	assert (method != NULL);
	assert (timer != NULL);

	TEST_NOT_FREE (method->message);

	if (method->flags == 4) {
		DBusMessage *reply;

		reply = dbus_message_new_method_return (method->message->message);
		assert (reply != NULL);

		dbus_message_append_args (reply,
					  DBUS_TYPE_INT32, &method->flags,
					  DBUS_TYPE_INVALID);

		ret = dbus_connection_send (method->message->conn, reply, NULL);
		assert (ret);

		dbus_message_unref (reply);
		nih_free (method->message);

	} else if (method->flags == 5) {
		DBusMessage *reply;

		reply = dbus_message_new_method_return (method->message->message);
		assert (reply != NULL);

		dbus_message_append_args (reply,
					  DBUS_TYPE_STRING, &method->input,
					  DBUS_TYPE_INT32, &method->flags,
					  DBUS_TYPE_INVALID);

		ret = dbus_connection_send (method->message->conn, reply, NULL);
		assert (ret);

		dbus_message_unref (reply);
		nih_free (method->message);

	} else if (method->flags == 6) {
		DBusMessage *reply;

		reply = dbus_message_new_method_return (method->message->message);
		assert (reply != NULL);

		ret = dbus_connection_send (method->message->conn, reply, NULL);
		assert (ret);

		dbus_message_unref (reply);
		nih_free (method->message);

	} else {
		ret = my_test_async_method_reply (method->message,
						  method->input);
		TEST_EQ (ret, 0);

		TEST_FREE (method->message);
	}

	nih_free (method);
}

int
my_test_async_method (void           *data,
		      NihDBusMessage *message,
		      const char     *input,
		      int32_t         flags)
{
	static int32_t last_flags = -1;

	AsyncMethod *method;

	TEST_ALLOC_SIZE (message, sizeof (NihDBusMessage));
	TEST_NE_P (message->conn, NULL);
	TEST_NE_P (message->message, NULL);

	TEST_NE_P (input, NULL);

	if ((flags == 1) && (last_flags != 1)) {
		last_flags = flags;

		nih_dbus_error_raise ("com.netsplit.Nih.IllegalValue",
				      "The value given was not legal");

		return -1;
	}

	if ((flags == 2) && (last_flags != 2)) {
		last_flags = flags;

		errno = ENOMEM;
		nih_error_raise_system ();
		return -1;
	}

	if ((flags == 3) && (last_flags != 3)) {
		last_flags = flags;

		errno = EBADF;
		nih_error_raise_system ();
		return -1;
	}

	NIH_MUST (method = nih_new (NULL, AsyncMethod));

	nih_ref (message, method);
	method->message = message;
	TEST_FREE_TAG (method->message);

	NIH_MUST (method->input = nih_strdup (method, input));

	method->flags = flags;

	NIH_MUST (nih_timer_add_timeout (NULL, 1,
					 (NihTimerCb)async_method_reply,
					 method));

	last_flags = flags;

	return 0;
}


int
my_byte_to_str (void            *data,
		NihDBusMessage  *message,
		uint8_t          input,
		const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%hhu", (unsigned char)input);

	return 0;
}

int
my_str_to_byte (void           *data,
		NihDBusMessage *message,
		const char     *input,
		uint8_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint8_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_boolean_to_str (void            *data,
		   NihDBusMessage  *message,
		   int              input,
		   const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input ? "True" : "False");

	return 0;
}

int
my_str_to_boolean (void           *data,
		   NihDBusMessage *message,
		   const char     *input,
		   int            *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	if (! strcmp (input, "False")) {
		*output = 0;
	} else {
		*output = 1;
	}

	return 0;
}

int
my_int16_to_str (void            *data,
		 NihDBusMessage  *message,
		 int16_t          input,
		 const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%hd", (short)input);

	return 0;
}

int
my_str_to_int16 (void           *data,
		 NihDBusMessage *message,
		 const char     *input,
		 int16_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (int16_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_uint16_to_str (void            *data,
		  NihDBusMessage  *message,
		  uint16_t         input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%hu", (unsigned short)input);

	return 0;
}

int
my_str_to_uint16 (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  uint16_t       *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint16_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_int32_to_str (void            *data,
		 NihDBusMessage  *message,
		 int32_t          input,
		 const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%d", (int)input);

	return 0;
}

int
my_str_to_int32 (void           *data,
		 NihDBusMessage *message,
		 const char     *input,
		 int32_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (int32_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_uint32_to_str (void            *data,
		  NihDBusMessage  *message,
		  uint32_t         input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%u", (unsigned int)input);

	return 0;
}

int
my_str_to_uint32 (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  uint32_t       *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint32_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_int64_to_str (void            *data,
		 NihDBusMessage  *message,
		 int64_t          input,
		 const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%lld", (long long)input);

	return 0;
}

int
my_str_to_int64 (void           *data,
		 NihDBusMessage *message,
		 const char     *input,
		 int64_t        *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (int64_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_uint64_to_str (void            *data,
		  NihDBusMessage  *message,
		  uint64_t         input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%llu", (unsigned long long)input);

	return 0;
}

int
my_str_to_uint64 (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  uint64_t       *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = (uint64_t)strtoimax (input, NULL, 10);

	return 0;
}

int
my_double_to_str (void            *data,
		  NihDBusMessage  *message,
		  double           input,
		  const char     **output)
{
	TEST_NE_P (output, NULL);

	*output = nih_sprintf (message, "%f", input);

	return 0;
}

int
my_str_to_double (void           *data,
		  NihDBusMessage *message,
		  const char     *input,
		  double         *output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = strtod (input, NULL);

	return 0;
}

int
my_object_path_to_str (void            *data,
		       NihDBusMessage  *message,
		       const char      *input,
		       const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_str_to_object_path (void            *data,
		       NihDBusMessage  *message,
		       const char      *input,
		       const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_signature_to_str (void            *data,
		     NihDBusMessage  *message,
		     const char      *input,
		     const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_str_to_signature (void            *data,
		     NihDBusMessage  *message,
		     const char      *input,
		     const char     **output)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (output, NULL);

	*output = nih_strdup (message, input);

	return 0;
}

int
my_int32_array_to_str (void            *data,
		       NihDBusMessage  *message,
		       int32_t         *array,
		       size_t           array_len,
		       const char     **output)
{
	char *str;

	TEST_NE_P (array, NULL);
	TEST_NE_P (output, NULL);

	str = NULL;

	for (size_t i = 0; i < array_len; i++)
		NIH_MUST (nih_strcat_sprintf (&str, message,
					      str ? " %d" : "%d",
					      (int)array[i]));

	*output = str;

	return 0;
}

int
my_str_to_int32_array (void            *data,
		       NihDBusMessage  *message,
		       const char      *input,
		       int32_t        **array,
		       size_t          *array_len)
{
	char **parts, **p;

	TEST_NE_P (input, NULL);
	TEST_NE_P (array, NULL);
	TEST_NE_P (array_len, NULL);

	*array = NULL;
	*array_len = 0;

	parts = nih_str_split (NULL, input, " ", FALSE);
	for (p = parts; p && *p; p++) {
		int      i;
		int32_t *new_array;

		sscanf (*p, "%d", &i);

		NIH_MUST (new_array = nih_realloc (
				  *array, message,
				  sizeof (int32_t) * (*array_len + 1)));
		*array = new_array;
		(*array)[(*array_len)++] = i;
	}

	nih_free (parts);

	return 0;
}

int
my_str_array_to_str (void            *data,
		     NihDBusMessage  *message,
		     const char     **array,
		     const char     **output)
{
	char       *str;
	const char **a;

	TEST_NE_P (array, NULL);
	TEST_NE_P (output, NULL);

	str = NULL;

	for (a = array; a && *a; a++)
		NIH_MUST (nih_strcat_sprintf (&str, message,
					      str ? " %s" : "%s", *a));

	*output = str;

	return 0;
}

int
my_str_to_str_array (void             *data,
		     NihDBusMessage   *message,
		     const char       *input,
		     const char     ***array)
{
	TEST_NE_P (input, NULL);
	TEST_NE_P (array, NULL);

	*array = (const char **)nih_str_split (message, input, " ", FALSE);

	return 0;
}


int
my_emit_signal (void           *data,
		NihDBusMessage *message,
		int32_t         signum)
{
	int ret = -1;

	switch (signum) {
	case 0:
		ret = my_test_signal (message->conn,
				      dbus_message_get_path (message->message),
				      "hello there", 0);
		break;
	case 1:
		ret = my_emit_byte (message->conn,
				    dbus_message_get_path (message->message),
				    65);
		break;
	case 2:
		ret = my_emit_boolean (message->conn,
				       dbus_message_get_path (message->message),
				       TRUE);
		break;
	case 3:
		ret = my_emit_int16 (message->conn,
				     dbus_message_get_path (message->message),
				     1701);
		break;
	case 4:
		ret = my_emit_uint16 (message->conn,
				      dbus_message_get_path (message->message),
				      1701);
		break;
	case 5:
		ret = my_emit_int32 (message->conn,
				     dbus_message_get_path (message->message),
				     1701);
		break;
	case 6:
		ret = my_emit_uint32 (message->conn,
				      dbus_message_get_path (message->message),
				      1701);
		break;
	case 7:
		ret = my_emit_int64 (message->conn,
				     dbus_message_get_path (message->message),
				     1701);
		break;
	case 8:
		ret = my_emit_uint64 (message->conn,
				      dbus_message_get_path (message->message),
				      1701);
		break;
	case 9:
		ret = my_emit_double (message->conn,
				      dbus_message_get_path (message->message),
				      3.141);
		break;
	case 10:
		ret = my_emit_string (message->conn,
				      dbus_message_get_path (message->message),
				      "test data");
		break;
	case 11:
		ret = my_emit_object_path (message->conn,
					   dbus_message_get_path (message->message),
					   "/com/netsplit/Nih");
		break;
	case 12:
		ret = my_emit_signature (message->conn,
					 dbus_message_get_path (message->message),
					 "a{sv}");
		break;
	case 13: {
		int32_t array[] = { 4, 8, 15, 16, 23, 42 };

		ret = my_emit_int32_array (message->conn,
					   dbus_message_get_path (message->message),
					   array, 6);
		break;
	}
	case 14: {
		char *array[] = { "this", "is", "a", "test", NULL };

		ret = my_emit_str_array (message->conn,
					 dbus_message_get_path (message->message),
					 array);
		break;
	}
	}

	TEST_EQ (ret, 0);

	return 0;
}


static DBusConnection *server_conn = NULL;

static int
my_connect_handler (DBusServer     *server,
		    DBusConnection *conn)
{
	NihDBusObject *object;

	assert (server_conn == NULL);
	server_conn = conn;

	object = nih_dbus_object_new (NULL, conn, "/com/netsplit/Nih",
				      my_interfaces, NULL);
	assert (object != NULL);

	return TRUE;
}

static pid_t server_pid;

DBusConnection *
my_setup (void)
{
	DBusConnection *conn;
	int             wait_fd = -1;

	TEST_CHILD_WAIT (server_pid, wait_fd) {
		DBusServer *server;

		nih_signal_set_handler (SIGTERM, nih_signal_handler);
		assert (nih_signal_add_handler (NULL, SIGTERM,
						nih_main_term_signal, NULL));

		server = nih_dbus_server ("unix:abstract=/com/netsplit/nih/test",
					  my_connect_handler, NULL);
		assert (server != NULL);

		TEST_CHILD_RELEASE (wait_fd);

		nih_main_loop ();

		if (server_conn) {
			dbus_connection_close (server_conn);
			dbus_connection_unref (server_conn);
		}

		dbus_server_disconnect (server);
		dbus_server_unref (server);

		dbus_shutdown ();

		exit (0);
	}

	conn = dbus_connection_open ("unix:abstract=/com/netsplit/nih/test",
				     NULL);
	assert (conn != NULL);

	return conn;
}

void
my_teardown (DBusConnection *conn)
{
	int status;

	kill (server_pid, SIGTERM);

	waitpid (server_pid, &status, 0);
	TEST_TRUE (WIFEXITED (status));
	TEST_EQ (WEXITSTATUS (status), 0);

	dbus_connection_unref (conn);

	dbus_shutdown ();
}

