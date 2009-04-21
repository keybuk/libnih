/* libnih
 *
 * test_dbus_proxy.c - test suite for nih-dbus/dbus_proxy.c
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
#include <nih-dbus/test_dbus.h>

#include <dbus/dbus.h>

#include <nih/macros.h>
#include <nih/alloc.h>

#include <nih-dbus/dbus_proxy.h>


void
test_proxy_new (void)
{
	DBusConnection *conn;
	NihDBusProxy *  proxy;

	TEST_FUNCTION ("nih_dbus_proxy_new");

	/* Check that we can create a proxy for a remote object with all of
	 * the right details filled in.
	 */
	TEST_FEATURE ("with destination name");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	assert (conn != NULL);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	TEST_ALLOC_FAIL {
		proxy = nih_dbus_proxy_new (NULL, conn, "com.netsplit.Nih",
					    "/com/netsplit/Nih");

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_ALLOC_PARENT (proxy->name, proxy);
		TEST_EQ_STR (proxy->name, "com.netsplit.Nih");

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->conn, conn);

		nih_free (proxy);
	}

	dbus_connection_unref (conn);

	dbus_shutdown ();


	/* Check that we can create a proxy for a remote object without
	 * a destination name in mind.
	 */
	TEST_FEATURE ("without destination name");
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	assert (conn != NULL);

	dbus_connection_set_exit_on_disconnect (conn, FALSE);

	TEST_ALLOC_FAIL {
		proxy = nih_dbus_proxy_new (NULL, conn, NULL,
					    "/com/netsplit/Nih");

		if (test_alloc_failed) {
			TEST_EQ_P (proxy, NULL);

			continue;
		}

		TEST_ALLOC_SIZE (proxy, sizeof (NihDBusProxy));

		TEST_EQ_P (proxy->name, NULL);

		TEST_ALLOC_PARENT (proxy->path, proxy);
		TEST_EQ_STR (proxy->path, "/com/netsplit/Nih");

		TEST_EQ_P (proxy->conn, conn);

		nih_free (proxy);
	}

	dbus_connection_unref (conn);

	dbus_shutdown ();
}


int
main (int   argc,
      char *argv[])
{
	test_proxy_new ();

	return 0;
}
