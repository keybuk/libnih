/* libnih
 *
 * test_dbus_proxy.c - test suite for nih-dbus/dbus_proxy.c
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
