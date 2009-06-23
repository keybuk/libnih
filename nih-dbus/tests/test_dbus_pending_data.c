/* libnih
 *
 * test_dbus_pending_data.c - test suite for nih-dbus/dbus_pending_data.c
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
#include <nih-dbus/test_dbus.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>

#include <nih-dbus/dbus_connection.h>

#include <nih-dbus/dbus_message.h>
#include <nih-dbus/dbus_pending_data.h>


void
my_reply_handler (void *          data,
		  NihDBusMessage *message,
		  const char *    arg1,
		  int32_t         arg2)
{
}

void
my_error_handler (void *          data,
		  NihDBusMessage *message)
{
}


void
test_new (void)
{
	NihDBusPendingData *pending_data;
	pid_t               dbus_pid;
	DBusConnection *    conn;

	TEST_FUNCTION ("nih_dbus_pending_data_new");
	TEST_DBUS (dbus_pid);
	TEST_DBUS_OPEN (conn);


	/* Check that we can create a new NihDBusPendingData structure,
	 * with all the details filled in correctly, and that it references
	 * the connection.
	 */
	TEST_FEATURE ("with handler");
	TEST_ALLOC_FAIL {
		pending_data = nih_dbus_pending_data_new (NULL, conn,
							  (NihDBusReplyHandler)my_reply_handler,
							  my_error_handler,
							  &conn);

		if (test_alloc_failed) {
			TEST_EQ_P (pending_data, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (pending_data, sizeof (NihDBusPendingData));
		TEST_EQ_P (pending_data->connection, conn);
		TEST_EQ_P (pending_data->handler, (NihDBusReplyHandler)my_reply_handler);
		TEST_EQ_P (pending_data->error_handler, my_error_handler);
		TEST_EQ_P (pending_data->data, &conn);

		nih_free (pending_data);
	}


	/* Check that the handler argument is optional and NULL may be
	 * specified for it.
	 */
	TEST_FEATURE ("with no handler");
	TEST_ALLOC_FAIL {
		pending_data = nih_dbus_pending_data_new (NULL, conn,
							  NULL,
							  my_error_handler,
							  &conn);

		if (test_alloc_failed) {
			TEST_EQ_P (pending_data, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (pending_data, sizeof (NihDBusPendingData));
		TEST_EQ_P (pending_data->connection, conn);
		TEST_EQ_P (pending_data->handler, NULL);
		TEST_EQ_P (pending_data->error_handler, my_error_handler);
		TEST_EQ_P (pending_data->data, &conn);

		nih_free (pending_data);
	}


	TEST_DBUS_CLOSE (conn);
	TEST_DBUS_END (dbus_pid);

	dbus_shutdown ();
}


int
main (int   argc,
      char *argv[])
{
	test_new ();

	return 0;
}
